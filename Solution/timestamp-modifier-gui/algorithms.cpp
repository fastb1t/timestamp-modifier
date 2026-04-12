#include "algorithms.h"


BOOL DrawLine(HDC hDC, int x0, int y0, int x1, int y1)
{
    MoveToEx(hDC, x0, y0, NULL);
    return LineTo(hDC, x1, y1);
}


HWND CreateToolTip(HINSTANCE hInstance, HWND hCtrlWnd, const TCHAR* pszText)
{
    if (!hCtrlWnd || !pszText)
    {
        return NULL;
    }

    HWND hTipWnd = CreateWindowEx(
        0,
        TOOLTIPS_CLASS,
        _T(""),
        WS_POPUP | TTS_ALWAYSTIP, // | TTS_BALLOON,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        GetParent(hCtrlWnd),
        NULL,
        hInstance,
        NULL
    );

    if (!hTipWnd)
    {
        return NULL;
    }

    TOOLINFO ti;
    memset(&ti, 0, sizeof(TOOLINFO));

    ti.cbSize = sizeof(TOOLINFO);
    ti.hwnd = GetParent(hCtrlWnd);
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.uId = (UINT_PTR)hCtrlWnd;
    ti.lpszText = (LPSTR)pszText;

    SendMessage(hTipWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);

    return hTipWnd;
}
