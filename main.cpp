#include<DxLib.h>
#include<regex>
#include<MidiPlayer.h>
#include"MIDIScreen.h"
#include"DxShell.h"
#include"ResLoader.h"
#include"resource1.h"
#include"ChooseList.h"

#pragma comment(lib,"ComCtl32.lib")

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
	int InitMIDIPlayer(UINT deviceId);
	int EndMIDIPlayer();
protected:
	void _OnFinishPlayCallback();
	void _OnProgramChangeCallback(int channel, int program);
	void _OnControlChangeCallback(int channel, int cc, int val);
	void _OnSysExCallback(PBYTE data,size_t length);
	bool OnLoadMIDI(TCHAR*);
	void OnCommandPlay();
	void OnDrop(HDROP);
	void OnSeekBar(int);
	static VMPlayer* _pObj;
private:
	static LRESULT CALLBACK ExtraProcess(HWND, UINT, WPARAM, LPARAM);
	static WNDPROC dxProcess;
	HWND hWindowDx;
	//当返回-2时为取消操作，其他为选择的设备ID
	unsigned ChooseDevice();
	void OnDraw();
	void OnLoop();
	bool LoadMIDI(const TCHAR* path);
	void DrawTime();
	void UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path);
	void UpdateTextLastTick();
	void ReChooseMIDIDevice();
	MIDIScreen ms;
	MidiPlayer* pmp = nullptr;
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
	TCHAR szLastTick[12] = TEXT("0:0:000");
	int showProgram = 0;//0=不显示，1=显示3列音色号，2=显示音色名，3=显示1列音色号和音色名，4=全部显示
	int drawHelpLabelX, drawHelpLabelY;
	int drawProgramX, drawProgramY, drawProgramOneChannelH;
	bool isNonDropPlay = false;
	bool fileLoadOK = false;
	int midiDeviceID = 0;

	int stepsperbar = 4;
};

const TCHAR helpLabel[] = TEXT("F1:帮助");
const TCHAR helpInfo[] = TEXT("【界面未标示的其他功能】\n\nZ: 加速 X: 恢复原速 C: 减速\nV: 用不同的颜色表示音色\nI: 显示MIDI数据\n"
	"R: 显示音色\nD: 重新选择MIDI设备\nF11: 切换全屏显示\n1,2...9,0: 静音/取消静音第1,2...9,10通道\n"
	"Shift+1,2...6: 静音/取消静音第11,12...16通道\nShift+Space：播放/暂停（无丢失）\n←/→: 定位\n\n"
	"屏幕左侧三栏数字分别表示：\n音色，CC0，CC32\n\n"
	"屏幕钢琴框架颜色表示的MIDI模式：\n蓝色:GM 橘黄色:GS 绿色:XG 银灰色:GM2\n\n"
	"制作：lxfly2000\nhttps://github.com/lxfly2000/VisualMIDIPlayer"
);
#include"Instruments.h"
#include<vector>
#include<string>
std::vector<std::wstring>mapProgramName;
std::vector<std::wstring>mapDrumName;

VMPlayer* VMPlayer::_pObj = nullptr;
WNDPROC VMPlayer::dxProcess = nullptr;

unsigned VMPlayer::ChooseDevice()
{
	unsigned nMidiDev = midiOutGetNumDevs(), cur = 0;
	TCHAR str[120];
	MIDIOUTCAPS caps;
	if (nMidiDev > 1)
	{
		std::vector<const TCHAR*>midiList;
		std::vector<std::basic_string<TCHAR>> midiListVector;
		for (UINT i = 0; i < nMidiDev; i++)
		{
			midiOutGetDevCaps(i, &caps, sizeof(caps));
			sprintfDx(str, TEXT("[%d] %s"), i, caps.szPname);
			midiListVector.push_back(str);
		}
		for (UINT i = 0; i < nMidiDev; i++)
			midiList.push_back(midiListVector[i].c_str());
		if (windowed)
			cur = (UINT)ChooseList(hWindowDx, TEXT("选择 MIDI 输出设备"), midiList.data(), (int)midiList.size(), midiDeviceID, NULL, NULL);
		else
			cur = (UINT)DxChooseListItem(TEXT("选择 MIDI 输出设备\n[Enter]确认 [Esc]退出 [↑/↓]选择"), midiList.data(), (int)midiList.size(),midiDeviceID);
		if (cur == (UINT)-1)
			cur = (UINT)-2;
	}
	if ((int)cur >= 0)
		midiDeviceID = (int)cur;
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
	if (strstrDx(param, TEXT("600p")))
	{
		w = 960;
		h = 600;
		param[0] = TEXT('\0');
	}
	else if (strstrDx(param, TEXT("720p")))
	{
		w = 1280;
		h = 720;
		param[0] = TEXT('\0');
	}
	SetOutApplicationLogValidFlag(FALSE);
	SetWindowText(TEXT("Visual MIDI Player"));
	windowed = strstrDx(param, TEXT("/f")) ? FALSE : TRUE;
	if (!windowed)
		param[0] = 0;
	ChangeWindowMode(windowed);
	SetAlwaysRunFlag(TRUE);
	DPIInfo hdpi;
	bool useHighDpi = hdpi.X(14) > 14;
	SetGraphMode(hdpi.X(w), hdpi.Y(h), 32);
	ChangeFont(TEXT("SimSun"));
	SetFontSize(hdpi.X(14));
	if (useHighDpi)
		ChangeFontType(DX_FONTTYPE_ANTIALIASING);
	DxShellSetUseHighDpi(useHighDpi);
	SetFontThickness(3);
	SetWindowSizeExtendRate(1.0, 1.0);
	GetDrawScreenSize(&screenWidth, &screenHeight);
	if (DxLib_Init() != 0)return -1;
	SetDrawScreen(DX_SCREEN_BACK);

	posYLowerText = screenHeight - (GetFontSize() + 4) * 2;

	drawProgramX = 4;
	drawProgramY = GetFontSize() + 4;
	drawProgramOneChannelH = posYLowerText - (GetFontSize() + 4);
	ms.SetRectangle(drawProgramX, drawProgramY, hdpi.X(w) - 8, drawProgramOneChannelH);
	drawProgramOneChannelH /= 16;

	int lineCount;
	GetDrawStringSize(&drawHelpLabelX, &drawHelpLabelY, &lineCount, helpLabel, (int)strlenDx(helpLabel));
	drawHelpLabelX = screenWidth - drawHelpLabelX;
	drawHelpLabelY = 0;

	for (int i = 0; i < ARRAYSIZE(programName); i++)
	{
		TCHAR tmp[30];
		wsprintf(tmp, TEXT("%3d %s"), i, programName[i]);
		mapProgramName.push_back(tmp);
	}
	for (int i = 0; i < ARRAYSIZE(drumName); i++)
	{
		TCHAR tmp[30];
		if (lstrlen(drumName[i]) == 0)
			wsprintf(tmp, TEXT("%3d"), i);
		else
			wsprintf(tmp, TEXT("%3d %s"), i, drumName[i]);
		mapDrumName.push_back(tmp);
	}

	//http://nut-softwaredevelopper.hatenablog.com/entry/2016/02/25/001647
	hWindowDx = GetMainWindowHandle();
	dxProcess = (WNDPROC)GetWindowLongPtr(hWindowDx, GWLP_WNDPROC);
	SetWindowLongPtr(hWindowDx, GWL_EXSTYLE, WS_EX_ACCEPTFILES | GetWindowLongPtr(hWindowDx, GWL_EXSTYLE));
	SetWindowLongPtr(hWindowDx, GWLP_WNDPROC, (LONG_PTR)ExtraProcess);

	UINT retChoose = strlenDx(param) ? MIDI_MAPPER : ChooseDevice();
	if (retChoose == (UINT)-2)
		return -2;
	InitMIDIPlayer(retChoose);

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

	return 0;
}

int VMPlayer::InitMIDIPlayer(UINT deviceId)
{
	pmp = new MidiPlayer(deviceId);
	pmp->SetOnFinishPlay([](void*) {VMPlayer::_pObj->_OnFinishPlayCallback(); }, nullptr);
	pmp->SetOnProgramChange([](BYTE ch, BYTE prog) {VMPlayer::_pObj->_OnProgramChangeCallback(ch, prog); });
	pmp->SetOnControlChange([](BYTE ch, BYTE cc, BYTE val) {VMPlayer::_pObj->_OnControlChangeCallback(ch, cc, val); });
	pmp->SetOnSysEx([](PBYTE data, size_t length) {VMPlayer::_pObj->_OnSysExCallback(data, length); });
	pmp->SetSendLongMsg(sendlong = true);
	ms.SetPlayerSrc(pmp);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
	return 0;
}

int VMPlayer::End()
{
	//DxLib_End();//Win7系统会莫名崩溃
	return EndMIDIPlayer();
}

int VMPlayer::EndMIDIPlayer()
{
	if (pmp)
	{
		pmp->Stop();
		pmp->Unload();
		delete pmp;
		pmp = nullptr;
	}
	return 0;
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

void VMPlayer::_OnControlChangeCallback(int channel, int cc, int val)
{
	switch (cc)
	{
	case 0:
		ms.chCC0[channel] = val;
		break;
	case 32:
		ms.chCC32[channel] = val;
		break;
	}
}

int memcmp_with_mask(LPCVOID a, LPCVOID b, LPCVOID mask, size_t length)
{
	LPCBYTE ba = (LPCBYTE)a;
	LPCBYTE bb = (LPCBYTE)b;
	LPCBYTE bmask = (LPCBYTE)mask;
	for (size_t i = 0; i < length; i++)
	{
		int ma = ba[i] & bmask[i];
		int mb = bb[i] & bmask[i];
		if (ma - mb)
			return ma - mb;
	}
	return 0;
}

void VMPlayer::_OnSysExCallback(PBYTE data, size_t length)
{
	static const BYTE GM1_System_On[] = { 0xF0,0x7E,0x7F,0x09,0x01,0xF7 };//切换至GM（根据VSC文档）
	static const BYTE GM2_System_On[] = { 0xF0,0x7E,0x7F,0x09,0x03,0xF7 };//切换至GM2（根据VSC文档）
	static const BYTE GM_System_Off[] = { 0xF0,0x7E,0x7F,0x09,0x02,0xF7 };//切换至GS（根据VSC文档）
	static const BYTE GS_Reset[] = { 0xF0,0x41,0x00/*0x00-0x1F*/,0x42,0x12,0x40,0x00,0x7F,0x00,0x41,0xF7 };//切换至GS（根据VSC文档）
	static const BYTE _GS_Reset_Mask[] = { 0xFF,0xFF,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
	static const BYTE GS_Exit[] = { 0xF0,0x41,0x00/*0x00-0x1F*/,0x42,0x12,0x40,0x00,0x7F,0x7F,0x42,0xF7 };//切换至GM（根据VSC文档）
	static const BYTE System_Mode_Set[] = { 0xF0,0x41,0x00/*0x00-0x1F*/,0x42,0x12,0x00,0x00,0x7F,0x00/*0x00-0xFF*/,0x00/*0x00-0xFF*/,0xF7 };//切换至GS（根据VSC文档）
	static const BYTE _System_Mode_Set_Mask[] = { 0xFF,0xFF,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xFF };
	static const BYTE XG_System_On[] = { 0xF0,0x43,0x10/*0x10-0x1F*/,0x4C,0x00,0x00,0x7E,0x00,0xF7 };//切换至XG（根据S-YXG50文档）
	static const BYTE _XG_System_On_Mask[] = { 0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
	if (memcmp(data, GM1_System_On, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(0);
	else if (memcmp(data, GM2_System_On, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(3);
	else if (memcmp(data, GM_System_Off, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(1);
	else if (memcmp_with_mask(data, GS_Reset, _GS_Reset_Mask, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(1);
	else if (memcmp_with_mask(data, GS_Exit, _GS_Reset_Mask, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(0);
	else if (memcmp_with_mask(data, System_Mode_Set, _System_Mode_Set_Mask, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(1);
	else if (memcmp_with_mask(data, XG_System_On, _XG_System_On_Mask, length) == 0)
		ms.ChangeDefaultKeyColorByMIDIMode(2);
	else
		return;
	for (int i = 0; i < 16; i++)
	{
		ms.chPrograms[i] = 0;
		ms.chCC0[i] = 0;
		ms.chCC32[i] = 0;
	}
}

bool VMPlayer::OnLoadMIDI(TCHAR* path)
{
	fileLoadOK = true;
	if (!LoadMIDI(path))fileLoadOK = false;
	if (!fileLoadOK)strcatDx(path, TEXT("（无效文件）"));
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, path);
	return fileLoadOK;
}

void VMPlayer::OnCommandPlay()
{
	isNonDropPlay = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);
	pmp->GetPlayStatus() ? pmp->Pause() : pmp->Play(true, !isNonDropPlay);
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
/*#ifdef _UNICODE
#define _T_RENAME _wrename
#else
#define _T_RENAME rename
#endif*/
		std::fstream f(path, std::ios::in | std::ios::binary);
		std::stringstream filemem;
		if (!f)
			return false;
		filemem << f.rdbuf();
		if (!pmp->LoadStream(filemem))
			return false;
		/*if (_T_RENAME(path, TEXT(VMP_TEMP_FILENAME)))return false;
		bool ok = pmp->LoadFile(VMP_TEMP_FILENAME);
		_T_RENAME(TEXT(VMP_TEMP_FILENAME), path);//特么……这样居然可以！！！（惊）
		if (!ok)return false;*/
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

void VMPlayer::ReChooseMIDIDevice()
{
	if (midiOutGetNumDevs() < 2)
	{
		if (windowed)
			MessageBox(hWindowDx, TEXT("没有其他MIDI设备可以选择。"), NULL, MB_ICONEXCLAMATION);
		else
			DxMessageBox((std::basic_string<TCHAR>(TEXT("没有其他MIDI设备可以选择。")) + LoadLocalString(IDS_DXMSG_APPEND_OK)).c_str());
		return;
	}
	UINT id = ChooseDevice();
	if ((int)id >= 0)
	{
		EndMIDIPlayer();
		InitMIDIPlayer(id);
		if (filepath[0])
			OnLoadMIDI(filepath);
	}
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
	TCHAR szDrop[12];
	if (isNonDropPlay)
		strcpyDx(szDrop, TEXT("-"));
	else
		sprintfDx(szDrop, TEXT("%d"), pmp->GetDrop());
	swprintf_s(szTimeInfo, TEXT("BPM:%7.3f 时间：%d:%02d.%03d Tick:%3d:%d:%03d/%s 事件:%6d/%d 复音:%3d 丢失：%s"), pmp->GetBPM(),
		minute, second, millisec, bar, step, tick, szLastTick, pmp->GetPosEventNum(), pmp->GetEventCount(),
		pmp->GetPolyphone(),szDrop);
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
				p = mapProgramName[ms.chPrograms[i]].c_str();
			TCHAR pMode1[50];
			switch (showProgram)
			{
			case 1:
				strncpyDx(pMode1, p, 3);
				sprintfDx(pMode1 + 3, TEXT(" %3d %3d"), ms.chCC0[i] & 0xFF, ms.chCC32[1] & 0xFF);
				DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, pMode1, 0x00FFFFFF);
				break;
			case 2:
				DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, p + 4, 0x00FFFFFF);
				break;
			case 3:
				DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, p, 0x00FFFFFF);
				break;
			case 4:
				strncpyDx(pMode1, p, 3);
				sprintfDx(pMode1 + 3, TEXT(" %3d %3d"), ms.chCC0[i] & 0xFF, ms.chCC32[1] & 0xFF);
				strcatDx(pMode1, p + 3);
				DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, pMode1, 0x00FFFFFF);
				break;
			}
		}
	}
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
		if (windowed ? SelectFile(filepath, NULL) : DxChooseFilePath(filepath, filepath))
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
		if (fileLoadOK&&pmp->GetMIDIMeta(metalist))
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
			if (windowed)
				ShellMessageBox(NULL, hWindowDx, metainfo.c_str(), L"MIDI数据", MB_ICONINFORMATION);
			else
				DxMessageBox((metainfo + LoadLocalString(IDS_DXMSG_APPEND_OK)).c_str());
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
	if (KeyManager::CheckOnHitKey(KEY_INPUT_D))
		ReChooseMIDIDevice();
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
	{
		if (windowed)
		{
			//MessageBox(hWindowDx, helpInfo, TEXT("帮助"), MB_ICONINFORMATION);
			TASKDIALOGCONFIG tdc{};
			tdc.cbSize = sizeof(tdc);
			tdc.hwndParent = hWindowDx;
			tdc.hInstance = GetModuleHandle(NULL);
			tdc.dwFlags = TDF_ENABLE_HYPERLINKS;
			tdc.dwCommonButtons = TDCBF_OK_BUTTON;
			tdc.pszWindowTitle = TEXT("帮助");
			tdc.pszMainIcon = TD_INFORMATION_ICON;
			std::basic_string<TCHAR>msgWithURL = helpInfo;
			msgWithURL = std::regex_replace(msgWithURL, std::basic_regex<TCHAR>(TEXT("(https?://[A-Za-z0-9_\\-\\./]+)")), TEXT("<a href=\"$1\">$1</a>"));
			tdc.pszContent = msgWithURL.c_str();
#undef LONG_PTR
			tdc.pfCallback = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData)
			{
				switch (msg)
				{
				case TDN_HYPERLINK_CLICKED:
					ShellExecute(hwnd, TEXT("open"), (LPCWSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
					break;
				}
				return S_OK;
			};
			TaskDialogIndirect(&tdc, NULL, NULL, NULL);
		}
		else
			DxMessageBox((std::basic_string<TCHAR>(helpInfo) + LoadLocalString(IDS_DXMSG_APPEND_OK)).c_str());
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_R))
		showProgram = (showProgram + 1) % 5;
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
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	VMPlayer vmp;
	if (vmp.Init(lpCmdLine) == 0)
		vmp.Run();
	return vmp.End();
}