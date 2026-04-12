#ifndef _WND_PROC_H_
#define _WND_PROC_H_

#include <windows.h>

// Процедура для обробки повідомлень головного вікна.
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
