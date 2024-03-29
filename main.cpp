#include<DxLib.h>
#include<MidiPlayer.h>
#include"MIDIScreen.h"
#include"CommonDialogs.h"
#include"DxShell.h"
#include"ResLoader.h"
#include"resource.h"
#include"resource1.h"
#include"vstplugin.h"
#include"MidiController/MidiController.h"

#pragma comment(lib,"ComCtl32.lib")


#define APP_TITLE "Visual MIDI Player"
#define VMP_TEMP_FILENAME "vmp_temp.mid"
#define CHOOSE_DEVICE_USER_CLICKED_CANCEL (UINT)-2
#define FILTER_VST "VST插件\0*.dll\0所有文件\0*\0\0"
#define FILTER_SOUNDFONT "SF2音色库\0*.sf2\0所有文件\0*\0\0"
#define FILTER_MIDI "MIDI 序列\0*.mid;*.rmi\0所有文件\0*\0\0"
#define STR_MENU_SHOWCONTROL "显示操作界面(&O)"
#define ID_MENU_SHOWCONTROL 2001
#define FILTER_DOMINO "Domino XML 文件\0*.xml\0所有文件\0*\0\0"

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
	static INT_PTR CALLBACK ControlProcess(HWND, UINT, WPARAM, LPARAM);
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
	//0=不显示，1=显示3列音色号，2=显示音色名，3=显示1列音色号和音色名，4=显示3列音色号和音色名，
	//5=用数字显示详细信息，6=用水平图形显示详细信息，7=用垂直图形显示详细信息，
	//8=用数字显示详细信息和音色名，9=用水平图形显示详细信息和音色名，10=用垂直图形显示详细信息和音色名
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

const TCHAR helpLabel[] = TEXT("F1:帮助");
const TCHAR helpInfo[] = TEXT("【界面未标示的其他功能】\n\nZ: 加速 X: 恢复原速 C: 减速\nV: 用不同的颜色表示音色\nI: 显示MIDI数据\n")
	TEXT("R: 显示音色等通道信息\nD: 重新选择MIDI输出设备\nN: 重新选择MIDI输入设备\nW: 显示/隐藏VST窗口\nT: 导出音频(使用VST播放时)\nF11: 切换全屏显示\n1,2...9,0: 静音/取消静音第1,2...9,10通道\n")
	TEXT("Shift+1,2...6: 静音/取消静音第11,12...16通道\nShift+Space：播放/暂停（无丢失）\n←/→: 定位\n\n")
	TEXT("通道信息的各数值分别表示：\n音色，CC0，CC32，调制(Modulation)，音量(Volume)，平衡(Pan)，表情(Expression)\n\n")
	TEXT("屏幕钢琴框架颜色表示的MIDI模式：\n蓝色:GM 橘黄色:GS 绿色:XG 银灰色:GM2\n\n")
	TEXT("若使用VST播放，你需要安装DirectX运行时来获得更好的性能：\n")
	TEXT("https://www.microsoft.com/download/details.aspx?id=8109\n\n")
	TEXT("制作：lxfly2000\nhttps://github.com/lxfly2000/VisualMIDIPlayer");
#include"Instruments.h"
#include<vector>
#include<string>

VMPlayer* VMPlayer::_pObj = nullptr;
WNDPROC VMPlayer::dxProcess = nullptr;
HWND hDlgControl = NULL;

unsigned VMPlayer::ChooseDevice(LPTSTR extraInfoPath,bool useDefaultIfOnlyOne)
{
	unsigned nMidiDev = midiOutGetNumDevs();
	UINT cur = 0;
	MIDIOUTCAPS caps;
	if (nMidiDev > 1||!useDefaultIfOnlyOne)
	{
		std::vector<const TCHAR*>midiList;
		std::vector<std::basic_string<TCHAR>> midiListVector;
		midiListVector.push_back(TEXT("MIDI 映射器"));
		for (int i = 0; i < (int)nMidiDev; i++)
		{
			midiOutGetDevCaps((UINT_PTR)i, &caps, sizeof(caps));
			midiListVector.push_back(caps.szPname);
		}
		for (UINT i = 0; i < nMidiDev+1; i++)
			midiList.push_back(midiListVector[i].c_str());
		midiList.push_back(TEXT("使用VST插件……"));
		//midiList.push_back(TEXT("使用SF2音色库……"));
		tagChooseDeviceDialog:
		cur = (UINT)CMDLG_ChooseList(hWindowDx, TEXT("选择 MIDI 输出设备"), midiList.data(), (int)midiList.size(),
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
		midiList.insert(midiList.begin(), TEXT("不使用"));
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
	case WM_SYSCOMMAND:
		switch (wp)
		{
		case ID_MENU_SHOWCONTROL:
			ShowWindow(hDlgControl, IsWindowVisible(hDlgControl) ? SW_HIDE : SW_SHOW);
			break;
		}
		break;
	case WM_DROPFILES://https://github.com/lxfly2000/XAPlayer/blob/master/Main.cpp#L301
		VMPlayer::_pObj->OnDrop((HDROP)wp);
		break;
	}
	return CallWindowProc(dxProcess, hwnd, msg, wp, lp);
}

#include<windowsx.h>
#include<atlmem.h>

struct DominoBankNode
{
	std::wstring name;
	WORD subBankNumber;
};

struct DominoPCNode
{
	std::wstring name;
	BYTE bankNumber;
	std::vector<DominoBankNode>banks;
};

struct DominoMapNode
{
	std::wstring name;
	std::vector<DominoPCNode>pcs;
};

struct DominoBank
{
	TCHAR dominoPath[MAX_PATH] = TEXT("");
	std::vector<DominoMapNode>instruments[2];//0:Inst 1:Drum
	BOOL LoadBank(HWND hwnd, LPCTSTR path, int channel)
	{
		TCHAR buttonText[263];
		if(!ReadNodes(path))
		{
			wsprintf(buttonText, TEXT("%s【加载失败】(&D)"), wcsrchr(path, '\\') + 1);
			SetDlgItemText(hwnd, IDC_DOMINO_BROWSE, buttonText);
			return FALSE;
		}
		LoadList(hwnd, channel);
		lstrcpy(dominoPath, path);
		wsprintf(buttonText, TEXT("%s(&D)"), wcsrchr(dominoPath, '\\') + 1);
		SetDlgItemText(hwnd, IDC_DOMINO_BROWSE, buttonText);
		return TRUE;
	}
	void LoadList(HWND hwnd, int channel)
	{
		HWND hTree = GetDlgItem(hwnd, IDC_TREE_DOMINO);
		TreeView_DeleteAllItems(hTree);
		std::vector<DominoMapNode>&dmap = instruments[channel == 10 ? 1 : 0];
		TVINSERTSTRUCT tis;
		tis.item.mask = TVIF_TEXT | TVIF_PARAM;
		tis.item.pszText = (channel == 10 ? TEXT("DrumSetList") : TEXT("InstrumentList"));
		tis.item.cchTextMax = lstrlen(tis.item.pszText);
		tis.item.lParam = MAKELPARAM((channel == 10 ? 1 : 0), 0);//List#
		tis.hInsertAfter = TVI_LAST;
		tis.hParent = TVI_ROOT;
		HTREEITEM hRoot = TreeView_InsertItem(hTree, &tis);
		for (int k = 0; k < dmap.size(); k++)
		{
			tis.item.pszText = (LPWSTR)dmap[k].name.c_str();
			tis.item.cchTextMax = dmap[k].name.length();
			tis.item.lParam = MAKELPARAM(k, 1);//Map#
			tis.hParent = hRoot;
			HTREEITEM hMap = TreeView_InsertItem(hTree, &tis);
			for (int j = 0; j < dmap[k].pcs.size(); j++)
			{
				TCHAR buf[64];
				wsprintf(buf, TEXT("[%d]%s"), dmap[k].pcs[j].bankNumber, dmap[k].pcs[j].name.c_str());
				tis.item.pszText = buf;
				tis.item.cchTextMax = ARRAYSIZE(buf);
				tis.item.lParam = MAKELPARAM(j, 2);//PC#
				tis.hParent = hMap;
				HTREEITEM hPC = TreeView_InsertItem(hTree, &tis);
				for (int i = 0; i < dmap[k].pcs[j].banks.size(); i++)
				{
					wsprintf(buf, TEXT("[%d,%d,%d]%s"), dmap[k].pcs[j].bankNumber, HIBYTE(dmap[k].pcs[j].banks[i].subBankNumber),
						LOBYTE(dmap[k].pcs[j].banks[i].subBankNumber), dmap[k].pcs[j].banks[i].name.c_str());
					tis.item.lParam = MAKELPARAM(i, 3);//SUB#
					tis.hParent = hPC;
					TreeView_InsertItem(hTree, &tis);
				}
			}
		}
	}
	BOOL ReadNodes(LPCTSTR path)
	{
/*Domino音色定义文件
ModuleData Name="模块名"
+ InstrumentList
| + Map Name="音色表名"
| | + PC Name="音色名" PC="音色号"
| | | + Bank Name="音色名" MSB="子音色号高字节" LSB="子音色号低字节"
| | | + Bank ...
| | + PC ...
| + Map ...
+ DrumSetList
| + Map Name="音色表名"
| | + PC Name="音色名" PC="音色号"
| | | + Bank Name="音色名" MSB="子音色号高字节" LSB="子音色号低字节"
| | | | + Tone Name="音名" Key="键号"
| | | | + Tone ...
| | | + Bank ...
| | + PC ...
| + Map ...
+ （其他选项）
*/
#define HC(x)if(FAILED(x))return FALSE
		VARIANT_BOOL vSuccess;
		CComVariant v;
		CComPtr<IXMLDOMDocument>pXmlDoc;//它的自动释放功能会与CoUninitalize起冲突，因此把它放到单独的函数中
		CComPtr<IXMLDOMElement>pXmlRootElem;
		//https://blog.csdn.net/zsscy/article/details/6444215
		HC(pXmlDoc.CoCreateInstance(CLSID_DOMDocument));
		//Variant可以用CComVariant简化
		//https://www.cnblogs.com/lingyun1120/archive/2011/11/02/2232709.html
		HC(pXmlDoc->load(CComVariant(path), &vSuccess));//可以用load加载XML文件，save保存文件，且保存后自动转为UTF-8编码（这点好评）
		if (vSuccess == FALSE)
			return FALSE;
		HC(pXmlDoc->get_documentElement(&pXmlRootElem));
		//读取XML
		LPCTSTR tagNames[] = { TEXT("InstrumentList"),TEXT("DrumSetList") };
		for (int m = 0; m < 2; m++)
		{
			CComPtr<IXMLDOMNodeList>pXmlModuleNodes;
			instruments[m].clear();
			HC(pXmlRootElem->getElementsByTagName(CComBSTR(tagNames[m]), &pXmlModuleNodes));
			CComPtr<IXMLDOMNode>pXmlModuleNode;
			LONG lengthList,lengthMap;
			HC(pXmlModuleNodes->get_length(&lengthList));
			if (lengthList < 1)
				continue;
			HC(pXmlModuleNodes->get_item(0, &pXmlModuleNode));
			CComPtr<IXMLDOMElement>pXmlModuleElem;
			HC(pXmlModuleNode->QueryInterface(&pXmlModuleElem));
			CComPtr<IXMLDOMNodeList>pXmlMapNodes;
			HC(pXmlModuleElem->getElementsByTagName(CComBSTR("Map"), &pXmlMapNodes));
			HC(pXmlMapNodes->get_length(&lengthMap));
			for (int k = 0; k < lengthMap; k++)
			{
				//Map
				instruments[m].push_back(DominoMapNode());
				DominoMapNode &dmap = instruments[m].back();
				CComPtr<IXMLDOMNode>pXmlMapNode;
				HC(pXmlMapNodes->get_item(k, &pXmlMapNode));
				CComPtr<IXMLDOMElement>pXmlMapElem;
				HC(pXmlMapNode->QueryInterface(&pXmlMapElem));
				HC(pXmlMapElem->getAttribute(CComBSTR("Name"), &v));
				dmap.name = v.bstrVal;
				CComPtr<IXMLDOMNodeList>pXmlPCNodes;
				HC(pXmlMapElem->getElementsByTagName(CComBSTR("PC"),&pXmlPCNodes));
				LONG lengthPC;
				HC(pXmlPCNodes->get_length(&lengthPC));
				for (int j = 0; j < lengthPC; j++)
				{
					//PC
					dmap.pcs.push_back(DominoPCNode());
					DominoPCNode&dpc = dmap.pcs.back();
					CComPtr<IXMLDOMNode>pXmlPCNode;
					HC(pXmlPCNodes->get_item(j, &pXmlPCNode));
					CComPtr<IXMLDOMElement>pXmlPCElem;
					HC(pXmlPCNode->QueryInterface(&pXmlPCElem));
					HC(pXmlPCElem->getAttribute(CComBSTR("Name"), &v));
					dpc.name = v.bstrVal;
					HC(pXmlPCElem->getAttribute(CComBSTR("PC"), &v));
					dpc.bankNumber = _ttoi(v.bstrVal);
					CComPtr<IXMLDOMNodeList>pXmlBankNodes;
					HC(pXmlPCElem->getElementsByTagName(CComBSTR("Bank"), &pXmlBankNodes));
					LONG lengthBank;
					HC(pXmlBankNodes->get_length(&lengthBank));
					for (int i = 0; i < lengthBank; i++)
					{
						//Bank
						dpc.banks.push_back(DominoBankNode());
						DominoBankNode&dbank = dpc.banks.back();
						CComPtr<IXMLDOMNode>pXmlBankNode;
						HC(pXmlBankNodes->get_item(i, &pXmlBankNode));
						CComPtr<IXMLDOMElement>pXmlBankElem;
						HC(pXmlBankNode->QueryInterface(&pXmlBankElem));
						HC(pXmlBankElem->getAttribute(CComBSTR("Name"), &v));
						dbank.name = v.bstrVal;
						HC(pXmlBankElem->getAttribute(CComBSTR("MSB"), &v));
						dbank.subBankNumber = (v.vt == VT_NULL ? 0 : _ttoi(v.bstrVal));
						HC(pXmlBankElem->getAttribute(CComBSTR("LSB"), &v));
						dbank.subBankNumber = (v.vt == VT_NULL ? 0 : MAKEWORD(_ttoi(v.bstrVal), dbank.subBankNumber));
					}
				}
			}
		}
		return TRUE;
#undef HC
	}
	void ChooseItem(HWND hwnd, LPTVITEM pItem)
	{
		HWND hTree = GetDlgItem(hwnd, IDC_TREE_DOMINO);
		int level = HIWORD(pItem->lParam);
		if (level < 2)
			return;
		int index[4] = { 0 };
		TVITEM tvi{};
		tvi.hItem = pItem->hItem;
		tvi.mask = TVIF_PARAM;
		do
		{
			TreeView_GetItem(hTree, &tvi);
			index[HIWORD(tvi.lParam)] = LOWORD(tvi.lParam);
			tvi.hItem = TreeView_GetParent(hTree, tvi.hItem);
		}while (tvi.hItem);
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_PROGRAM), instruments[index[0]][index[1]].pcs[index[2]].bankNumber - 1);
		SetDlgItemInt(hwnd, IDC_EDIT_MSB, HIBYTE(instruments[index[0]][index[1]].pcs[index[2]].banks[index[3]].subBankNumber), FALSE);
		SetDlgItemInt(hwnd, IDC_EDIT_LSB, LOBYTE(instruments[index[0]][index[1]].pcs[index[2]].banks[index[3]].subBankNumber), FALSE);
		SendMessage(hwnd, WM_COMMAND, IDC_BUTTON_PC, 0);
	}
}dominoBank;

INT_PTR CALLBACK VMPlayer::ControlProcess(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		CheckDlgButton(hwnd, IDC_CHECK_KEEP_EDIT, BST_CHECKED);
		CheckDlgButton(hwnd, IDC_CHECK_AUTOPC, BST_CHECKED);
		SendDlgItemMessage(hwnd, IDC_SPIN_CHANNEL, UDM_SETRANGE32, 1, 16);
		SendDlgItemMessage(hwnd, IDC_SPIN_MSB, UDM_SETRANGE32, 0, 127);
		SendDlgItemMessage(hwnd, IDC_SPIN_LSB, UDM_SETRANGE32, 0, 127);
		SetDlgItemInt(hwnd, IDC_EDIT_CHANNEL, 1, FALSE);
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_PROGRAM), 0);
		HWND hMsg = GetDlgItem(hwnd, IDC_COMBO_SENDMSG);
		ComboBox_AddString(hMsg, TEXT("F0 7E 7F 09 01 F7"));
		ComboBox_AddString(hMsg, TEXT("F0 7E 7F 09 02 F7"));
		ComboBox_AddString(hMsg, TEXT("F0 7E 7F 09 03 F7"));
		ComboBox_AddString(hMsg, TEXT("F0 41 00 42 12 40 00 7F 00 41 F7"));
		ComboBox_AddString(hMsg, TEXT("F0 41 00 42 12 40 00 7F 7F 42 F7"));
		ComboBox_AddString(hMsg, TEXT("F0 41 00 42 12 00 00 7F 00 00 F7"));
		ComboBox_AddString(hMsg, TEXT("F0 43 10 4C 00 00 7E 00 F7"));
	}
		break;
	case WM_SHOWWINDOW:
		CheckMenuItem(GetSystemMenu(_pObj->hWindowDx, FALSE), ID_MENU_SHOWCONTROL, wp ? MF_CHECKED : MF_UNCHECKED);
		break;
	case WM_COMMAND:
		switch (LOWORD(wp))
		{
		case IDCANCEL:
			ShowWindow(hwnd, SW_HIDE);
			break;
		case IDC_EDIT_CHANNEL:
			if (HIWORD(wp) == EN_CHANGE)
			{
				HWND h = GetDlgItem(hwnd, IDC_COMBO_PROGRAM);
				if (h)
				{
					int isel = ComboBox_GetCurSel(h);
					static int ilastch = 10;
					int ich = GetDlgItemInt(hwnd, IDC_EDIT_CHANNEL, NULL, FALSE);
					if (ich == 10 || ilastch == 10)
					{
						ComboBox_ResetContent(h);
						TCHAR t[32];
						for (int i = 0; i < 128; i++)
						{
							if (ich == 10)
								wsprintf(t, TEXT("[%d]%s"), i, drumName[i]);
							else
								wsprintf(t, TEXT("[%d]%s"), i, programName[i]);
							ComboBox_AddString(h, t);
						}
						ComboBox_SetCurSel(h, isel);
						dominoBank.LoadList(hwnd, ich);
					}
					ilastch = ich;
				}
			}
			break;
		case IDC_COMBO_PROGRAM:
			if (HIWORD(wp) == CBN_SELCHANGE && IsDlgButtonChecked(hwnd, IDC_CHECK_AUTOPC) == BST_CHECKED)
				SendMessage(hwnd, WM_COMMAND, IDC_BUTTON_PC, 0);
			break;
		case IDC_EDIT_MSB:
		case IDC_EDIT_LSB:
			if (HIWORD(wp) == EN_CHANGE && IsDlgButtonChecked(hwnd, IDC_CHECK_AUTOPC) == BST_CHECKED)
				SendMessage(hwnd, WM_COMMAND, IDC_BUTTON_PC, 0);
			break;
		case IDOK:
			if (_pObj&&_pObj->pmp)
			{
				TCHAR buf[256];
				ComboBox_GetText(GetDlgItem(hwnd, IDC_COMBO_SENDMSG), buf, ARRAYSIZE(buf));
				std::vector<BYTE> mdata;
				for (int i = 0; i < ARRAYSIZE(buf); i++)
				{
					if (buf[i] >= '0'&&buf[i] <= '9')
						mdata.push_back(buf[i] - '0');
					else if (buf[i] >= 'A'&&buf[i] <= 'F')
						mdata.push_back(buf[i] - 'A' + 10);
					else if (buf[i] >= 'a'&&buf[i] <= 'f')
						mdata.push_back(buf[i] - 'a' + 10);
				}
				if (mdata.size() % 2 == 1)
					mdata.push_back(0);
				for (int i = 0; i < mdata.size();i++)
				{
					mdata[i] = (mdata[i] << 4) | mdata[i + 1];
					mdata.erase(mdata.begin() + i + 1);
				}
				while (mdata.size() < 4)
					mdata.push_back(0);
				if (mdata.size() == 4)
					_pObj->pmp->_ProcessMidiShortEvent(*(PDWORD)mdata.data(), true);
				else
				{
					MIDIHDR hdr = { 0 };
					hdr.lpData = (LPSTR)mdata.data();
					hdr.dwBufferLength = mdata.size();
					_pObj->pmp->_ProcessMidiLongEvent(&hdr, true);
				}
			}
			if (IsDlgButtonChecked(hwnd, IDC_CHECK_KEEP_EDIT) == BST_UNCHECKED)
				ComboBox_SetText(GetDlgItem(hwnd, IDC_COMBO_SENDMSG), TEXT(""));
			break;
		case IDC_BUTTON_PC:
			if (_pObj&&_pObj->pmp)
			{
				int ch = (GetDlgItemInt(hwnd, IDC_EDIT_CHANNEL, NULL, FALSE) - 1) & 0xF;
				int pg = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_PROGRAM)) & 0x7F;
				int msb = GetDlgItemInt(hwnd, IDC_EDIT_MSB, NULL, FALSE);
				int lsb = GetDlgItemInt(hwnd, IDC_EDIT_LSB, NULL, FALSE);
				_pObj->pmp->_ProcessMidiShortEvent((0xB0 | ch) | (msb << 16), true);
				_pObj->pmp->_ProcessMidiShortEvent((0xB0 | ch) | 0x2000 | (lsb << 16), true);
				_pObj->pmp->_ProcessMidiShortEvent((0xC0 | ch) | (pg << 8), true);
			}
			break;
		case IDC_DOMINO_BROWSE:
			if (CMDLG_ChooseFile(hwnd, dominoBank.dominoPath, dominoBank.dominoPath, TEXT(FILTER_DOMINO)))
				dominoBank.LoadBank(hwnd, dominoBank.dominoPath, GetDlgItemInt(hwnd, IDC_EDIT_CHANNEL, NULL, FALSE));
			break;
		}
		break;
	case WM_NOTIFY://较老的控件使用WM_COMMAND发送通知，新控件使用WM_NOTIFY发送通知
		if (((LPNMHDR)lp)->code == TVN_SELCHANGED && ((LPNMHDR)lp)->idFrom == IDC_TREE_DOMINO)
			dominoBank.ChooseItem(hwnd, &((LPNMTREEVIEW)lp)->itemNew);
		break;
	}
	return 0;
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
	AppendMenu(GetSystemMenu(hWindowDx, FALSE), MF_STRING, ID_MENU_SHOWCONTROL, TEXT(STR_MENU_SHOWCONTROL));
	dxProcess = (WNDPROC)GetWindowLongPtr(hWindowDx, GWLP_WNDPROC);
	SetWindowLongPtr(hWindowDx, GWL_EXSTYLE, WS_EX_ACCEPTFILES | GetWindowLongPtr(hWindowDx, GWL_EXSTYLE));
	SetWindowLongPtr(hWindowDx, GWLP_WNDPROC, (LONG_PTR)ExtraProcess);

	hDlgControl = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONTROL), hWindowDx, ControlProcess);

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
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
		{
			IXAudio2*x = nullptr;
			HRESULT hrx = XAudio2Create(&x);
			if (x)
				x->Release();
			if (hrx == 0x80040154)
			{
				if (MessageBox(hWindowDx, TEXT("XAudio2加载失败。\n你需要安装或升级DirectX运行时来使用此功能。\n\n如需安装请点击“确定”。"), NULL, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK)
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
#endif
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
			//CMDLG_MessageBox(hWindowDx, TEXT("加载失败，请检查文件是否是有效的SF2音色库文件。"), NULL, MB_ICONEXCLAMATION);
			CMDLG_MessageBox(hWindowDx, TEXT("SF2功能尚未制作。"), NULL, MB_ICONEXCLAMATION);
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
		if (ProcessMessage())
			break;
		if (GetActiveWindow() != hDlgControl)
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
		DrawChannelInfo();
	DrawString(drawHelpLabelX, drawHelpLabelY, helpLabel, 0x00FFFFFF);
	ScreenFlip();
}

static LPCTSTR GetSymbolicValueText(int v, bool center = false, bool vertical = false)
{
	static LPCTSTR symbolicValueText[] =
	{
		TEXT("　　"),TEXT("▏　"),TEXT("▎　"),TEXT("▍　"),TEXT("▌　"),TEXT("▋　"),TEXT("▊　"),TEXT("▉　"),TEXT("█　"),
		TEXT("█▏"),TEXT("█▎"),TEXT("█▍"),TEXT("█▌"),TEXT("█▋"),TEXT("█▊"),TEXT("█▉"),TEXT("██")
	};
	static LPCTSTR symbolicValueTextVertical[] =
	{
		TEXT("　"),TEXT("▁"),TEXT("▂"),TEXT("▃"),TEXT("▄"),TEXT("▅"),TEXT("▆"),TEXT("▇"),TEXT("█")
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
#define GetChannelEnabledText(ch) pmp->GetChannelEnabled(ch)?TEXT(""):TEXT("静音")
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
				CMDLG_MessageBox(hWindowDx, TEXT("请先停止播放，调整VST的参数为初始状态并确保没有其他因素影响VST的声音后再导出。"), NULL, MB_ICONEXCLAMATION);
			else if (CMDLG_ChooseFile(hWindowDx, filepath, fileToExport, TEXT(FILTER_MIDI)))
			{
				sprintfDx(wavePath, TEXT("%s.wav"), fileToExport);
				if (CMDLG_ChooseSaveFile(hWindowDx, wavePath, wavePath, TEXT("波形音频（仅限32位IEEE浮点48KHz格式）\0*.wav\0\0")))
				{
					std::basic_string<TCHAR>titleConverting = TEXT("正在导出 ");
					titleConverting.append(wavePath);
					titleConverting.append(TEXT(" ……"));
					SetWindowText(titleConverting.c_str());
					if (pmp->PluginExportToWav(fileToExport, wavePath, hWindowDx) == 0)
						CMDLG_MessageBox(hWindowDx, TEXT("导出成功。"), fileToExport, MB_ICONINFORMATION);
					else
						CMDLG_MessageBox(hWindowDx, TEXT("导出失败。"), fileToExport, MB_ICONERROR);
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
		CMDLG_InfoBox(hWindowDx, helpInfo, TEXT("帮助"));
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