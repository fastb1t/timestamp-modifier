#include <tchar.h>
#include <windows.h>

#include "wnd_proc.h"
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")


// [_tWinMain]: точка входу в програму.
int WINAPI _tWinMain(
    _In_        HINSTANCE hInstance,        // Дескриптор екземпляра програми, який передається від операційної системи при запуску програми.
    _In_opt_    HINSTANCE hPrevInstance,    // Дескриптор попереднього екземпляра програми. Застарілий параметр, як правило, має значення NULL.
    _In_        LPTSTR lpCmdLine,           // Рядок, що містить командний рядок, переданий програмі при запуску.
    _In_        int nShowCmd                // Параметр, який вказує, як вікно програми має бути показане після створення.
)
{
    const TCHAR szWindowName[] = _T("timestamp-modifier-gui");              // Заголовок програми.
    const TCHAR szClassName[] = _T("__timestamp-modifier-gui__class__");    // І'мя класу вікна.
    const TCHAR szMutexName[] = _T("__timestamp-modifier-gui__mutex__");    // І'мя м'ютекса.


    // Створюємо м'ютекс.
    HANDLE hMutex = CreateMutex(NULL, FALSE, szMutexName);
    if (hMutex == NULL)
    {
        MessageBox(NULL, _T("Failed to create mutex."), _T("Error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);
        return 1;
    }
    else
    {
        // Перевіряємо чи це не є повторний запуск програми.
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            MessageBox(NULL, _T("The program instance is already running."), _T("Error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);

            HWND hWnd = FindWindow(szClassName, szWindowName); // Шукаємо вже запущений екземпляр програми.
            if (hWnd != NULL)
            {
                if (IsIconic(hWnd)) // Якщо вікно згорнуте.
                {
                    ShowWindow(hWnd, SW_RESTORE); // Відновлюєм.
                }

                SetForegroundWindow(hWnd); // Ставим фокус на вікно.
            }

            CloseHandle(hMutex);
            return 1;
        }
    }


    WNDCLASSEX wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);                                   // Вказуємо розмір структури.
    wcex.style = CS_HREDRAW | CS_VREDRAW;                               // Стиль класу.
    wcex.lpfnWndProc = WindowProcedure;                                 // Вказівник на віконну процедуру.
    wcex.cbClsExtra = 0;                                                // Це поле система заповнює сама.
    wcex.cbWndExtra = 0;                                                // Це поле система заповнює сама.
    wcex.hInstance = hInstance;                                         // Дескриптор екземпляра, який містить віконну процедуру для класу.
    //wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));       // Дескриптор значка.
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);                       // Дескриптор значка.
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);                         // Дескриптор курсору.
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);                        // Дескриптор кисті фону.
    wcex.lpszMenuName = NULL;                                           // Вказівник на меню.
    wcex.lpszClassName = szClassName;                                   // Вказуємо і'мя класу вікна.
    //wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));     // Дескриптор маленького значка.
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);                     // Дескриптор маленького значка.

    // Реєструємо клас для головного вікна.
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("Failed to register window class."), _T("Error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);

        ReleaseMutex(hMutex);
        CloseHandle(hMutex);

        return 1;
    }


    // Стиль вікна.
    DWORD dwStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;

    // Розширений стиль для вікна.
    DWORD dwExStyle = WS_EX_APPWINDOW;

    RECT rc;

    // Задаємо розмір робочої області вікна.
    SetRect(&rc, 0, 0, 840, 500);

    // Коригуємо розмір вікна враховуючи його стилі.
    AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);

    const int iWindowWidth = rc.right - rc.left;    // Ширина вікна з урахуванням стилів вікна.
    const int iWindowHeight = rc.bottom - rc.top;   // Висота вікна з урахуванням стилів вікна.

    // Отримуємо розмір робочої області екрану, для розміщення вікна в центрі екрану.
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

    // Створюємо головне вікно.
    HWND hWnd = CreateWindowEx(
        dwExStyle,                                                      // Розширений стиль вікна.
        szClassName,                                                    // Ім'я класу вікна.
        szWindowName,                                                   // Заголовок вікна.
        dwStyle,                                                        // Стиль вікна.
        (GetSystemMetrics(SM_CXSCREEN) >> 1) - (iWindowWidth >> 1),     // Координата верхнього лівого кута.
        ((rc.bottom - rc.top) >> 1) - (iWindowHeight >> 1),             // Координата верхнього правого кута.
        iWindowWidth,                                                   // Ширина вікна.
        iWindowHeight,                                                  // Висота вікна.
        HWND_DESKTOP,                                                   // Дескриптор батьківського вікна.
        NULL,                                                           // Дескриптор меню.
        hInstance,                                                      // Дескриптор екземпляра
        NULL                                                            // Вказівник на додаткові дані для передачі в процедуру обробки повідомлень.
    );

    // Перевіряємо чи вікно створилось.
    if (hWnd == NULL)
    {
        MessageBox(NULL, _T("Failed to create window."), _T("Error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);

        UnregisterClass(szClassName, hInstance);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);

        return 1;
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);

    MSG msg;
    BOOL bRet;

    // Цикл отримання та обробки повідомлень.
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            // Помилка отримання повідомлення.
            break;
        }
        else
        {
            // Обробляєм отримані повідомлення.
            DispatchMessage(&msg);
            TranslateMessage(&msg);
        }
    }

    UnregisterClass(szClassName, hInstance);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return static_cast<int>(msg.wParam);
}
// [/_tWinMain]
