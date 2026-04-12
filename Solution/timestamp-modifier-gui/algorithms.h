#ifndef _ALGORITHMS_H_
#define _ALGORITHMS_H_

#include <tchar.h>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>


typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> String;
typedef std::basic_stringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> StringStream;

template<typename T>
String ToString(T t)
{
    StringStream ss;
    ss << t;
    return ss.str();
}


BOOL DrawLine(HDC hDC, int x0, int y0, int x1, int y1);

HWND CreateToolTip(HINSTANCE hInstance, HWND hCtrlWnd, const TCHAR* pszText);

#endif // !_ALGORITHMS_H_
