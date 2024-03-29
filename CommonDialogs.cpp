#include "common.h"
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

BOOL SelectFile(HWND hwnd, TCHAR* filepath, TCHAR* filename, LPCTSTR filter, bool isSave = false)
{
	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof OPENFILENAME;
	ofn.hwndOwner = hwnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filepath;
	ofn.lpstrTitle = isSave ? TEXT("选择保存位置") : TEXT("选择文件");
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = filename;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	if (isSave)
		ofn.Flags |= OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = NULL;
	return isSave ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);
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

int CMDLG_ChooseSaveFile(HWND hwnd, LPCTSTR initPath, LPTSTR outputPath, LPCTSTR filter)
{
	if (useDxDialogs)
	{
		TCHAR buf[MAX_PATH];
		strcpyDx(buf, initPath);
		if (DxGetInputString(LoadLocalString(IDS_DXMSG_INPUT_SAVE_PATH), buf, MAX_PATH) != -1)
		{
			strcpyDx(outputPath, buf);
			return TRUE;
		}
		return FALSE;
	}
	else if (initPath == outputPath)
	{
		return SelectFile(hwnd, outputPath, NULL, filter, true);
	}
	else
	{
		TCHAR filePath[MAX_PATH];
		lstrcpy(filePath, initPath);
		if (SelectFile(hwnd, filePath, NULL, filter, true))
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
#if _WIN32_WINNT>=_WIN32_WINNT_VISTA
		TASKDIALOGCONFIG tdc{};
		tdc.cbSize = sizeof(tdc);
		tdc.hwndParent = hwnd;
		tdc.hInstance = GetModuleHandle(NULL);
		tdc.dwFlags = TDF_ENABLE_HYPERLINKS;
		tdc.dwCommonButtons = TDCBF_OK_BUTTON;
		tdc.pszWindowTitle = title;
		tdc.pszMainIcon = TD_INFORMATION_ICON;
		std::basic_string<TCHAR>msgWithURL = msg;
		msgWithURL = std::regex_replace(msgWithURL, std::basic_regex<TCHAR>(TEXT("(https?://[A-Za-z0-9_\\-\\./\\?&=]+)")), TEXT("<a href=\"$1\">$1</a>"));
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
		typedef HRESULT(WINAPI*TTaskDialogIndirect)(_In_ const TASKDIALOGCONFIG *pTaskConfig, _Out_opt_ int *pnButton, _Out_opt_ int *pnRadioButton, _Out_opt_ BOOL *pfVerificationFlagChecked);
		TTaskDialogIndirect f = (TTaskDialogIndirect)GetProcAddress(LoadLibrary(TEXT("comctl32.dll")), "TaskDialogIndirect");
		if (f)
			f(&tdc, NULL, NULL, NULL);
		else
			MessageBox(hwnd, msg, tdc.pszWindowTitle, MB_ICONINFORMATION);
#elif _WIN32_WINNT>=_WIN32_WINNT_WINXP
		ShellMessageBox(GetModuleHandle(NULL), hwnd, msg, title, MB_ICONINFORMATION);
#else
		MessageBox(hwnd, msg, title, MB_ICONINFORMATION);
#endif
	}
}
