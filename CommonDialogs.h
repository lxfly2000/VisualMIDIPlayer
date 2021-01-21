#pragma once
#include <Windows.h>
void CMDLG_DxShellSetUseHighDpi(bool);
void CMDLG_SetUseDxDialogs(bool);
//返回值：按确定/是时返回TRUE，否则返回FALSE
//appendTitle仅在使用Dx对话框时有效
//当uType含有MB_YESNO选项时Dx对话框会附加是/否选项的文字，且确定/取消键被指定为Y/N
int CMDLG_MessageBox(HWND hwnd, LPCTSTR msg, LPCTSTR title = NULL, UINT uType = MB_OK, bool useShellBox = false, bool appendTitle = false);
//appendTitle仅在使用Dx对话框时有效
//在使用Win32对话框时，该对话框可以替换文本中的链接文字为可点击的链接
void CMDLG_InfoBox(HWND hwnd, LPCTSTR msg, LPCTSTR title = NULL, bool appendTitle = false);
//hwnd仅在使用Win32对话框时有效
int CMDLG_ChooseList(HWND hwnd,LPCTSTR title,LPCTSTR *options,int cOptions,int defaultChoose);
//initPath和outputPath可以为相同值
//返回值：按确定时返回TRUE，否则返回FALSE
//outputPath只有在按了确定时才会被更新
//hwnd仅在使用Win32对话框时有效
int CMDLG_ChooseFile(HWND hwnd,LPCTSTR initPath, LPTSTR outputPath,LPCTSTR filter);
