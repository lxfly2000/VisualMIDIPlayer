#define _CRT_SECURE_NO_WARNINGS


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

#ifdef _M_X64
#pragma comment(lib,"libMidiPlayer/XAudio2_8/Lib/winv6.3/um/x64/XAudio2.lib")
#elif defined(_M_ARM)
#pragma comment(lib,"libMidiPlayer/XAudio2_8/Lib/winv6.3/um/arm/XAudio2.lib")
#elif defined(_M_ARM64)
#error "The ARM64 architecture is not supported yet."
#elif defined(_M_IX86)
#pragma comment(lib,"libMidiPlayer/XAudio2_8/Lib/winv6.3/um/x86/XAudio2.lib")
#else
#error "XAudio2 doesn't support this platform."
#endif


#define APP_TITLE "Visual MIDI Player"
#define VMP_TEMP_FILENAME "vmp_temp.mid"
#define CHOOSE_DEVICE_USER_CLICKED_CANCEL (UINT)-2
#define FILTER_VST "VST���\0*.dll\0�����ļ�\0*\0\0"
#define FILTER_SOUNDFONT "SF2��ɫ��\0*.sf2\0�����ļ�\0*\0\0"
#define FILTER_MIDI "MIDI ����\0*.mid;*.rmi\0�����ļ�\0*\0\0"

enum CONTROL_CHANGE_NUMBER
{
	CC_BANK_SELECT_MSB=0,//Def:0
	CC_MODULATION=1,//Def:0
	CC_VOLUME=7,//Def:100
	CC_PAN=10,//Def:64
	CC_EXPRESSION=11,//Def:127
	CC_BANK_SELECT_LSB=32//Def:0
};

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
	void _OnProgramChangeCallback(BYTE channel, BYTE program);
	void _OnControlChangeCallback(BYTE channel, BYTE cc, BYTE val);
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
	//������CHOOSE_DEVICE_USER_CLICKED_CANCELʱΪȡ������������Ϊѡ����豸ID
	unsigned ChooseDevice(LPTSTR path, bool useDefaultIfOnlyOne);
	//������CHOOSE_DEVICE_USER_CLICKED_CANCELʱΪȡ��������-1Ϊ��ʹ�ã�����Ϊѡ����豸ID
	unsigned ChooseInputDevice();
	void OnDraw();
	void OnLoop();
	bool LoadMIDI(const TCHAR* path);
	void DrawTime();
	void DrawChannelInfo();
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
	//0=����ʾ��1=��ʾ3����ɫ�ţ�2=��ʾ��ɫ����3=��ʾ1����ɫ�ź���ɫ����4=��ʾ3����ɫ�ź���ɫ����
	//5=��������ʾ��ϸ��Ϣ��6=��ˮƽͼ����ʾ��ϸ��Ϣ��7=�ô�ֱͼ����ʾ��ϸ��Ϣ��
	//8=��������ʾ��ϸ��Ϣ����ɫ����9=��ˮƽͼ����ʾ��ϸ��Ϣ����ɫ����10=�ô�ֱͼ����ʾ��ϸ��Ϣ����ɫ��
	int showProgram = 0;
	int drawHelpLabelX, drawHelpLabelY;
	int drawProgramX, drawProgramY, drawProgramOneChannelH;
	bool isNonDropPlay = false;
	bool fileLoadOK = true;
	UINT midiDeviceID = 0,midiInDeviceID=-1;
	int displayWinWidth;
	TCHAR szLyric[128];
	BYTE chPrograms[16];
	BYTE chCC[16][128];

	int stepsperbar = 4;
};

const TCHAR helpLabel[] = TEXT("F1:����");
const TCHAR helpInfo[] = TEXT("������δ��ʾ���������ܡ�\n\nZ: ���� X: �ָ�ԭ�� C: ����\nV: �ò�ͬ����ɫ��ʾ��ɫ\nI: ��ʾMIDI����\n")
	TEXT("R: ��ʾ��ɫ��ͨ����Ϣ\nD: ����ѡ��MIDI����豸\nN: ����ѡ��MIDI�����豸\nW: ��ʾ/����VST����\nT: ������Ƶ(ʹ��VST����ʱ)\nF11: �л�ȫ����ʾ\n1,2...9,0: ����/ȡ��������1,2...9,10ͨ��\n")
	TEXT("Shift+1,2...6: ����/ȡ��������11,12...16ͨ��\nShift+Space������/��ͣ���޶�ʧ��\n��/��: ��λ\n\n")
	TEXT("ͨ����Ϣ�ĸ���ֵ�ֱ��ʾ��\n��ɫ��CC0��CC32������(Modulation)������(Volume)��ƽ��(Pan)������(Expression)\n\n")
	TEXT("��Ļ���ٿ����ɫ��ʾ��MIDIģʽ��\n��ɫ:GM �ٻ�ɫ:GS ��ɫ:XG ����ɫ:GM2\n\n")
	TEXT("������lxfly2000\nhttps://github.com/lxfly2000/VisualMIDIPlayer");
#include"Instruments.h"
#include<vector>
#include<string>

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
		midiListVector.push_back(TEXT("MIDI ӳ����"));
		for (int i = 0; i < (int)nMidiDev; i++)
		{
			midiOutGetDevCaps((UINT_PTR)i, &caps, sizeof(caps));
			midiListVector.push_back(caps.szPname);
		}
		for (UINT i = 0; i < nMidiDev+1; i++)
			midiList.push_back(midiListVector[i].c_str());
		midiList.push_back(TEXT("ʹ��VST�������"));
		//midiList.push_back(TEXT("ʹ��SF2��ɫ�⡭��"));
		tagChooseDeviceDialog:
		cur = (UINT)CMDLG_ChooseList(hWindowDx, TEXT("ѡ�� MIDI ����豸"), midiList.data(), (int)midiList.size(),
			(midiDeviceID == MIDI_DEVICE_USE_VST_PLUGIN || midiDeviceID == MIDI_DEVICE_USE_SOUNDFONT2) ? nMidiDev - midiDeviceID - 2 : midiDeviceID + 1);
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
		midiList.insert(midiList.begin(), TEXT("��ʹ��"));
		cur = (UINT)CMDLG_ChooseList(hWindowDx, TEXT("ѡ�� MIDI �����豸"), midiList.data(), (int)midiList.size(), midiInDeviceID + 1);
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
	ZeroMemory(chPrograms, sizeof(chPrograms));
	for (int channel = 0; channel < 16; channel++)
	{
		for (int cc = 0; cc < 128; cc++)
		{
			switch (cc)
			{
			case CC_VOLUME:
				chCC[channel][cc] = 100;
				break;
			case CC_PAN:
				chCC[channel][cc] = 64;
				break;
			case CC_EXPRESSION:
				chCC[channel][cc] = 127;
				break;
			default:
				chCC[channel][cc] = 0;
				break;
			}
		}
	}
	ms.SetProgramsPointer(chPrograms);

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
	SetWindowText(TEXT(APP_TITLE));
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
		{
			IXAudio2*x = nullptr;
			HRESULT hrx = XAudio2Create(&x);
			if (x)
				x->Release();
			if (hrx == 0x80040154)
			{
				if (MessageBox(hWindowDx, TEXT("XAudio2����ʧ�ܡ�\n����Ҫ��װ������DirectX����ʱ��ʹ�ô˹��ܡ�\n\n���谲װ������ȷ������"), NULL, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK)
				{
					TCHAR directxDir[MAX_PATH];
					GetModuleFileName(GetModuleHandle(NULL), directxDir, ARRAYSIZE(directxDir));
					*(TCHAR*)strrchrDx(directxDir, '\\') = 0;
					TCHAR exePath[MAX_PATH];
					wsprintf(exePath, TEXT("%s\\directx_Jun2010_redist.exe"), directxDir);
					HRSRC hRsrc = FindResource(NULL, TEXT("directx_Jun2010_redist.exe"), TEXT("EXE"));
					LPVOID dataPtr = LockResource(LoadResource(NULL, hRsrc));
					std::ofstream f(exePath, std::ios::binary);
					f.write((LPSTR)dataPtr, SizeofResource(NULL, hRsrc));
					f.close();
					lstrcat(directxDir, TEXT("\\DirectX"));
					SHELLEXECUTEINFO se{ sizeof(SHELLEXECUTEINFO) };
					se.hwnd = hWindowDx;
					se.lpVerb = TEXT("open");
					se.lpFile = exePath;
					TCHAR params[MAX_PATH];
					wsprintf(params, TEXT("/Q /C /T:\"%s\""), directxDir);
					se.lpParameters = params;
					se.fMask = SEE_MASK_NOCLOSEPROCESS;
					if (ShellExecuteEx(&se))
					{
						WaitForSingleObject(se.hProcess, INFINITE);
						DeleteFile(se.lpFile);
						wsprintf(params, TEXT("%s\\DXSETUP.EXE"), directxDir);
						if (GetFileAttributes(params) != INVALID_FILE_ATTRIBUTES)
						{
							se.lpFile = params;
							se.lpDirectory = directxDir;
							se.lpParameters = TEXT("/silent");
							if (ShellExecuteEx(&se))
								WaitForSingleObject(se.hProcess, INFINITE);
						}
						int len = lstrlen(directxDir);
						directxDir[len] = directxDir[len + 1] = 0;
						SHFILEOPSTRUCT sfo{ hWindowDx,FO_DELETE,directxDir,NULL,FOF_NOCONFIRMATION };
						SHFileOperation(&sfo);
					}
				}
				break;
			}
		}
#ifdef _M_X64
			CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܣ������Ƿ�����Ч��VST�����\n\n* �������֧��64λ��VST�������֧��VST3�����"), NULL, MB_ICONEXCLAMATION);
#elif defined(_M_ARM)
			CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܣ������Ƿ�����Ч��VST�����\n\n* �������֧��ARM��VST�������֧��VST3�����"), NULL, MB_ICONEXCLAMATION);
#elif defined(_M_ARM64)
			CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܣ������Ƿ�����Ч��VST�����\n\n* �������֧��ARM64��VST�������֧��VST3�����"), NULL, MB_ICONEXCLAMATION);
#elif defined(_M_IX86)
			CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܣ������Ƿ�����Ч��VST�����\n\n* �������֧��32λ��VST�������֧��VST3�����"), NULL, MB_ICONEXCLAMATION);
#else
			CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܣ������Ƿ�����Ч��VST�����\n\n* ������֧��VST3�����"), NULL, MB_ICONEXCLAMATION);
#endif
			break;
		case MIDI_DEVICE_USE_SOUNDFONT2:default:
			//CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܣ������ļ��Ƿ�����Ч��SF2��ɫ���ļ���"), NULL, MB_ICONEXCLAMATION);
			CMDLG_MessageBox(hWindowDx, TEXT("SF2������δ������"), NULL, MB_ICONEXCLAMATION);
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
	if (pmp->GetDeviceID() == MIDI_DEVICE_USE_SOUNDFONT2 || pmp->GetDeviceID() == MIDI_DEVICE_USE_VST_PLUGIN)
	{
		pmc->SetOnData([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->pmp->_ProcessMidiShortEvent(midiMessage, true); });
		pmc->SetOnMoreData([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->pmp->_ProcessMidiShortEvent(midiMessage, true); });
	}
	else
	{
		pmc->SetOnData([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->pmp->_ProcessMidiShortEvent(midiMessage, false); });
		pmc->SetOnMoreData([](DWORD_PTR timestamp_ms, DWORD_PTR midiMessage) {_pObj->pmp->_ProcessMidiShortEvent(midiMessage, false); });
		if (pmp)
			midiConnect((HMIDI)pmc->GetHandle(), pmp->GetHandle(), NULL);
	}
	return 0;
}

int VMPlayer::End()
{
	//DxLib_End();//Win7ϵͳ��Ī������
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

void VMPlayer::_OnProgramChangeCallback(BYTE channel, BYTE program)
{
	chPrograms[channel] = program;
}

void VMPlayer::_OnControlChangeCallback(BYTE channel, BYTE cc, BYTE val)
{
	chCC[channel][cc] = val;
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
	static const BYTE GM1_System_On[] = { 0xF0,0x7E,0x7F,0x09,0x01,0xF7 };//�л���GM������VSC�ĵ���
	static const BYTE GM2_System_On[] = { 0xF0,0x7E,0x7F,0x09,0x03,0xF7 };//�л���GM2������VSC�ĵ���
	static const BYTE GM_System_Off[] = { 0xF0,0x7E,0x7F,0x09,0x02,0xF7 };//�л���GS������VSC�ĵ���
	static const BYTE GS_Reset[] = { 0xF0,0x41,0x00/*0x00-0x1F*/,0x42,0x12,0x40,0x00,0x7F,0x00,0x41,0xF7 };//�л���GS������VSC�ĵ���
	static const BYTE _GS_Reset_Mask[] = { 0xFF,0xFF,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
	static const BYTE GS_Exit[] = { 0xF0,0x41,0x00/*0x00-0x1F*/,0x42,0x12,0x40,0x00,0x7F,0x7F,0x42,0xF7 };//�л���GM������VSC�ĵ���
	static const BYTE System_Mode_Set[] = { 0xF0,0x41,0x00/*0x00-0x1F*/,0x42,0x12,0x00,0x00,0x7F,0x00/*0x00-0xFF*/,0x00/*0x00-0xFF*/,0xF7 };//�л���GS������VSC�ĵ���
	static const BYTE _System_Mode_Set_Mask[] = { 0xFF,0xFF,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xFF };
	static const BYTE XG_System_On[] = { 0xF0,0x43,0x10/*0x10-0x1F*/,0x4C,0x00,0x00,0x7E,0x00,0xF7 };//�л���XG������S-YXG50�ĵ���
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
		chPrograms[i] = 0;
		chCC[i][CC_BANK_SELECT_MSB] = 0;
		chCC[i][CC_BANK_SELECT_LSB] = 0;
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
		_T_RENAME(TEXT(VMP_TEMP_FILENAME), path);//��ô����������Ȼ���ԣ�����������
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
		loopIncludeStart = la ? true : false;
		loopIncludeEnd = lb ? true : false;
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
			if (pmc)
			{
				UINT iid;
				midiInGetID(pmc->GetHandle(), &iid);
				EndMIDIInput();
				InitMIDIInput(iid);
			}
			break;
		}
	}
}

void VMPlayer::ReChooseMIDIInDevice()
{
	if (midiInGetNumDevs() == 0)
	{
		CMDLG_MessageBox(hWindowDx, TEXT("û��MIDI�����豸����ѡ��"), NULL, MB_ICONEXCLAMATION);
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
	swprintf_s(szTimeInfo, TEXT("BPM:%7.3f ʱ�䣺%d:%02d.%03d Tick:%3d:%d:%03d/%s �¼�:%6d/%d ����:%3d ��ʧ��%s"), pmp->GetBPM(),
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
		DrawChannelInfo();
	DrawString(drawHelpLabelX, drawHelpLabelY, helpLabel, 0x00FFFFFF);
	ScreenFlip();
}

static LPCTSTR GetSymbolicValueText(int v, bool center = false, bool vertical = false)
{
	static LPCTSTR symbolicValueText[] =
	{
		TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),
		TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����"),TEXT("����")
	};
	static LPCTSTR symbolicValueTextVertical[] =
	{
		TEXT("��"),TEXT("�x"),TEXT("�y"),TEXT("�z"),TEXT("�{"),TEXT("�|"),TEXT("�}"),TEXT("�~"),TEXT("��")
	};
	static LPCTSTR symbolicValueTextCenter[] =
	{
		TEXT("<<<  "),TEXT(" <<  "),TEXT("  <  "),TEXT("  |  "),TEXT("  >  "),TEXT("  >> "),TEXT("  >>>")
	};
	if (center)
		return symbolicValueTextCenter[v * ARRAYSIZE(symbolicValueTextCenter) / 128];
	else if (vertical)
		return symbolicValueTextVertical[v * ARRAYSIZE(symbolicValueTextVertical) / 128];
	else
		return symbolicValueText[v * ARRAYSIZE(symbolicValueText) / 128];
}

static LPCTSTR GetPanDigitalText(BYTE pan)
{
	static TCHAR panText[4];
	if (pan < 64)
		sprintfDx(panText, TEXT("-%2d"), 64 - pan);
	else if (pan == 64)
		strcpyDx(panText, TEXT("  0"));
	else
		sprintfDx(panText, TEXT("+%2d"), pan - 64);
	return panText;
}

void VMPlayer::DrawChannelInfo()
{
#define GetChannelEnabledText(ch) pmp->GetChannelEnabled(ch)?TEXT(""):TEXT("����")
	for (int i = 0; i < 16; i++)
	{
		TCHAR buf[56];
		switch (showProgram)
		{
		case 1:
			sprintfDx(buf, TEXT("%3d %3d %3d"), chPrograms[i], chCC[i][CC_BANK_SELECT_MSB], chCC[i][CC_BANK_SELECT_LSB]);
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 2:
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, i==9?drumName[chPrograms[i]]:programName[chPrograms[i]], 0x00FFFFFF);
			break;
		case 3:
			sprintfDx(buf, TEXT("%3d %s"), chPrograms[i], i == 9 ? drumName[chPrograms[i]] : programName[chPrograms[i]]);
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 4:
			sprintfDx(buf, TEXT("%3d %3d %3d %s"), chPrograms[i], chCC[i][CC_BANK_SELECT_MSB], chCC[i][CC_BANK_SELECT_LSB],
				i == 9 ? drumName[chPrograms[i]] : programName[chPrograms[i]]);
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 5://7*4+2==30
			sprintfDx(buf, TEXT("%3d %3d %3d %3d %3d %s %3d %s"),
				chPrograms[i],
				chCC[i][CC_BANK_SELECT_MSB],
				chCC[i][CC_BANK_SELECT_LSB],
				chCC[i][CC_MODULATION],
				chCC[i][CC_VOLUME],
				GetPanDigitalText(chCC[i][CC_PAN]),
				chCC[i][CC_EXPRESSION],
				GetChannelEnabledText(i));
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 6://3*4+2+5+2+2+2=25
			sprintfDx(buf, TEXT("%3d %3d %3d %s%s%s%s%s"),
				chPrograms[i],
				chCC[i][CC_BANK_SELECT_MSB],
				chCC[i][CC_BANK_SELECT_LSB],
				GetSymbolicValueText(chCC[i][CC_MODULATION]),
				GetSymbolicValueText(chCC[i][CC_VOLUME]),
				GetSymbolicValueText(chCC[i][CC_PAN], true),
				GetSymbolicValueText(chCC[i][CC_EXPRESSION]),
				GetChannelEnabledText(i));
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 7://3*4+1+5+2+2+2=24
			sprintfDx(buf, TEXT("%3d %3d %3d %s %s%s%s %s"),
				chPrograms[i],
				chCC[i][CC_BANK_SELECT_MSB],
				chCC[i][CC_BANK_SELECT_LSB],
				GetSymbolicValueText(chCC[i][CC_MODULATION], false, true),
				GetSymbolicValueText(chCC[i][CC_VOLUME],false,true),
				GetSymbolicValueText(chCC[i][CC_PAN], true,true),
				GetSymbolicValueText(chCC[i][CC_EXPRESSION], false, true),
				GetChannelEnabledText(i));
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 8://7*4+3+23=54
			sprintfDx(buf, TEXT("%3d %3d %3d %3d %3d %s %3d %s\n%s"),
				chPrograms[i],
				chCC[i][CC_BANK_SELECT_MSB],
				chCC[i][CC_BANK_SELECT_LSB],
				chCC[i][CC_MODULATION],
				chCC[i][CC_VOLUME],
				GetPanDigitalText(chCC[i][CC_PAN]),
				chCC[i][CC_EXPRESSION],
				GetChannelEnabledText(i),
				i == 9 ? drumName[chPrograms[i]] : programName[chPrograms[i]]);
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 9://3*4+2+5+2+2+3+23=49
			sprintfDx(buf, TEXT("%3d %3d %3d %s%s%s%s%s\n%s"),
				chPrograms[i],
				chCC[i][CC_BANK_SELECT_MSB],
				chCC[i][CC_BANK_SELECT_LSB],
				GetSymbolicValueText(chCC[i][CC_MODULATION]),
				GetSymbolicValueText(chCC[i][CC_VOLUME]),
				GetSymbolicValueText(chCC[i][CC_PAN], true),
				GetSymbolicValueText(chCC[i][CC_EXPRESSION]),
				GetChannelEnabledText(i),
				i == 9 ? drumName[chPrograms[i]] : programName[chPrograms[i]]);
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		case 10://3*4+1+5+2+2+3+23=48
			sprintfDx(buf, TEXT("%3d %3d %3d %s %s%s%s %s\n%s"),
				chPrograms[i],
				chCC[i][CC_BANK_SELECT_MSB],
				chCC[i][CC_BANK_SELECT_LSB],
				GetSymbolicValueText(chCC[i][CC_MODULATION], false, true),
				GetSymbolicValueText(chCC[i][CC_VOLUME], false, true),
				GetSymbolicValueText(chCC[i][CC_PAN], true,true),
				GetSymbolicValueText(chCC[i][CC_EXPRESSION], false, true),
				GetChannelEnabledText(i),
				i == 9 ? drumName[chPrograms[i]] : programName[chPrograms[i]]);
			DrawString(drawProgramX, drawProgramY + drawProgramOneChannelH * i, buf, 0x00FFFFFF);
			break;
		}
	}
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
		if (CMDLG_ChooseFile(hWindowDx,filepath, filepath, TEXT(FILTER_MIDI)))
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
			const TCHAR metatype[7][4] = { L"�ġ���",L"�桡Ȩ",L"������",L"������",L"�衡��",L"�ꡡ��",L"�ãգ�" };
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
			CMDLG_MessageBox(hWindowDx, metainfo.c_str(), TEXT("MIDI����"), MB_ICONINFORMATION, true);
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
		if (!windowed)
		{
			ChangeWindowMode(windowed = TRUE);
			CMDLG_SetUseDxDialogs(!windowed);
		}
		if (pmp->GetDeviceID() == MIDI_DEVICE_USE_VST_PLUGIN)
			((VstPlugin*)pmp->GetPlugin())->ShowPluginWindow(!((VstPlugin*)pmp->GetPlugin())->IsPluginWindowShown());
	}
	if (KeyManager::CheckOnHitKey(KEY_INPUT_T))
	{
		if (pmp->GetDeviceID() == MIDI_DEVICE_USE_VST_PLUGIN)
		{
			TCHAR fileToExport[MAX_PATH], wavePath[MAX_PATH];
			if (pmp->GetPlayStatus() == TRUE)
				CMDLG_MessageBox(hWindowDx, TEXT("����ֹͣ���ţ�����VST�Ĳ���Ϊ��ʼ״̬��ȷ��û����������Ӱ��VST���������ٵ�����"), NULL, MB_ICONEXCLAMATION);
			else if (CMDLG_ChooseFile(hWindowDx, filepath, fileToExport, TEXT(FILTER_MIDI)))
			{
				sprintfDx(wavePath, TEXT("%s.wav"), fileToExport);
				if (CMDLG_ChooseSaveFile(hWindowDx, wavePath, wavePath, TEXT("������Ƶ������32λIEEE����48KHz��ʽ��\0*.wav\0\0")))
				{
					std::basic_string<TCHAR>titleConverting = TEXT("���ڵ��� ");
					titleConverting.append(wavePath);
					titleConverting.append(TEXT(" ����"));
					SetWindowText(titleConverting.c_str());
					if (pmp->PluginExportToWav(fileToExport, wavePath, hWindowDx) == 0)
						CMDLG_MessageBox(hWindowDx, TEXT("�����ɹ���"), fileToExport, MB_ICONINFORMATION);
					else
						CMDLG_MessageBox(hWindowDx, TEXT("����ʧ�ܡ�"), fileToExport, MB_ICONERROR);
					SetWindowText(TEXT(APP_TITLE));
				}
			}
		}
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
		CMDLG_InfoBox(hWindowDx, helpInfo, TEXT("����"));
	if (KeyManager::CheckOnHitKey(KEY_INPUT_R))
		showProgram = (showProgram + 1) % 11;
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
	snprintfDx(str, strsize, TEXT("Space:����/��ͣ S:ֹͣ O:���ļ� Esc:�˳� L:ѭ��[%s] P:����[%s] E:���ͳ���Ϣ[%s] ��/��:����[%3d%%]\n%s%s"),
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