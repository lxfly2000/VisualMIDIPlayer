#include<DxLib.h>
#include<MidiPlayer.h>
#include"MIDIScreen.h"
#include"CommonDialogs.h"
#include"DxShell.h"
#include"ResLoader.h"
#include"resource1.h"
#include"vstplugin.h"
#include"MidiController/MidiController.h"

#pragma comment(lib,"ComCtl32.lib")

#define VMP_TEMP_FILENAME "vmp_temp.mid"
#define CHOOSE_DEVICE_USER_CLICKED_CANCEL (UINT)-4
#define FILTER_VST "VST插件\0*.dll\0所有文件\0*\0\0"
#define FILTER_SOUNDFONT "SF2音色库\0*.sf2\0所有文件\0*\0\0"

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
	int InitMIDIPlayer(UINT deviceId, LPVOID extraInfo);
	int InitMIDIInput(UINT deviceId);
	int EndMIDIPlayer();
	int EndMIDIInput();
	MidiController* pmc = nullptr;
	MidiPlayer* pmp = nullptr;
protected:
	void _OnFinishPlayCallback();
	void _OnProgramChangeCallback(int channel, int program);
	void _OnControlChangeCallback(int channel, int cc, int val);
	void _OnSysExCallback(PBYTE data,size_t length);
	void _OnLyricText(const MIDIMetaStructure& meta);
	bool OnLoadMIDI(TCHAR*);
	void OnCommandPlay();
	void OnCommandStop();
	void OnDrop(HDROP);
	void OnSeekBar(int);
	static VMPlayer* _pObj;
private:
	static LRESULT CALLBACK ExtraProcess(HWND, UINT, WPARAM, LPARAM);
	static WNDPROC dxProcess;
	HWND hWindowDx;
	//当返回CHOOSE_DEVICE_USER_CLICKED_CANCEL时为取消操作，其他为选择的设备ID
	unsigned ChooseDevice(LPTSTR path, bool useDefaultIfOnlyOne);
	//当返回CHOOSE_DEVICE_USER_CLICKED_CANCEL时为取消操作，-1为不使用，其他为选择的设备ID
	unsigned ChooseInputDevice();
	void OnDraw();
	void OnLoop();
	bool LoadMIDI(const TCHAR* path);
	void DrawTime();
	void UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path, bool clearLyrics = false);
	void UpdateTextLastTick();
	void ReChooseMIDIDevice();
	void ReChooseMIDIInDevice();
	MIDIScreen ms;
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
	TCHAR filepath[MAX_PATH];
	TCHAR szStr[156];
	TCHAR szTimeInfo[100];
	TCHAR szLastTick[12];
	int showProgram = 0;//0=不显示，1=显示3列音色号，2=显示音色名，3=显示1列音色号和音色名，4=全部显示
	int drawHelpLabelX, drawHelpLabelY;
	int drawProgramX, drawProgramY, drawProgramOneChannelH;
	bool isNonDropPlay = false;
	bool fileLoadOK = true;
	UINT midiDeviceID = 0,midiInDeviceID=-1;
	int displayWinWidth;
	TCHAR szLyric[128];

	int stepsperbar = 4;
};

const TCHAR helpLabel[] = TEXT("F1:帮助");
const TCHAR helpInfo[] = TEXT("【界面未标示的其他功能】\n\nZ: 加速 X: 恢复原速 C: 减速\nV: 用不同的颜色表示音色\nI: 显示MIDI数据\n")
	TEXT("R: 显示音色\nD: 重新选择MIDI输出设备\nN: 重新选择MIDI输入设备\nW: 显示/隐藏VST窗口\nF11: 切换全屏显示\n1,2...9,0: 静音/取消静音第1,2...9,10通道\n")
	TEXT("Shift+1,2...6: 静音/取消静音第11,12...16通道\nShift+Space：播放/暂停（无丢失）\n←/→: 定位\n\n")
	TEXT("屏幕左侧三栏数字分别表示：\n音色，CC0，CC32\n\n")
	TEXT("屏幕钢琴框架颜色表示的MIDI模式：\n蓝色:GM 橘黄色:GS 绿色:XG 银灰色:GM2\n\n")
	TEXT("制作：lxfly2000\nhttps://github.com/lxfly2000/VisualMIDIPlayer");
#include"Instruments.h"
#include<vector>
#include<string>
std::vector<std::wstring>mapProgramName;
std::vector<std::wstring>mapDrumName;

VMPlayer* VMPlayer::_pObj = nullptr;
WNDPROC VMPlayer::dxProcess = nullptr;

unsigned VMPlayer::ChooseDevice(LPTSTR extraInfoPath,bool useDefaultIfOnlyOne)
{
	unsigned nMidiDev = midiOutGetNumDevs();
	UINT cur = 0;
	MIDIOUTCAPS caps;
	if (nMidiDev > 1||!useDefaultIfOnlyOne)
	{
		std::vector<const TCHAR*>midiList;
		std::vector<std::basic_string<TCHAR>> midiListVector;
		for (int i = -1; i < (int)nMidiDev; i++)
		{
			midiOutGetDevCaps((UINT_PTR)i, &caps, sizeof(caps));
			midiListVector.push_back(caps.szPname);
		}
		for (UINT i = 0; i < nMidiDev+1; i++)
			midiList.push_back(midiListVector[i].c_str());
		midiList.push_back(TEXT("使用VST插件……"));
		midiList.push_back(TEXT("使用SF2音色库……"));
		tagChooseDeviceDialog:
		cur = (UINT)CMDLG_ChooseList(hWindowDx, TEXT("选择 MIDI 输出设备"), midiList.data(), (int)midiList.size(), midiDeviceID + 1);
		if (cur == (UINT)-1)
			cur = CHOOSE_DEVICE_USER_CLICKED_CANCEL;
		else if (cur == nMidiDev + 1)
		{
			if (CMDLG_ChooseFile(hWindowDx,extraInfoPath, extraInfoPath, TEXT(FILTER_VST)) == FALSE)
				goto tagChooseDeviceDialog;
			cur = MIDI_DEVICE_USE_VST_PLUGIN;
		}
		else if (cur == nMidiDev + 2)
		{
			if (CMDLG_ChooseFile(hWindowDx,extraInfoPath, extraInfoPath, TEXT(FILTER_SOUNDFONT)) == FALSE)
				goto tagChooseDeviceDialog;
			cur = MIDI_DEVICE_USE_SOUNDFONT2;
		}
		else
			cur--;
	}
	if (cur != CHOOSE_DEVICE_USER_CLICKED_CANCEL)
		midiDeviceID = cur;
	return cur;
}

unsigned VMPlayer::ChooseInputDevice()
{
	unsigned nMidiDev = midiInGetNumDevs();
	UINT cur = 0;
	MIDIINCAPS caps;
	if (nMidiDev > 0)
	{
		std::vector<const TCHAR*>midiList;
		std::vector<std::basic_string<TCHAR>> midiListVector;
		for (UINT i = 0; i < nMidiDev; i++)
		{
			midiInGetDevCaps(i, &caps, sizeof(caps));
			midiListVector.push_back(caps.szPname);
		}
		for (UINT i = 0; i < nMidiDev; i++)
			midiList.push_back(midiListVector[i].c_str());
		midiList.insert(midiList.begin(), TEXT("[-] 不使用"));
		cur = (UINT)CMDLG_ChooseList(hWindowDx, TEXT("选择 MIDI 输入设备"), midiList.data(), (int)midiList.size(), midiInDeviceID + 1);
		if (cur == (UINT)-1)
			cur = CHOOSE_DEVICE_USER_CLICKED_CANCEL;
		else
			cur--;
	}
	if (cur != CHOOSE_DEVICE_USER_CLICKED_CANCEL)
		midiInDeviceID = cur;
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
	strcpyDx(filepath, TEXT(""));
	strcpyDx(szLastTick, TEXT("0:0:000"));
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
	CMDLG_SetUseDxDialogs(!windowed);
	SetAlwaysRunFlag(TRUE);
	DPIInfo hdpi;
	bool useHighDpi = hdpi.X(14) > 14;
	SetGraphMode(displayWinWidth = hdpi.X(w), hdpi.Y(h), 32);
	ChangeFont(TEXT("SimSun"));
	SetFontSize(hdpi.X(14));
	if (useHighDpi)
		ChangeFontType(DX_FONTTYPE_ANTIALIASING);
	CMDLG_DxShellSetUseHighDpi(useHighDpi);
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

	while (true)
	{
		TCHAR pluginExtraPath[MAX_PATH] = TEXT("");
		UINT retChoose = strlenDx(param) ? MIDI_MAPPER : ChooseDevice(pluginExtraPath, true);
		if (retChoose == CHOOSE_DEVICE_USER_CLICKED_CANCEL)
			return CHOOSE_DEVICE_USER_CLICKED_CANCEL;
		if (InitMIDIPlayer(retChoose, pluginExtraPath) == 0)
			break;
	}

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

int VMPlayer::InitMIDIPlayer(UINT deviceId, LPVOID extraInfo)
{
	pmp = new MidiPlayer(deviceId, extraInfo);
	if(pmp->GetInitResult())
	{
		switch (pmp->GetDeviceID())
		{
		case MIDI_DEVICE_USE_VST_PLUGIN:
#ifdef _M_X64
			CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查是否是有效的VST插件。\n\n* 本程序仅支持64位的VST插件，不支持VST3插件。"), NULL, MB_ICONEXCLAMATION);
#elif defined(_M_ARM)
			CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查是否是有效的VST插件。\n\n* 本程序仅支持ARM的VST插件，不支持VST3插件。"), NULL, MB_ICONEXCLAMATION);
#elif defined(_M_ARM64)
			CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查是否是有效的VST插件。\n\n* 本程序仅支持ARM64的VST插件，不支持VST3插件。"), NULL, MB_ICONEXCLAMATION);
#elif defined(_M_IX86)
			CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查是否是有效的VST插件。\n\n* 本程序仅支持32位的VST插件，不支持VST3插件。"), NULL, MB_ICONEXCLAMATION);
#else
			CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查是否是有效的VST插件。\n\n* 本程序不支持VST3插件。"), NULL, MB_ICONEXCLAMATION);
#endif
			break;
		case MIDI_DEVICE_USE_SOUNDFONT2:default:
			//TODO
			//CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查文件是否是有效的音色库文件。"), NULL, MB_ICONEXCLAMATION);
			CMDLG_MessageBox(hWindowDx, TEXT("SF2功能尚在制作中。"), NULL, MB_ICONEXCLAMATION);
			break;
		}
		delete pmp;
		pmp = nullptr;
		return -1;
	}
	pmp->SetOnFinishPlay([](void*) {VMPlayer::_pObj->_OnFinishPlayCallback(); }, nullptr);
	pmp->SetOnProgramChange([](BYTE ch, BYTE prog) {VMPlayer::_pObj->_OnProgramChangeCallback(ch, prog); });
	pmp->SetOnControlChange([](BYTE ch, BYTE cc, BYTE val) {VMPlayer::_pObj->_OnControlChangeCallback(ch, cc, val); });
	pmp->SetOnSysEx([](PBYTE data, size_t length) {VMPlayer::_pObj->_OnSysExCallback(data, length); });
	pmp->SetOnLyricText([](const MIDIMetaStructure& meta) {VMPlayer::_pObj->_OnLyricText(meta); });
	pmp->SetSendLongMsg(sendlong = true);
	ms.SetPlayerSrc(pmp);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath, true);
	return 0;
}

int VMPlayer::InitMIDIInput(UINT deviceId)
{
	pmc = MidiController::CreateMidiController(deviceId, true);
	pmc->SetOnClose([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->EndMIDIInput(); });
	pmc->SetOnData([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->pmp->_ProcessMidiShortEvent(midiMessage, false); });
	pmc->SetOnMoreData([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->pmp->_ProcessMidiShortEvent(midiMessage, false); });
	if (pmp)
		midiConnect((HMIDI)pmc->GetHandle(), pmp->GetHandle(), NULL);
	return 0;
}

int VMPlayer::End()
{
	//DxLib_End();//Win7系统会莫名崩溃
	EndMIDIInput();
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

int VMPlayer::EndMIDIInput()
{
	if (pmc)
	{
		if (pmp)
			midiDisconnect((HMIDI)pmc->GetHandle(), pmp->GetHandle(), NULL);
		if (MidiController::ReleaseMidiController(pmc) == MMSYSERR_NOERROR)
			pmc = nullptr;
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
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath, true);
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

void VMPlayer::_OnLyricText(const MIDIMetaStructure& meta)
{
	szLyric[MultiByteToWideChar(CP_ACP, NULL, (char*)meta.pData, meta.dataLength, szLyric, ARRAYSIZE(szLyric) - 1)] = 0;
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath);
}

bool VMPlayer::OnLoadMIDI(TCHAR* path)
{
	fileLoadOK = true;
	if (!LoadMIDI(path))fileLoadOK = false;
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, path, true);
	return fileLoadOK;
}

void VMPlayer::OnCommandPlay()
{
	isNonDropPlay = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);
	pmp->GetPlayStatus() ? pmp->Pause() : pmp->Play(true, !isNonDropPlay);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath, true);
}

void VMPlayer::OnCommandStop()
{
	pmp->Stop(false);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath, true);
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
	while (true)
	{
		TCHAR pluginExtraPath[MAX_PATH] = TEXT("");
		UINT id = ChooseDevice(pluginExtraPath, false);
		if (id == CHOOSE_DEVICE_USER_CLICKED_CANCEL)
		{
			if (pmp)
				return;
			id = MIDI_MAPPER;
		}
		EndMIDIPlayer();
		if (InitMIDIPlayer(id, pluginExtraPath) == 0)
		{
			if (filepath[0])
				OnLoadMIDI(filepath);
			break;
		}
	}
}

void VMPlayer::ReChooseMIDIInDevice()
{
	if (midiInGetNumDevs() == 0)
	{
		CMDLG_MessageBox(hWindowDx, TEXT("没有MIDI输入设备可以选择。"), NULL, MB_ICONEXCLAMATION);
		return;
	}
	UINT id = ChooseInputDevice();
	if ((int)id >= 0)
	{
		EndMIDIInput();
		InitMIDIInput(id);
	}
	else if ((int)id == -1)
		EndMIDIInput();
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
		CMDLG_SetUseDxDialogs(!windowed);
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_SPACE))
		OnCommandPlay();
	if (KeyManager::CheckOnHitKey(KEY_INPUT_S))
		OnCommandStop();
	if (KeyManager::CheckOnHitKey(KEY_INPUT_O))
	{
		if (CMDLG_ChooseFile(hWindowDx,filepath, filepath, TEXT("MIDI 序列\0*.mid;*.rmi\0所有文件\0*\0\0")))
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
			CMDLG_MessageBox(hWindowDx, metainfo.c_str(), TEXT("MIDI数据"), MB_ICONINFORMATION, true);
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
	if (KeyManager::CheckOnHitKey(KEY_INPUT_N))
		ReChooseMIDIInDevice();
	if (KeyManager::CheckOnHitKey(KEY_INPUT_W))
	{
		if (pmp->GetDeviceID() == MIDI_DEVICE_USE_VST_PLUGIN)
			((VstPlugin*)pmp->GetPlugin())->ShowPluginWindow(!((VstPlugin*)pmp->GetPlugin())->IsPluginWindowShown());
	}
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
		CMDLG_InfoBox(hWindowDx, helpInfo, TEXT("帮助"));
	if (KeyManager::CheckOnHitKey(KEY_INPUT_R))
		showProgram = (showProgram + 1) % 5;
}

void VMPlayer::UpdateString(TCHAR *str, int strsize, bool isplaying, const TCHAR *path, bool clearLyrics)
{
	TCHAR displayPath[MAX_PATH];
	const TCHAR* _strPlayStat = isplaying ? LoadLocalString(IDS_STATUS_PLAYING) : LoadLocalString(IDS_STATUS_IDLE);
	TCHAR strPlayStat[10];
	strcpyDx(strPlayStat, _strPlayStat);
	int mw = displayWinWidth - GetDrawStringWidth(strPlayStat, (int)strlenDx(strPlayStat));
	TCHAR szAppend[20];
	if (!fileLoadOK)
		mw -= GetDrawStringWidth(LoadLocalString(IDS_INVALID_FILE), 6);
	else if (posLoopEnd)
	{
		snprintfDx(szAppend, ARRAYSIZE(szAppend), TEXT("%c%d,%d%c"), loopIncludeStart ? '[' : '(',
			posLoopStart, posLoopEnd, loopIncludeEnd ? ']' : ')');
		mw -= GetDrawStringWidth(szAppend, (int)strlenDx(szAppend));
	}
	if (clearLyrics)
		szLyric[0] = 0;
	TCHAR strOn[3], strOff[3], strNoLoad[16];
	strcpyDx(strOn, LoadLocalString(IDS_ON));
	strcpyDx(strOff, LoadLocalString(IDS_OFF));
	strcpyDx(strNoLoad, LoadLocalString(IDS_NO_OPEN_FILE));
	snprintfDx(str, strsize, TEXT("Space:播放/暂停 S:停止 O:打开文件 Esc:退出 L:循环[%s] P:力度[%s] E:发送长消息[%s] ↑/↓:音量[%3d%%]\n%s%s"),
		loopOn ? strOn : strOff, pressureOn ? strOn : strOff, sendlong ? strOn : strOff,
		volume, strPlayStat, szLyric[0] ? szLyric : (path[0] ? ShortenPath(path, FALSE, displayPath, GetDefaultFontHandle(), mw) : strNoLoad));
	if (!fileLoadOK)
		strcatDx(str, LoadLocalString(IDS_INVALID_FILE));
	else if (posLoopEnd)
		strcatDx(str, szAppend);
}

void VMPlayer::OnSeekBar(int dbars)
{
	pmp->SetPos(pmp->GetPosTick() + dbars*pmp->GetStepsPerBar()*pmp->GetQuarterNoteTicks());
	pmp->Panic(true);
	UpdateString(szStr, ARRAYSIZE(szStr), pmp->GetPlayStatus() == TRUE, filepath, true);
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