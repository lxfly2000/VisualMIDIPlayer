#pragma once
#include<Windows.h>

//������Դ�ַ���
LPCWSTR LoadLocalStringW(UINT id);

#ifdef _UNICODE
#define LoadLocalString LoadLocalStringW
#endif
