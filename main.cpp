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
	TCHAR szTimeInfo[100];
	TCHAR szLastTick[12] = TEXT("1:01:000");
	bool helpOpened = false;
	int showProgram = 0;//0=不显示，1=显示音色号，2=显示音色名，3=全部显示
	int drawHelpLabelX, drawHelpLabelY, drawHelpX, drawHelpY;
	int drawProgramX, drawProgramY, drawProgramOneChannelH;

	int stepsperbar = 4;
};

const TCHAR helpLabel[] = TEXT("F1:帮助");
const TCHAR helpInfo[] = TEXT("【界面未标示的其他功能】\n\nZ: 加速 X: 恢复原速 C: 减速\nV: 用不同的颜色表示音色\nI: 显示MIDI数据\n"
	"R: 显示音色\nF11: 切换全屏显示\n1,2...9,0: 静音/取消静音第1,2...9,10通道\n"
	"Shift+1,2...6: 静音/取消静音第11,12...16通道\nShift+Space：播放/暂停（无丢失）\n←/→: 定位\n\n按F1关闭帮助。");
TCHAR programName[][30] = {
L"Acoustic Grand Piano",
L"Bright Acoustic Piano",
L"Electric Grand Piano",
L"Honky-tonk Piano",
L"Electric Piano 1",
L"Electric Piano 2",
L"Harpsichord",
L"Clavi",
L"Celesta",
L"Glockenspiel",
L"Music Box",
L"Vibraphone",
L"Marimba",
L"Xylophone",
L"Tubular Bells",
L"Dulcimer",
L"Drawbar Organ",
L"Percussive Organ",
L"Rock Organ",
L"Church Organ",
L"Reed Organ",
L"Accordion",
L"Harmonica",
L"Tango Accordion",
L"Acoustic Guitar (nylon)",
L"Acoustic Guitar (steel)",
L"Electric Guitar (jazz)",
L"Electric Guitar (clean)",
L"Electric Guitar (muted)",
L"Overdriven Guitar",
L"Distortion Guitar",
L"Guitar harmonics",
L"Acoustic Bass",
L"Electric Bass (finger)",
L"Electric Bass (pick)",
L"Fretless Bass",
L"Slap Bass 1",
L"Slap Bass 2",
L"Synth Bass 1",
L"Synth Bass 2",
L"Violin",
L"Viola",
L"Cello",
L"Contrabass",
L"Tremolo Strings",
L"Pizzicato Strings",
L"Orchestral Harp",
L"Timpani",
L"String Ensemble 1",
L"String Ensemble 2",
L"SynthStrings 1",
L"SynthStrings 2",
L"Choir Aahs",
L"Voice Oohs",
L"Synth Voice",
L"Orchestra Hit",
L"Trumpet",
L"Trombone",
L"Tuba",
L"Muted Trumpet",
L"French Horn",
L"Brass Section",
L"SynthBrass 1",
L"SynthBrass 2",
L"Soprano Sax",
L"Alto Sax",
L"Tenor Sax",
L"Baritone Sax",
L"Oboe",
L"English Horn",
L"Bassoon",
L"Clarinet",
L"Piccolo",
L"Flute",
L"Recorder",
L"Pan Flute",
L"Blown Bottle",
L"Shakuhachi",
L"Whistle",
L"Ocarina",
L"Lead 1 (square)",
L"Lead 2 (sawtooth)",
L"Lead 3 (calliope)",
L"Lead 4 (chiff)",
L"Lead 5 (charang)",
L"Lead 6 (voice)",
L"Lead 7 (fifths)",
L"Lead 8 (bass + lead)",
L"Pad 1 (new age)",
L"Pad 2 (warm)",
L"Pad 3 (polysynth)",
L"Pad 4 (choir)",
L"Pad 5 (bowed)",
L"Pad 6 (metallic)",
L"Pad 7 (halo)",
L"Pad 8 (sweep)",
L"FX 1 (rain)",
L"FX 2 (soundtrack)",
L"FX 3 (crystal)",
L"FX 4 (atmosphere)",
L"FX 5 (brightness)",
L"FX 6 (goblins)",
L"FX 7 (echoes)",
L"FX 8 (sci-fi)",
L"Sitar",
L"Banjo",
L"Shamisen",
L"Koto",
L"Kalimba",
L"Bag pipe",
L"Fiddle",
L"Shanai",
L"Tinkle Bell",
L"Agogo",
L"Steel Drums",
L"Woodblock",
L"Taiko Drum",
L"Melodic Tom",
L"Synth Drum",
L"Reverse Cymbal",
L"Guitar Fret Noise",
L"Breath Noise",
L"Seashore",
L"Bird Tweet",
L"Telephone Ring",
L"Helicopter",
L"Applause",
L"Gunshot"
};
TCHAR drumName[][30] = {
L"0 Standard",
L"1 Standard 2",
L"2 Standard L/R",
L"8 Room",
L"9 Hip Hop",
L"10 Jungle",
L"11 Techno",
L"12 Room L/R",
L"13 House",
L"16 Power",
L"24 Electronic",
L"25 TR-808",
L"26 Dance",
L"27 CR-78",
L"28 TR-606",
L"29 TR-707",
L"30 TR-909",
L"32 Jazz",
L"33 Jazz L/R",
L"40 Brush",
L"41 Brush 2",
L"42 Brush 2 L/R",
L"48 Orchestra",
L"49 Ethnic",
L"50 Kick & Snare",
L"51 Kick & Snare 2",
L"52 Asia",
L"53 Cymbal & Claps",
L"54 Gamelan",
L"55 Gamelan 2",
L"56 SFX",
L"57 Rhythm FX",
L"58 Rhythm FX 2",
L"59 Rhythm FX 3",
L"60 SFX 2",
L"61 Voice",
L"62 Cymbal & Claps 2",
L"127 CM-64/32"
};
#include<vector>
#include<string>
std::vector<std::wstring>mapDrumName;

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
	SetWindowText(TEXT("Visual MIDI Player"));
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
	pmp->SetOnFinishPlay([](void*) {VMPlayer::_pObj->_OnFinishPlayCallback(); }, nullptr);
	pmp->SetOnProgramChange([](int ch, int prog) {VMPlayer::_pObj->_OnProgramChangeCallback(ch, prog); });
	pmp->SetSendLongMsg(sendlong = true);
	ms.SetPlayerSrc(pmp);
	drawProgramX = 4;
	drawProgramY = GetFontSize() + 4;
	drawProgramOneChannelH = posYLowerText - (GetFontSize() + 4);
	ms.SetRectangle(drawProgramX, drawProgramY, hdpi.X(w) - 8, drawProgramOneChannelH);
	drawProgramOneChannelH /= 16;
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

	int lineCount;
	GetDrawStringSize(&drawHelpLabelX, &drawHelpLabelY, &lineCount, helpLabel, (int)strlenDx(helpLabel));
	drawHelpLabelX = screenWidth - drawHelpLabelX;
	drawHelpLabelY = 0;
	GetDrawStringSize(&drawHelpX, &drawHelpY, &lineCount, helpInfo, (int)strlenDx(helpInfo));
	drawHelpX = (screenWidth - drawHelpX) / 2;
	drawHelpY = (screenHeight - drawHelpY) / 2;

	for (int i = 0; i < ARRAYSIZE(programName); i++)
	{
		TCHAR tmp[30];
		wsprintf(tmp, TEXT("%3d %s"), i, programName[i]);
		lstrcpy(programName[i], tmp);
	}
	for (int i = 0; i < ARRAYSIZE(drumName); i++)
	{
		TCHAR buf[24],buf2[30];
		unsigned n;
		sscanfDx(drumName[i], TEXT("%d %[^\n]"), &n, buf);
		wsprintf(buf2, TEXT("%3d %s"), n, buf);
		while (mapDrumName.size() < n)
		{
			TCHAR buf3[4];
			wsprintf(buf3, TEXT("%3d"), mapDrumName.size());
			mapDrumName.push_back(buf3);
		}
		mapDrumName.push_back(buf2);
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
	pmp->GetPlayStatus() ? pmp->Pause() : pmp->Play(true, !CheckHitKey(KEY_INPUT_LSHIFT) && !CheckHitKey(KEY_INPUT_RSHIFT));
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
	swprintf_s(szTimeInfo, TEXT("BPM:%7.3f 时间：%d:%02d.%03d Tick:%3d:%d:%03d/%s 事件:%6d/%d 复音:%3d 丢失：%d"), pmp->GetBPM(),
		minute, second, millisec, bar, step, tick, szLastTick, pmp->GetPosEventNum(), pmp->GetEventCount(),
		pmp->GetPolyphone(),pmp->GetDrop());
	DrawString(0, 0, szTimeInfo, 0x00FFFFFF);
}

void VMPlayer::OnDraw()
{
	ClearDrawScreen();
	ms.Draw();
	DrawTime();
	DrawString(0, posYLowerText, szStr, 0x00FFFFFF);
	if (showProgram)
	{
		for (int i = 0; i < 16; i++)
		{
			LPCTSTR p;
			if (i == 9)
				p = mapDrumName[ms.chPrograms[i]].c_str();
			else
				p = programName[ms.chPrograms[i]];
			switch (showProgram)
			{
			case 1:
				DrawNString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, p, 3, 0x00FFFFFF);
				break;
			case 2:
				DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, p + 4, 0x00FFFFFF);
				break;
			case 3:
				DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, p, 0x00FFFFFF);
				break;
			}
		}
	}
	if (helpOpened)
		DrawString(drawHelpX, drawHelpY, helpInfo, 0x00FFFFFF);
	else
		DrawString(drawHelpLabelX, drawHelpLabelY, helpLabel, 0x00FFFFFF);
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
	if (KeyManager::CheckOnHitKey(KEY_INPUT_F1))
		helpOpened = !helpOpened;
	if (KeyManager::CheckOnHitKey(KEY_INPUT_R))
		showProgram = (showProgram + 1) % 4;
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