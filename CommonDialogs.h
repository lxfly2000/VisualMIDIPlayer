#pragma once
#include <Windows.h>
void CMDLG_DxShellSetUseHighDpi(bool);
void CMDLG_SetUseDxDialogs(bool);
//����ֵ����ȷ��/��ʱ����TRUE�����򷵻�FALSE
//appendTitle����ʹ��Dx�Ի���ʱ��Ч
//��uType����MB_YESNOѡ��ʱDx�Ի���ḽ����/��ѡ������֣���ȷ��/ȡ������ָ��ΪY/N
int CMDLG_MessageBox(HWND hwnd, LPCTSTR msg, LPCTSTR title = NULL, UINT uType = MB_OK, bool useShellBox = false, bool appendTitle = false);
//appendTitle����ʹ��Dx�Ի���ʱ��Ч
//��ʹ��Win32�Ի���ʱ���öԻ�������滻�ı��е���������Ϊ�ɵ��������
void CMDLG_InfoBox(HWND hwnd, LPCTSTR msg, LPCTSTR title = NULL, bool appendTitle = false);
//hwnd����ʹ��Win32�Ի���ʱ��Ч
int CMDLG_ChooseList(HWND hwnd,LPCTSTR title,LPCTSTR *options,int cOptions,int defaultChoose);
//initPath��outputPath����Ϊ��ֵͬ
//����ֵ����ȷ��ʱ����TRUE�����򷵻�FALSE
//outputPathֻ���ڰ���ȷ��ʱ�Żᱻ����
//hwnd����ʹ��Win32�Ի���ʱ��Ч
int CMDLG_ChooseFile(HWND hwnd,LPCTSTR initPath, LPTSTR outputPath,LPCTSTR filter);
