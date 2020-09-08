#pragma once
#include<Windows.h>
#ifdef __cplusplus
extern "C"{
#endif
//从所给选项中选择项目（单选）
//hwnd: 父窗口句柄
//title: 标题，可以为空
//options: 选项
//cOptions: 有多少个选项
//nDefault: 默认选项（默认选项会预先选中）
//pcszCheck: 单选框文字（可以为NULL）
//retChosen: 单选框是否选中（可以为NULL）
//返回值为所选项索引，如果选了取消则返回-1
int ChooseList(HWND hwnd, LPCTSTR title, LPCTSTR *options, int cOptions, int nDefault,LPCTSTR pcszCheck,BOOL *retChosen);
#ifdef __cplusplus
}
#endif
