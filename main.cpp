#include<DxLib.h>
#include<MidiPlayer.h>
#include"MIDIScreen.h"

#define VMP_TEMP_FILENAME "vmp_temp.mid"

class DPIInfo
{
public:
	DPIInfo()
	{
		HDC h = GetDC(0);
		sx = GetDeviceCaps(h, LOGPIXELSX);
		sy = GetDeviceCaps(h, LOGPIXELSY);
	}
	template<typename Tnum>Tnum X(Tnum n)const { return n * sx / odpi; }
	template<typename Tnum>Tnum Y(Tnum n)const { return n * sy / odpi; }
private:
	const int odpi = USER_DEFAULT_SCREEN_DPI;
	int sx, sy;
};

class VMPlayer
{
public:
	int Init(TCHAR* param);
	void Run();
	int End();
protected:
	void _OnFinishPlayCallback();
	void _OnProgramChangeCallback(int channel, int program);
	bool OnLoadMIDI(TCHAR*);
	void OnCommandPlay();
	void OnDrop(HDROP);
	void OnSeekBar(int);
	static VMPlayer* _pObj;
private:
	static LRESULT CALLBACK ExtraProcess(HWND, UINT, WPARAM, LPARAM);
	static WNDPROC dxProcess;
	HWND hWindowDx;
	unsigned ChooseDevice();
	void OnDraw();
	void OnLoop();
	bool LoadMIDI(const TCHAR* path);
	void DrawTime();
	void UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path);
	void UpdateTextLastTick();
	MIDIScreen ms;
	MidiPlayer* pmp;
	bool running = true;
	bool sendlong;
	bool loopOn = true;
	bool pressureOn = true;
	int posYLowerText;
	int windowed;
	int screenWidth, screenHeight;
	int millisec = 0, minute = 0, second = 0, bar = 0, step = 0, tick = 0;
	unsigned volume = 100u;
	int posLoopStart, posLoopEnd;
	bool loopIncludeStart = false, loopIncludeEnd = true;
	TCHAR filepath[MAX_PATH] = L"";
	TCHAR szStr[156];
	TCHAR szTimeInfo[80];
	TCHAR szLastTick[12] = TEXT("1:01:000");

	int stepsperbar = 4;
};

VMPlayer* VMPlayer::_pObj = nullptr;
WNDPROC VMPlayer::dxProcess = nullptr;

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
			sprintfDx(str, TEXT("Enter:确认 Esc:退出 ↑/↓:选择\n选择 MIDI 输出设备：[%2d]%s"), (int)cur, caps.szPname);
			DrawString(0, posYLowerText, str, 0x00FFFFFF);
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

LRESULT CALLBACK VMPlayer::ExtraProcess(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DROPFILES://https://github.com/lxfly2000/XAPlayer/blob/master/Main.cpp#L301
		VMPlayer::_pObj->OnDrop((HDROP)wp);
		break;
	}
	return CallWindowProc(dxProcess, hwnd, msg, wp, lp);
}

int VMPlayer::Init(TCHAR* param)
{
	_pObj = this;
	int w = 800, h = 600;
	if (strcmpDx(param, TEXT("600p")) == 0)
	{
		w = 960;
		h = 600;
		param[0] = TEXT('\0');
	}
	else if (strcmpDx(param, TEXT("720p")) == 0)
	{
		w = 1280;
		h = 720;
		param[0] = TEXT('\0');
	}
	SetOutApplicationLogValidFlag(FALSE);
	TCHAR title[50] = TEXT("Visual MIDI Player");
	SetWindowText(title);
	ChangeWindowMode(windowed = TRUE);
	SetAlwaysRunFlag(TRUE);
	DPIInfo hdpi;
	SetGraphMode(hdpi.X(w), hdpi.Y(h), 32);
	ChangeFont(TEXT("SimSun"));
	SetFontSize(hdpi.X(14));
	if (hdpi.X(14) > 14)ChangeFontType(DX_FONTTYPE_ANTIALIASING);
	SetFontThickness(3);
	GetDrawScreenSize(&screenWidth, &screenHeight);
	if (DxLib_Init() != 0)return -1;
	SetDrawScreen(DX_SCREEN_BACK);

	posYLowerText = screenHeight - (GetFontSize() + 4) * 2;

	pmp = new MidiPlayer(strlenDx(param) ? MIDI_MAPPER : ChooseDevice());
	if (pmp->IsUsingWinRTMidi())
		lstrcat(title, TEXT(" (WinRT MIDI)"));
	else
		lstrcat(title, TEXT(" (WinMM)"));
	SetWindowText(title);
	pmp->SetOnFinishPlay([](void*) {VMPlayer::_pObj->_OnFinishPlayCallback(); }, nullptr);
	pmp->SetOnProgramChange([](int ch, int prog) {VMPlayer::_pObj->_OnProgramChangeCallback(ch, prog); });
	pmp->SetSendLongMsg(sendlong = true);
	ms.SetPlayerSrc(pmp);
	ms.SetRectangle(4, GetFontSize() + 4, hdpi.X(w) - 8, posYLowerText - (GetFontSize() + 4));
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);

	if (strlenDx(param))
	{
		if (param[0] == TEXT('\"'))
		{
			strcpyDx(filepath, param + 1);
			filepath[strlenDx(filepath) - 1] = TEXT('\0');
		}
		else strcpyDx(filepath, param);
		if (OnLoadMIDI(filepath))OnCommandPlay();
	}

	//http://nut-softwaredevelopper.hatenablog.com/entry/2016/02/25/001647
	hWindowDx = GetMainWindowHandle();
	dxProcess = (WNDPROC)GetWindowLongPtr(hWindowDx, GWLP_WNDPROC);
	SetWindowLongPtr(hWindowDx, GWL_EXSTYLE, WS_EX_ACCEPTFILES | GetWindowLongPtr(hWindowDx, GWL_EXSTYLE));
	SetWindowLongPtr(hWindowDx, GWLP_WNDPROC, (LONG_PTR)ExtraProcess);
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

void VMPlayer::_OnProgramChangeCallback(int channel, int program)
{
	ms.chPrograms[channel] = program;
}

bool VMPlayer::OnLoadMIDI(TCHAR* path)
{
	bool ok = true;
	if (!LoadMIDI(path))ok = false;
	if (!ok)strcatDx(path, TEXT("（无效文件）"));
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, path);
	return ok;
}

void VMPlayer::OnCommandPlay()
{
	pmp->GetPlayStatus() ? pmp->Pause() : pmp->Play(true);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
}

void VMPlayer::OnDrop(HDROP hdrop)
{
	DragQueryFile(hdrop, 0, filepath, MAX_PATH);
	DragFinish(hdrop);
	if (OnLoadMIDI(filepath))OnCommandPlay();
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
		bool ok = pmp->LoadFile(VMP_TEMP_FILENAME);
		_T_RENAME(TEXT(VMP_TEMP_FILENAME), path);//特么……这样居然可以！！！（惊）
		if (!ok)return false;
	}
	sprintfDx(loopfilepath, TEXT("%s.txt"), path);
	int hLoopFile = FileRead_open(loopfilepath);
	if (hLoopFile != -1 && hLoopFile != 0)
	{
		FileRead_scanf(hLoopFile, TEXT("%d %d"), &posLoopStart, &posLoopEnd);
		int la = 0, lb = 1;
		if (!FileRead_eof(hLoopFile))FileRead_scanf(hLoopFile, TEXT("%d"), &la);
		if (!FileRead_eof(hLoopFile))FileRead_scanf(hLoopFile, TEXT("%d"), &lb);
		loopIncludeStart = (bool)la;
		loopIncludeEnd = (bool)lb;
		pmp->SetLoop((float)posLoopStart, (float)posLoopEnd, loopIncludeStart, loopIncludeEnd);
		loopOn = true;
	}
	else
	{
		posLoopStart = posLoopEnd = 0;
		pmp->SetLoop(0.0f, 0.0f);
		loopOn = false;
	}
	FileRead_close(hLoopFile);
	UpdateTextLastTick();
	return true;
}

void VMPlayer::UpdateTextLastTick()
{
	tick = (int)pmp->GetLastEventTick();
	step = tick / pmp->GetQuarterNoteTicks();
	tick %= pmp->GetQuarterNoteTicks();
	bar = step / stepsperbar;
	step %= stepsperbar;
	swprintf_s(szLastTick, TEXT("%d:%d:%03d"), bar, step, tick);
}

void VMPlayer::DrawTime()
{
	if (stepsperbar != pmp->GetStepsPerBar())
	{
		stepsperbar = pmp->GetStepsPerBar();
		UpdateTextLastTick();
	}
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
	swprintf_s(szTimeInfo, TEXT("BPM:%5.3f 时间：%d:%02d.%03d Tick:%3d:%d:%03d/%s 事件：%5d/%d 复音数：%2d"), pmp->GetBPM(),
		minute, second, millisec, bar, step, tick, szLastTick, pmp->GetPosEventNum(), pmp->GetEventCount(),
		pmp->GetPolyphone());
	DrawString(0, 0, szTimeInfo, 0x00FFFFFF);
}

void VMPlayer::OnDraw()
{
	ClearDrawScreen();
	ms.Draw();
	DrawTime();
	DrawString(0, posYLowerText, szStr, 0x00FFFFFF);
	ScreenFlip();
}

BOOL SelectFile(TCHAR *filepath, TCHAR *filename)
{
	OPENFILENAME ofn = { sizeof OPENFILENAME };
	ofn.lStructSize = sizeof OPENFILENAME;
	ofn.hwndOwner = GetActiveWindow();
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = TEXT("MIDI 序列\0*.mid;*.rmi\0所有文件\0*\0\0");
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
		OnCommandPlay();
	if (KeyManager::CheckOnHitKey(KEY_INPUT_S))
	{
		pmp->Stop(false);
		UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_O))
	{
		if (!windowed)ChangeWindowMode(windowed = TRUE);
		if (SelectFile(filepath, NULL))
			OnLoadMIDI(filepath);
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
	if (KeyManager::CheckOnHitKey(KEY_INPUT_V))
		ms.presentProgram = !ms.presentProgram;
	if (KeyManager::CheckOnHitKey(KEY_INPUT_I))
	{
		std::list<MIDIMetaStructure> metalist;
		if (pmp->GetMIDIMeta(metalist))
		{
			std::wstring metainfo;
			const TCHAR metatype[7][4] = { L"文　本",L"版　权",L"序列名",L"乐器名",L"歌　词",L"标　记",L"ＣＵＥ" };
			while (metalist.size())
			{
				const MIDIMetaStructure &ms = metalist.front();
				if (ms.midiMetaEventType > 0 && ms.midiMetaEventType <= 7)
				{
					WCHAR wcharbuf[256] = L"";
					if (metainfo.size())
						metainfo.append(L"\n");
					metainfo.append(L"[");
					metainfo.append(metatype[ms.midiMetaEventType - 1]);
					metainfo.append(L"]");
					MultiByteToWideChar(CP_ACP, NULL, (char*)ms.pData, ms.dataLength, wcharbuf, ARRAYSIZE(wcharbuf) - 1);
					metainfo.append(wcharbuf);
				}
				metalist.pop_front();
			}
			ShellMessageBox(NULL, hWindowDx, metainfo.c_str(), L"MIDI数据", MB_ICONINFORMATION);
		}
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_LEFT))OnSeekBar(-1);
	if (KeyManager::CheckOnHitKey(KEY_INPUT_RIGHT))OnSeekBar(1);
	if (KeyManager::CheckOnHitKey(KEY_INPUT_Z))
		pmp->SetPlaybackSpeed(pmp->GetPlaybackSpeed() * 2.0f);
	if (KeyManager::CheckOnHitKey(KEY_INPUT_X))
		pmp->SetPlaybackSpeed(1.0f);
	if (KeyManager::CheckOnHitKey(KEY_INPUT_C))
		pmp->SetPlaybackSpeed(pmp->GetPlaybackSpeed() / 2.0f);
	for (int i = KEY_INPUT_1; i <= KEY_INPUT_0; i++)
	{
		if (KeyManager::CheckOnHitKey(i))
		{
			unsigned ch = i - KEY_INPUT_1;
			if ((CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT)) && ch + 10 < 16)
				ch += 10;
			pmp->SetChannelEnabled(ch, !pmp->GetChannelEnabled(ch));
		}
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
		snprintfDx(szappend, ARRAYSIZE(szappend), TEXT("%c%d,%d%c"), loopIncludeStart ? '[' : '(',
			posLoopStart, posLoopEnd, loopIncludeEnd ? ']' : ')');
		strcatDx(str, szappend);
	}
}

void VMPlayer::OnSeekBar(int dbars)
{
	pmp->SetPos(pmp->GetPosTick() + dbars*pmp->GetStepsPerBar()*pmp->GetQuarterNoteTicks());
	pmp->Panic(true);
}

#ifdef _UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	VMPlayer vmp;
	if (vmp.Init(lpCmdLine) == 0)
		vmp.Run();
	return vmp.End();
}