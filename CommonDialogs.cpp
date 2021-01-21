#include "CommonDialogs.h"
#include "DxShell.h"
#include "ChooseList.h"
#include "ResLoader.h"
#include "resource1.h"
#include <string>
#include <regex>

static bool useDxDialogs = false;

void CMDLG_DxShellSetUseHighDpi(bool b)
{
	DxShellSetUseHighDpi(b);
}

void CMDLG_SetUseDxDialogs(bool b)
{
	useDxDialogs = b;
}

int CMDLG_MessageBox(HWND hwnd, LPCTSTR msg, LPCTSTR title, UINT uType, bool useShellBox, bool appendTitle)
{
	if (useDxDialogs)
	{
		std::basic_string<TCHAR> strMsg = msg;
		if (title&&appendTitle)
		{
			strMsg.append(2, '\n');
			strMsg = title + strMsg;
		}
		if (uType&MB_YESNO)
			strMsg += LoadLocalString(IDS_DXMSG_APPEND_YESNO);
		else
			strMsg += LoadLocalString(IDS_DXMSG_APPEND_OK);
		if (uType&MB_YESNO)
			return DxMessageBox(strMsg.c_str(), KEY_INPUT_Y, KEY_INPUT_N);
		else
			return DxMessageBox(strMsg.c_str());
	}
	else if (useShellBox)
	{
		int r = ShellMessageBox(NULL, hwnd, msg, title, uType);
		return (r == IDOK || r == IDYES) ? TRUE : FALSE;
	}
	else
	{
		int r = MessageBox(hwnd, msg, title, uType);
		return (r == IDOK || r == IDYES) ? TRUE : FALSE;
	}
}

int CMDLG_ChooseList(HWND hwnd, LPCTSTR title, LPCTSTR *options, int cOptions, int defaultChoose)
{
	if (useDxDialogs)
	{
		std::basic_string<TCHAR> strTitle = title;
		strTitle += TEXT("\n[Enter]确认 [Esc]退出 [↑/↓]选择");
		return DxChooseListItem(strTitle.c_str(), options, cOptions, defaultChoose);
	}
	else
		return ChooseList(hwnd, title, options, cOptions, defaultChoose, NULL, NULL);
}

BOOL SelectFile(HWND hwnd, TCHAR *filepath, TCHAR *filename,LPCTSTR filter)
{
	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof OPENFILENAME;
	ofn.hwndOwner = hwnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filepath;
	ofn.lpstrTitle = TEXT("选择文件");
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = filename;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = NULL;
	return GetOpenFileName(&ofn);
}

int CMDLG_ChooseFile(HWND hwnd,LPCTSTR initPath, LPTSTR outputPath, LPCTSTR filter)
{
	if (useDxDialogs)
	{
		return DxChooseFilePath(initPath, outputPath);
	}
	else if (initPath == outputPath)
	{
		return SelectFile(hwnd,outputPath, NULL, filter);
	}
	else
	{
		TCHAR filePath[MAX_PATH];
		lstrcpy(filePath, initPath);
		if (SelectFile(hwnd,filePath, NULL, filter))
		{
			lstrcpy(outputPath, filePath);
			return TRUE;
		}
		return FALSE;
	}
}

void CMDLG_InfoBox(HWND hwnd, LPCTSTR msg, LPCTSTR title, bool appendTitle)
{
	if (useDxDialogs)
	{
		std::basic_string<TCHAR> strMsg = msg;
		if (title&&appendTitle)
		{
			strMsg.append(2, '\n');
			strMsg = title + strMsg;
		}
		strMsg += LoadLocalString(IDS_DXMSG_APPEND_OK);
		DxMessageBox(strMsg.c_str());
	}
	else
	{
		TASKDIALOGCONFIG tdc{};
		tdc.cbSize = sizeof(tdc);
		tdc.hwndParent = hwnd;
		tdc.hInstance = GetModuleHandle(NULL);
		tdc.dwFlags = TDF_ENABLE_HYPERLINKS;
		tdc.dwCommonButtons = TDCBF_OK_BUTTON;
		tdc.pszWindowTitle = title;
		tdc.pszMainIcon = TD_INFORMATION_ICON;
		std::basic_string<TCHAR>msgWithURL = msg;
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
}
