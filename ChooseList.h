#pragma once
#include<Windows.h>
#ifdef __cplusplus
extern "C"{
#endif
//������ѡ����ѡ����Ŀ����ѡ��
//hwnd: �����ھ��
//title: ���⣬����Ϊ��
//options: ѡ��
//cOptions: �ж��ٸ�ѡ��
//nDefault: Ĭ��ѡ�Ĭ��ѡ���Ԥ��ѡ�У�
//pcszCheck: ��ѡ�����֣�����ΪNULL��
//retChosen: ��ѡ���Ƿ�ѡ�У�����ΪNULL��
//����ֵΪ��ѡ�����������ѡ��ȡ���򷵻�-1
int ChooseList(HWND hwnd, LPCTSTR title, LPCTSTR *options, int cOptions, int nDefault,LPCTSTR pcszCheck,BOOL *retChosen);
#ifdef __cplusplus
}
#endif
