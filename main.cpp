#include<DxLib.h>
#include<MidiPlayer.h>
#include"MIDIScreen.h"

class VMPlayer
{
public:
	int Init(const TCHAR* param);
	void Run();
	int End();
private:
	void OnDraw();
	void OnLoop();
	bool LoadMIDI(const TCHAR* path);
	void DrawTime();
	void UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path);
	MIDIScreen ms;
	MidiPlayer mp;
	bool running = true;
	bool sendlong;
	bool loopOn = true;
	int windowed;
	int screenWidth, screenHeight;
	int millisec = 0, minute = 0, second = 0, bar = 0, step = 0, tick = 0;
	unsigned volume = 100u;
	int posLoopStart, posLoopEnd;
	TCHAR filepath[MAX_PATH] = L"";
	TCHAR szStr[150];
	TCHAR szTimeInfo[80];
	TCHAR szLastTick[12] = TEXT("1:01:000");

	const int stepsperbar = 4;
};

int VMPlayer::Init(const TCHAR* param)
{
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
	GetDrawScreenSize(&screenWidth, &screenHeight);
	if (DxLib_Init() != 0)return -1;
	SetDrawScreen(DX_SCREEN_BACK);
	mp.SetSendLongMsg(sendlong = false);
	ms.SetPlayerSrc(&mp);
	ms.SetRectangle(4, 18, w - 8, h - 54);
	UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	return 0;
}

int VMPlayer::End()
{
	mp.Stop();
	mp.Unload();
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
	if (!mp.LoadFile(p))return false;
	sprintfDx(loopfilepath, TEXT("%s.txt"), path);
	int hLoopFile = FileRead_open(loopfilepath);
	if (hLoopFile != -1 && hLoopFile != 0)
	{
		FileRead_scanf(hLoopFile, TEXT("%d %d"), &posLoopStart, &posLoopEnd);
		mp.SetLoop((float)posLoopStart, (float)posLoopEnd);
		loopOn = true;
	}
	else
	{
		posLoopStart = posLoopEnd = 0;
		mp.SetLoop(0.0f, 0.0f);
		loopOn = false;
	}
	FileRead_close(hLoopFile);
	tick = (int)mp.GetLastEventTick();
	step = tick / mp.GetQuarterNoteTicks();
	tick %= mp.GetQuarterNoteTicks();
	bar = step / stepsperbar;
	step %= stepsperbar;
	swprintf_s(szLastTick, TEXT("%d:%02d:%03d"), bar + 1, step + 1, tick);
	return true;
}

void VMPlayer::DrawTime()
{
	millisec = mp.GetLastEventTick() ? (int)(mp.GetPosTimeInSeconds()*1000.0) : 0;
	second = millisec / 1000;
	millisec %= 1000;
	minute = second / 60;
	second %= 60;
	tick = (int)mp.GetPosTick();
	step = tick / mp.GetQuarterNoteTicks();
	tick %= mp.GetQuarterNoteTicks();
	bar = step / stepsperbar;
	step %= stepsperbar;
	swprintf_s(szTimeInfo, TEXT("BPM:%5.3f 时间：%d:%02d.%03d Tick:%3d:%02d:%03d/%s 事件：%5d 复音数：%2d"), mp.GetBPM(),
		minute, second, millisec, bar + 1, step + 1, tick, szLastTick, mp.GetPosEventNum(), mp.GetPolyphone());
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
		mp.GetPlayStatus() ? mp.Pause() : mp.Play(true);
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_S))
	{
		mp.Stop(false);
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_O))
	{
		if (!windowed)ChangeWindowMode(windowed = TRUE);
		if (SelectFile(filepath, NULL))
			LoadMIDI(filepath);
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_E))
	{
		mp.SetSendLongMsg(sendlong = !sendlong);
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_L))
	{
		loopOn = !loopOn;
		if (mp.GetLastEventTick())
		{
			if (loopOn)
			{
				if (posLoopEnd)
					mp.SetLoop((float)posLoopStart, (float)posLoopEnd);
				else mp.SetLoop(0.0f, (float)mp.GetLastEventTick());
			}
			else mp.SetLoop(0.0f, 0.0f);
		}
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_UP))
	{
		volume = min(100, volume + 5);
		mp.SetVolume(volume);
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_DOWN))
	{
		if (volume)volume -= 5;
		mp.SetVolume(volume);
		UpdateString(szStr, ARRAYSIZE(szStr), mp.GetPlayStatus() == TRUE, filepath);
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
	snprintfDx(str, strsize, TEXT("Space:播放/暂停 S:停止 O:打开文件 Esc:退出 L:循环[%s] E:发送长消息[%s] ↑/↓:音量调整[%3d%%]\n%s：%s"),
		loopOn ? TEXT("开") : TEXT("关"), sendlong ? TEXT("开") : TEXT("关"),
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