#include<DxLib.h>
#include<MidiPlayer.h>
#include"MIDIScreen.h"

#define VMP_TEMP_FILENAME "vmp_temp.mid"

class VMPlayer
{
public:
	VMPlayer();
	~VMPlayer();
	int Init(const TCHAR* param);
	void Run();
	int End();
protected:
	void _OnFinishPlayCallback();
	static VMPlayer* _pObj;
private:
	unsigned ChooseDevice();
	void OnDraw();
	void OnLoop();
	bool LoadMIDI(const TCHAR* path);
	void DrawTime();
	void UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path);
	MIDIScreen ms;
	MidiPlayer* pmp;
	bool running = true;
	bool sendlong;
	bool loopOn = true;
	bool pressureOn = true;
	int windowed;
	int screenWidth, screenHeight;
	int millisec = 0, minute = 0, second = 0, bar = 0, step = 0, tick = 0;
	unsigned volume = 100u;
	int posLoopStart, posLoopEnd;
	TCHAR filepath[MAX_PATH] = L"";
	TCHAR szStr[156];
	TCHAR szTimeInfo[80];
	TCHAR szLastTick[12] = TEXT("1:01:000");

	const int stepsperbar = 4;
};

VMPlayer* VMPlayer::_pObj = nullptr;

VMPlayer::VMPlayer()
{
}

VMPlayer::~VMPlayer()
{
}

unsigned VMPlayer::ChooseDevice()
{
	unsigned nMidiDev = midiOutGetNumDevs(), cur = 0;
	TCHAR str[120];
	MIDIOUTCAPS caps;
	bool loop = true;
	if (nMidiDev > 1)
	{
		while (loop)
		{
			if (ProcessMessage() == -1)loop = false;
			ClearDrawScreen();
			midiOutGetDevCaps(cur, &caps, sizeof caps);
			sprintfDx(str, TEXT("选择 MIDI 输出设备：\n%2d:%s"), (int)cur, caps.szPname);
			DrawString(0, 0, str, 0x00FFFFFF);
			ScreenFlip();
			switch (WaitKey())
			{
			case KEY_INPUT_UP:
				if (cur != MIDI_MAPPER)cur--;
				break;
			case KEY_INPUT_DOWN:
				if (cur != nMidiDev - 1)cur++;
				break;
			case KEY_INPUT_ESCAPE:running = loop = false; break;
			case KEY_INPUT_RETURN:loop = false; break;
			}
		}
	}
	return cur;
}

int VMPlayer::Init(const TCHAR* param)
{
	_pObj = this;
	int w = 800, h = 600;
	if (strcmpDx(param, TEXT("600p")) == 0)
	{
		w = 960;
		h = 600;
	}
	else if (strcmpDx(param, TEXT("720p")) == 0)
	{
		w = 1280;
		h = 720;
	}
	SetOutApplicationLogValidFlag(FALSE);
	ChangeWindowMode(windowed = TRUE);
	SetAlwaysRunFlag(TRUE);
	SetGraphMode(w, h, 32);
	ChangeFont(TEXT("SimSun"));
	SetFontSize(14);
	SetFontThickness(3);
	GetDrawScreenSize(&screenWidth, &screenHeight);
	if (DxLib_Init() != 0)return -1;
	SetDrawScreen(DX_SCREEN_BACK);

	pmp = new MidiPlayer(ChooseDevice());
	pmp->SetOnFinishPlay([](void*) {VMPlayer::_pObj->_OnFinishPlayCallback(); }, nullptr);
	pmp->SetSendLongMsg(sendlong = true);
	ms.SetPlayerSrc(pmp);
	ms.SetRectangle(4, 18, w - 8, h - 54);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	return 0;
}

int VMPlayer::End()
{
	pmp->Stop();
	pmp->Unload();
	delete pmp;
	return DxLib_End();
}

void VMPlayer::Run()
{
	while (running)
	{
		if (ProcessMessage())break;
		OnLoop();
		OnDraw();
	}
}

void VMPlayer::_OnFinishPlayCallback()
{
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
}

//http://blog.chinaunix.net/uid-23860671-id-189905.html
BOOL UnicodeToGBK(char* out,int outsize, const TCHAR* str)
{
	int needlength = WideCharToMultiByte(CP_OEMCP, NULL, str, -1, NULL, 0, NULL, FALSE);
	if (outsize < needlength)return FALSE;
	WideCharToMultiByte(CP_OEMCP, NULL, str, -1, out, outsize, NULL, FALSE);
	return TRUE;
}

bool VMPlayer::LoadMIDI(const TCHAR* path)
{
	TCHAR loopfilepath[MAX_PATH];
	char p[ARRAYSIZE(filepath) * 2];
	UnicodeToGBK(p, ARRAYSIZE(p), path);
	if (!pmp->LoadFile(p))
	{
#ifdef _UNICODE
#define _T_RENAME _wrename
#else
#define _T_RENAME rename
#endif
		if (_T_RENAME(path, TEXT(VMP_TEMP_FILENAME)))return false;
		pmp->LoadFile(VMP_TEMP_FILENAME);
		_T_RENAME(TEXT(VMP_TEMP_FILENAME), path);//特么……这样居然可以！！！（惊）
	}
	sprintfDx(loopfilepath, TEXT("%s.txt"), path);
	int hLoopFile = FileRead_open(loopfilepath);
	if (hLoopFile != -1 && hLoopFile != 0)
	{
		FileRead_scanf(hLoopFile, TEXT("%d %d"), &posLoopStart, &posLoopEnd);
		pmp->SetLoop((float)posLoopStart, (float)posLoopEnd);
		loopOn = true;
	}
	else
	{
		posLoopStart = posLoopEnd = 0;
		pmp->SetLoop(0.0f, 0.0f);
		loopOn = false;
	}
	FileRead_close(hLoopFile);
	tick = (int)pmp->GetLastEventTick();
	step = tick / pmp->GetQuarterNoteTicks();
	tick %= pmp->GetQuarterNoteTicks();
	bar = step / stepsperbar;
	step %= stepsperbar;
	swprintf_s(szLastTick, TEXT("%d:%02d:%03d"), bar + 1, step + 1, tick);
	return true;
}

void VMPlayer::DrawTime()
{
	millisec = pmp->GetLastEventTick() ? (int)(pmp->GetPosTimeInSeconds()*1000.0) : 0;
	second = millisec / 1000;
	millisec %= 1000;
	minute = second / 60;
	second %= 60;
	tick = (int)pmp->GetPosTick();
	step = tick / pmp->GetQuarterNoteTicks();
	tick %= pmp->GetQuarterNoteTicks();
	bar = step / stepsperbar;
	step %= stepsperbar;
	swprintf_s(szTimeInfo, TEXT("BPM:%5.3f 时间：%d:%02d.%03d Tick:%3d:%02d:%03d/%s 事件：%5d/%d 复音数：%2d"), pmp->GetBPM(),
		minute, second, millisec, bar + 1, step + 1, tick, szLastTick, pmp->GetPosEventNum(), pmp->GetEventCount(),
		pmp->GetPolyphone());
	DrawString(0, 0, szTimeInfo, 0x00FFFFFF);
}

void VMPlayer::OnDraw()
{
	ClearDrawScreen();
	ms.Draw();
	DrawTime();
	DrawString(0, screenHeight - 36, szStr, 0x00FFFFFF);
	ScreenFlip();
}

BOOL SelectFile(TCHAR *filepath, TCHAR *filename)
{
	OPENFILENAME ofn = { sizeof OPENFILENAME };
	ofn.lStructSize = sizeof OPENFILENAME;
	ofn.hwndOwner = GetActiveWindow();
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = TEXT("MIDI 序列\0*.mid\0所有文件\0*\0\0");
	ofn.lpstrFile = filepath;
	ofn.lpstrTitle = TEXT("选择文件");
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = filename;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = TEXT("mid");
	return GetOpenFileName(&ofn);
}

class KeyManager
{
public:
	static bool CheckOnHitKey(int key)
	{
		if (CheckHitKey(key))
		{
			if (!pressedKey)
			{
				pressedKey = key;
				return true;
			}
		}
		else if (key == pressedKey)pressedKey = 0;
		return false;
	}
private:
	static int pressedKey;
};
int KeyManager::pressedKey = 0;

void VMPlayer::OnLoop()
{
	if (KeyManager::CheckOnHitKey(KEY_INPUT_ESCAPE))
		running = false;
	if (KeyManager::CheckOnHitKey(KEY_INPUT_F11))
	{
		windowed ^= TRUE;
		ChangeWindowMode(windowed);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_SPACE))
	{
		pmp->GetPlayStatus() ? pmp->Pause() : pmp->Play(true);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_S))
	{
		pmp->Stop(false);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_O))
	{
		if (!windowed)ChangeWindowMode(windowed = TRUE);
		if (SelectFile(filepath, NULL))
			LoadMIDI(filepath);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_E))
	{
		pmp->SetSendLongMsg(sendlong = !sendlong);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_L))
	{
		loopOn = !loopOn;
		if (pmp->GetLastEventTick())
		{
			if (loopOn)
			{
				if (posLoopEnd)
					pmp->SetLoop((float)posLoopStart, (float)posLoopEnd);
				else pmp->SetLoop(0.0f, (float)pmp->GetLastEventTick());
			}
			else pmp->SetLoop(0.0f, 0.0f);
		}
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_UP))
	{
		volume = min(100, volume + 5);
		pmp->SetVolume(volume);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_DOWN))
	{
		if (volume)volume -= 5;
		pmp->SetVolume(volume);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_P))
	{
		ms.SetPresentPressure(pressureOn = !pressureOn);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
}

void VMPlayer::UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path)
{
	TCHAR path2[80];
	if (strlenDx(path) > ARRAYSIZE(path2))
	{
		path = strrchrDx(path, TEXT('\\')) + 1;
		size_t n = strlenDx(path);
		if (n > ARRAYSIZE(path2))
		{
			strncpyDx(path2, path, ARRAYSIZE(path2));
			path = strrchrDx(path, TEXT('.'));
			n = ARRAYSIZE(path2) - 3 - strlenDx(path);
			sprintfDx(path2 + n, TEXT("..%s"), path);
			path = path2;
		}
	}
	snprintfDx(str, strsize, TEXT("Space:播放/暂停 S:停止 O:打开文件 Esc:退出 L:循环[%s] P:力度[%s] E:发送长消息[%s] ↑/↓:音量[%3d%%]\n%s：%s"),
		loopOn ? TEXT("开") : TEXT("关"), pressureOn ? TEXT("开") : TEXT("关"), sendlong ? TEXT("开") : TEXT("关"),
		volume, isplaying ? TEXT("正在播放") : TEXT("当前文件"), path[0] == TEXT('\0') ? TEXT("未选择") : path);
	if (posLoopEnd)
	{
		TCHAR szappend[20];
		snprintfDx(szappend, ARRAYSIZE(szappend), TEXT("[%d,%d]"), posLoopStart, posLoopEnd);
		strcatDx(str, szappend);
	}
}

#ifdef _UNICODE
int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	VMPlayer vmp;
	if (vmp.Init(lpCmdLine) == 0)
		vmp.Run();
	return vmp.End();
}