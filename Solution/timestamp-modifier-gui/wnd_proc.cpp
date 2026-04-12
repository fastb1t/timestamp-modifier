#include <tchar.h>
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "wnd_proc.h"
#include "resource.h"
#include "algorithms.h"

static BOOL OnCreate(HWND, LPCREATESTRUCT);                                     	// WM_CREATE
static void OnCommand(HWND, int, HWND, UINT);                                      	// WM_COMMAND
static void OnPaint(HWND);                                                        	// WM_PAINT
static void OnDestroy(HWND);                                                        // WM_DESTROY

#define IDC_LIST                    1001
#define IDC_LIST_ADD                1002
#define IDC_LIST_DEL                1003
#define IDC_LIST_CLEAR              1004
#define IDC_       1005
//#define IDC_IGNORE_COMMENT_LINE     1006
//#define IDC_IGNORE_DUPLICATE_FILE   1007
//#define IDC_COMMENTS_SETTINGS       1008
#define IDC_RUN                     1009

static std::string g_sMainPath;

static HINSTANCE g_hInstance = NULL;
static HWND g_hWnd = NULL;

static HBRUSH g_hBackgroundBrush = NULL;
static HFONT g_hTitleFont = NULL;
static HFONT g_hDefaultFont = NULL;

/*
enum class STATUS {
    STATUS_EMPTY = 0,
    STATUS_WAIT = 1, // В ожидании.
    STATUS_PROCESSING = 2, // Обработка.
    STATUS_DONE = 3, // Завершено.
};

enum class RESULT {
    RESULT_EMPTY = 0,
    RESULT_DONE = 1, // Строк кода: N.
    RESULT_IGNORED = 2, // Проигнорирован: дубликат файла №N.
    RESULT_NOT_FOUND = 3 // Файл не найден.
};

struct FileData {
    TCHAR szFileName[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    uint32_t crc32;
    STATUS status;
    RESULT result;
    uint32_t lines;
};

static std::vector<FileData*> g_data;

static size_t g_AllLines = 0;
static size_t g_AllLinesNoFilters = 0;


// [ClearData]:
static void ClearData()
{
    if (g_data.size() > 0)
    {
        for (std::vector<FileData*>::const_iterator it = g_data.begin(); it != g_data.end(); it++)
        {
            delete (*it);
        }

        g_data.clear();
    }
}
// [/ClearData]


// [AddFile]:
static void AddFile(const String& sFile)
{
    if (sFile.empty())
    {
        MessageBox(g_hWnd, _T("Указано пустое имя файла."), _T("Ошибка"), MB_OK | MB_ICONERROR);
        return;
    }

    size_t pos = sFile.rfind('\\');
    if (pos == std::string::npos)
    {
        MessageBox(g_hWnd, _T("Указан неверный путь к файлу."), _T("Ошибка"), MB_OK | MB_ICONERROR);
        return;
    }

    String sFileName = sFile.substr(pos + 1, sFile.length() - pos - 1);
    String sPath = sFile.substr(0, pos);

    if (sFileName.length() >= MAX_PATH || sPath.length() >= MAX_PATH)
    {
        MessageBox(g_hWnd, _T("Имя файла или путь к нему превышает допустимую длину."), _T("Ошибка"), MB_OK | MB_ICONERROR);
        return;
    }

    HWND hListViewWnd = GetDlgItem(g_hWnd, IDC_LIST);
    if (!hListViewWnd)
    {
        return;
    }

    FileData* fd = new (std::nothrow) FileData;
    if (fd == nullptr)
    {
        MessageBox(g_hWnd, _T("Failed to allocate memory."), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    lstrcpy(fd->szFileName, sFileName.c_str());
    lstrcpy(fd->szPath, sPath.c_str());

    fd->crc32 = 0;
    fd->status = STATUS::STATUS_EMPTY;
    fd->result = RESULT::RESULT_EMPTY;
    fd->lines = 0;

    g_data.push_back(fd);

    LV_ITEM lvi;
    memset(&lvi, 0, sizeof(LV_ITEM));

    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(hListViewWnd);

    ListView_InsertItem(hListViewWnd, &lvi);
    ListView_SetItemText(hListViewWnd, lvi.iItem, 1, (LPSTR)sFileName.c_str());
    ListView_SetItemText(hListViewWnd, lvi.iItem, 2, (LPSTR)sPath.c_str());
}
// [/AddFile]


// [SetStatus]:
static void SetStatus(HWND hListViewWnd, int iItem, STATUS status)
{
    String sStr;

    switch (status)
    {
    case STATUS::STATUS_WAIT:
    {
        sStr = _T("В ожидании");
    }
    break;

    case STATUS::STATUS_PROCESSING:
    {
        sStr = _T("Обработка");
    }
    break;

    case STATUS::STATUS_DONE:
    {
        sStr = _T("Завершено");
    }
    break;
    }

    if (iItem < (int)g_data.size())
    {
        g_data[iItem]->status = status;
    }

    ListView_SetItemText(hListViewWnd, iItem, 4, (LPSTR)sStr.c_str());
}
// [/SetStatus]


// [SetResult]:
static void SetResult(HWND hListViewWnd, int iItem, RESULT result, const String& sParam = _T(""))
{
    String sStr;

    switch (result)
    {
    case RESULT::RESULT_DONE:
    {
        sStr = _T("Строк кода: ");
        sStr += sParam;
    }
    break;

    case RESULT::RESULT_IGNORED:
    {
        sStr = _T("Проигнорирован: дубликат файла №");
        sStr += sParam;
    }
    break;

    case RESULT::RESULT_NOT_FOUND:
    {
        sStr = _T("Файл не найден");
    }
    break;
    }

    if (iItem < (int)g_data.size())
    {
        g_data[iItem]->result = result;
    }

    ListView_SetItemText(hListViewWnd, iItem, 5, (LPSTR)sStr.c_str());
}
// [/SetResult]


// [Thread0]:
static DWORD WINAPI Thread0(LPVOID lpArgument)
{
    HWND hWnd = static_cast<HWND>(lpArgument);

    HWND hListViewWnd = GetDlgItem(hWnd, IDC_LIST);

    SetClassLongPtr(hWnd, GCL_STYLE, GetClassLongPtr(hWnd, GCL_STYLE) | CS_NOCLOSE);

    int iItem = -1;
    for (std::vector<FileData*>::iterator it = g_data.begin(); it != g_data.end(); it++)
    {
        iItem++;

        SetStatus(hListViewWnd, iItem, STATUS::STATUS_WAIT);
        SetResult(hListViewWnd, iItem, RESULT::RESULT_EMPTY);
    }


    // Настройки.
    bool bIgnoreEmptyLines = Button_GetCheck(GetDlgItem(hWnd, IDC_IGNORE_EMPTY_LINE));
    bool bIgnoreCommentLine = Button_GetCheck(GetDlgItem(hWnd, IDC_IGNORE_COMMENT_LINE));
    bool bIgnoreDuplicateFile = Button_GetCheck(GetDlgItem(hWnd, IDC_IGNORE_DUPLICATE_FILE));


    g_AllLines = 0;
    g_AllLinesNoFilters = 0;

    iItem = -1;


    // Перебираем всі файлі зі списку.

    String sCurrentFileName;

    for (std::vector<FileData*>::iterator CurrentFileIterator = g_data.begin(); CurrentFileIterator != g_data.end(); CurrentFileIterator++)
    {
        iItem++;

        sCurrentFileName = BuildPath((*CurrentFileIterator)->szPath, (*CurrentFileIterator)->szFileName);

        SetStatus(hListViewWnd, iItem, STATUS::STATUS_PROCESSING);

        if (!FileExists(sCurrentFileName))
        {
            SetStatus(hListViewWnd, iItem, STATUS::STATUS_DONE);
            SetResult(hListViewWnd, iItem, RESULT::RESULT_NOT_FOUND);

            continue;
        }


        BYTE* lpBuffer = nullptr;
        DWORD dwFileSize = 0;

        HANDLE hFile = CreateFile(sCurrentFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            dwFileSize = GetFileSize(hFile, NULL);
            if (dwFileSize > 16 * 1024 * 1024) // Max File Size == 16 MB.
            {
                MessageBox(hWnd, _T("Слишком большой размер файла."), _T("Ошибка!"), MB_OK | MB_ICONERROR | MB_TOPMOST);

                CloseHandle(hFile);
                continue;
            }

            if (dwFileSize == 0)
            {

            }

            lpBuffer = new (std::nothrow) BYTE[dwFileSize + 1];
            if (lpBuffer == nullptr)
            {
                MessageBox(hWnd, _T("Failed to allocate memory."), _T("Error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);

                CloseHandle(hFile);
                continue;
            }

            DWORD dwReaded;
            if (!ReadFile(hFile, lpBuffer, dwFileSize, &dwReaded, NULL))
            {
                MessageBox(hWnd, _T("Failed to read data."), _T("Error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);

                delete[] lpBuffer;
                CloseHandle(hFile);
                continue;
            }

            (*CurrentFileIterator)->crc32 = RtlComputeCrc32(0, lpBuffer, dwFileSize);

            StringStream ss;
            ss << std::uppercase << std::hex << (*CurrentFileIterator)->crc32;
            String sTmp = ss.str();

            ListView_SetItemText(GetDlgItem(hWnd, IDC_LIST), iItem, 3, (LPSTR)sTmp.c_str());

            CloseHandle(hFile);
        }
        else
        {
            SetStatus(hListViewWnd, iItem, STATUS::STATUS_DONE);
            SetResult(hListViewWnd, iItem, RESULT::RESULT_NOT_FOUND);

            continue;
        }

        if (bIgnoreDuplicateFile)
        {
            bool bDuplicateFound = false;
            int iIndex = -1;
            for (std::vector<FileData*>::const_iterator it = g_data.begin(); it != g_data.end(); it++)
            {
                iIndex++;

                if ((*CurrentFileIterator)->crc32 == (*it)->crc32 && CurrentFileIterator > it)
                {
                    bDuplicateFound = true;
                    break;
                }
            }

            if (bDuplicateFound)
            {
                SetStatus(hListViewWnd, iItem, STATUS::STATUS_DONE);
                SetResult(hListViewWnd, iItem, RESULT::RESULT_IGNORED, ToString(iIndex + 1));

                if (lpBuffer)
                {
                    delete[] lpBuffer;
                }

                continue;
            }
        }



        // Фильтрируем коментарии.

        std::vector<CommentInfo*> comments;

        const std::string sCurrentFileExtension = GetFileExtension(sCurrentFileName);
        if (sCurrentFileExtension.length() > 0)
        {
            const std::vector<CommentInfo*> all_comments = Comments_GetAllComments();
            if (all_comments.size() > 0)
            {
                std::string sExtension;

                for (std::vector<CommentInfo*>::const_iterator CommentIterator = all_comments.begin(); CommentIterator != all_comments.end(); CommentIterator++)
                {
                    if (lstrlen((*CommentIterator)->szFileExtension) > 0)
                    {
                        sExtension = (*CommentIterator)->szFileExtension;

                        if ((*CommentIterator)->szFileExtension[0] != '.')
                        {
                            sExtension.insert(sExtension.begin(), '.');
                        }

                        if (sCurrentFileExtension == sExtension)
                        {
                            comments.push_back((*CommentIterator));
                        }
                    }
                }
            }
        }



        std::string sFileData((char*)lpBuffer, dwFileSize);

        // Удаляем символы возврата каретки.
        sFileData.erase(std::remove(sFileData.begin(), sFileData.end(), '\r'), sFileData.end());

        // Заменяем табуляцию пробелами.
        std::replace(sFileData.begin(), sFileData.end(), '\t', ' ');

        // Удаляем все пробелы.
        sFileData.erase(std::remove(sFileData.begin(), sFileData.end(), ' '), sFileData.end());



        // Если задействовано правило для игнорирования строк содержащих лишь комментарий.
        if (bIgnoreCommentLine)
        {
            std::string sComments;

            enum class SEARCH_STRING_QUEUE {
                COMMENT_BEGIN = 1,
                COMMENT_END = 2
            };

            SEARCH_STRING_QUEUE SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_BEGIN;

            std::string sSearchString;

            size_t CurrentPos = 0;

            size_t CommentBeginPos = 0;
            size_t CommentEndPos = 0;

            std::string sComment;


            // Сначала удаляем комментарии, которые могут занимать несколько строк.
            for (std::vector<CommentInfo*>::const_iterator CurrentCommentIterator = comments.begin(); CurrentCommentIterator != comments.end(); CurrentCommentIterator++)
            {
                if ((*CurrentCommentIterator)->bEndCommentAtEndLine == true)
                {
                    continue;
                }


                SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_BEGIN;

                sSearchString = (*CurrentCommentIterator)->szBeginComment;

                CurrentPos = 0;

                CommentBeginPos = 0;
                CommentEndPos = 0;

                sComment.clear();


                while ((CurrentPos = sFileData.find(sSearchString, CurrentPos)) != std::string::npos)
                {
                    switch (SearchStringQueue)
                    {
                    case SEARCH_STRING_QUEUE::COMMENT_BEGIN:
                    {
                        SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_END;

                        CommentBeginPos = CurrentPos;

                        CurrentPos += sSearchString.length();

                        sSearchString = (*CurrentCommentIterator)->szEndComment;
                    }
                    break;

                    case SEARCH_STRING_QUEUE::COMMENT_END:
                    {
                        SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_BEGIN;

                        CommentEndPos = CurrentPos + lstrlen((*CurrentCommentIterator)->szEndComment);

                        CurrentPos += sSearchString.length();

                        sSearchString = (*CurrentCommentIterator)->szBeginComment;
                    }
                    break;
                    }


                    if (CommentEndPos != 0)
                    {
                        sComment = sFileData.substr(CommentBeginPos, CommentEndPos - CommentBeginPos);

                        bool b = false;

                        if (std::count(sComment.begin(), sComment.end(), '\n') > 0)
                        {
                            size_t pos1 = sFileData.rfind('\n', CommentBeginPos);
                            size_t pos2 = sFileData.find('\n', CommentEndPos);

                            if (sFileData.substr(pos1 + 1, CommentBeginPos - pos1 - 1).length() > 0 &&
                                sFileData.substr(CommentEndPos, pos2 - CommentEndPos).length() > 0)
                            {
                                b = true;
                            }
                        }

                        sFileData.erase(CommentBeginPos, CommentEndPos - CommentBeginPos);

                        if (b)
                        {
                            sFileData.insert(sFileData.begin() + CommentBeginPos, '\n');
                        }

                        CurrentPos -= CommentEndPos - CommentBeginPos;

                        CommentBeginPos = 0;
                        CommentEndPos = 0;

                        sComments += sComment + "\n";

                        sComment.clear();
                    }
                }
            }


            // Удаляем комментарии, которые заканчиваются в конце строки.
            for (std::vector<CommentInfo*>::const_iterator CurrentCommentIterator = comments.begin(); CurrentCommentIterator != comments.end(); CurrentCommentIterator++)
            {
                if ((*CurrentCommentIterator)->bEndCommentAtEndLine == false)
                {
                    continue;
                }


                SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_BEGIN;

                sSearchString = (*CurrentCommentIterator)->szBeginComment;

                CurrentPos = 0;

                CommentBeginPos = 0;
                CommentEndPos = 0;

                sComment.clear();


                while ((CurrentPos = sFileData.find(sSearchString, CurrentPos)) != std::string::npos)
                {
                    switch (SearchStringQueue)
                    {
                    case SEARCH_STRING_QUEUE::COMMENT_BEGIN:
                    {
                        SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_END;

                        CommentBeginPos = CurrentPos;

                        CurrentPos += sSearchString.length();

                        sSearchString = "\n";
                    }
                    break;

                    case SEARCH_STRING_QUEUE::COMMENT_END:
                    {
                        SearchStringQueue = SEARCH_STRING_QUEUE::COMMENT_BEGIN;

                        CommentEndPos = CurrentPos;

                        CurrentPos += sSearchString.length();

                        sSearchString = (*CurrentCommentIterator)->szBeginComment;
                    }
                    break;
                    }


                    if (CommentEndPos != 0)
                    {
                        if (CommentBeginPos > 0)
                        {
                            if (sFileData[CommentBeginPos - 1] == '\n')
                            {
                                //CommentBeginPos--;
                            }
                        }

                        sComment = sFileData.substr(CommentBeginPos, CommentEndPos - CommentBeginPos);

                        sFileData.erase(CommentBeginPos, CommentEndPos - CommentBeginPos);

                        CurrentPos -= CommentEndPos - CommentBeginPos;

                        CommentBeginPos = 0;
                        CommentEndPos = 0;

                        sComments += sComment + "\n";

                        sComment.clear();
                    }
                }
            }


            MessageBox(hWnd, sComments.c_str(), "", MB_OK);
        }



        // TODO: удалять символы перевода на новую строку, если в строке был лишь комментарий.



        // Подсчитать количество строк (учитывая только фильтр для игнорирования пустых строк).

        //*
        int iLinesCounter = 0;// 1;

        char cPrev = 0;
        for (size_t i = 0; i < sFileData.length(); i++)
        {
            if (sFileData[i] == '\r')
            {
                continue;
            }

            if (sFileData[i] == '\n')
            {
                if (cPrev == '\n' && bIgnoreEmptyLines)
                {
                    continue;
                }

                iLinesCounter++;
            }

            cPrev = sFileData[i];
        }
        //*



        SetStatus(hListViewWnd, iItem, STATUS::STATUS_DONE);
        SetResult(hListViewWnd, iItem, RESULT::RESULT_DONE, ToString(iLinesCounter));

        g_AllLines += iLinesCounter;
        g_AllLinesNoFilters += std::count(lpBuffer, lpBuffer + dwFileSize, '\n');

        if (lpBuffer)
        {
            delete[] lpBuffer;
        }

        Sleep(10);
    }

    SetClassLongPtr(hWnd, GCL_STYLE, GetClassLongPtr(hWnd, GCL_STYLE) & ~CS_NOCLOSE);

    InvalidateRect(hWnd, NULL, FALSE);

    ExitThread(0);
}
// [/Thread0]
//*/

// [WindowProcedure]: 
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hWnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);

    case WM_CTLCOLORSTATIC:
    {
        //if ((HWND)lParam == GetDlgItem(hWnd, IDC_IGNORE_EMPTY_LINE) ||
        //    (HWND)lParam == GetDlgItem(hWnd, IDC_IGNORE_COMMENT_LINE) ||
        //    (HWND)lParam == GetDlgItem(hWnd, IDC_IGNORE_DUPLICATE_FILE))
        //{
        //    SetBkMode((HDC)wParam, TRANSPARENT);
        //    return (LRESULT)g_hBackgroundBrush;
        //}
    }
    break;

    case WM_NOTIFY:
    {
        LPNMHDR lpNMHDR = reinterpret_cast<LPNMHDR>(lParam);
        if (lpNMHDR)
        {
            if (lpNMHDR->code == NM_CUSTOMDRAW && lpNMHDR->idFrom == IDC_LIST)
            {
                LPNMLVCUSTOMDRAW lpNMLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
                if (lpNMLVCD)
                {
                    switch (lpNMLVCD->nmcd.dwDrawStage)
                    {
                    case CDDS_PREPAINT:
                    {
                        return CDRF_NOTIFYITEMDRAW;
                    }
                    break;

                    case CDDS_ITEMPREPAINT:
                    {
                        lpNMLVCD->nmcd.uItemState &= ~CDIS_SELECTED;
                        if (ListView_GetItemState(lpNMLVCD->nmcd.hdr.hwndFrom, lpNMLVCD->nmcd.dwItemSpec, LVIS_SELECTED))
                        {
                            lpNMLVCD->clrTextBk = RGB(220, 220, 210);
                        }
                        return CDRF_NOTIFYSUBITEMDRAW;
                    }
                    break;

                    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                    {
                        switch (lpNMLVCD->iSubItem)
                        {
                        case 0:
                        {
                            TCHAR szText[12] = { 0 };
                            wsprintf(szText, _T("%d"), lpNMLVCD->nmcd.dwItemSpec + 1);

                            SIZE size;
                            GetTextExtentPoint32(lpNMLVCD->nmcd.hdc, szText, lstrlen(szText), &size);
                            TextOut(
                                lpNMLVCD->nmcd.hdc,
                                lpNMLVCD->nmcd.rc.left,
                                lpNMLVCD->nmcd.rc.top + ((lpNMLVCD->nmcd.rc.bottom - lpNMLVCD->nmcd.rc.top) >> 1) - (size.cy >> 1),
                                szText,
                                lstrlen(szText)
                            );

                            return CDRF_SKIPDEFAULT;
                        }
                        break;

                        case 4:
                        {
                            /*
                            switch (g_data[lpNMLVCD->nmcd.dwItemSpec]->status)
                            {
                            case STATUS::STATUS_EMPTY:
                                break;

                            case STATUS::STATUS_WAIT:
                                break;

                            case STATUS::STATUS_PROCESSING:
                                break;

                            case STATUS::STATUS_DONE:
                                break;
                            }
                            //*/
                        }
                        break;

                        case 5:
                        {
                            /*
                            switch (g_data[lpNMLVCD->nmcd.dwItemSpec]->result)
                            {
                            case RESULT::RESULT_EMPTY:
                                break;

                            case RESULT::RESULT_DONE:
                                lpNMLVCD->clrText = RGB(0, 150, 0);
                                break;

                            case RESULT::RESULT_IGNORED:
                                lpNMLVCD->clrText = RGB(150, 150, 0);
                                break;

                            case RESULT::RESULT_NOT_FOUND:
                                lpNMLVCD->clrText = RGB(150, 0, 0);
                                break;
                            }
                            //*/
                        }
                        break;

                        default:
                        {
                            lpNMLVCD->clrText = RGB(0, 0, 0);
                        }
                        break;
                        }

                        return CDRF_NEWFONT;
                    }
                    break;

                    default:
                        return CDRF_DODEFAULT;
                    }
                }
            }
        }
    }
    break;

    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
// [/WindowProcedure]


// [OnCreate]: WM_CREATE
static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
    INITCOMMONCONTROLSEX icсex = { 0 };
    icсex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icсex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icсex);

    //g_sMainPath = GetThisPath(lpcs->hInstance);

    g_hInstance = lpcs->hInstance;
    g_hWnd = hWnd;


    RECT rc;
    GetClientRect(hWnd, &rc);
    const int iWindowWidth = rc.right - rc.left;
    const int iWindowHeight = rc.bottom - rc.top;


    g_hBackgroundBrush = CreateSolidBrush(RGB(210, 220, 220));

    g_hTitleFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Arial"));

    g_hDefaultFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Arial"));


    HWND hListViewWnd = CreateWindowEx(0, WC_LISTVIEW, _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | LVS_REPORT,
        10, 40, iWindowWidth - 20, 220, hWnd, (HMENU)IDC_LIST, lpcs->hInstance, NULL);
    SendMessage(GetDlgItem(hWnd, IDC_LIST), WM_SETFONT, (WPARAM)g_hDefaultFont, 0L);

    ListView_SetExtendedListViewStyle(hListViewWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMN lvc;
    memset(&lvc, 0, sizeof(LVCOLUMN));

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;

    lvc.cx = 30;
    lvc.pszText = (LPSTR)_T("№");
    ListView_InsertColumn(hListViewWnd, 0, &lvc);

    lvc.cx = 150;
    lvc.pszText = (LPSTR)_T("Файл");
    ListView_InsertColumn(hListViewWnd, 1, &lvc);

    lvc.cx = 200;
    lvc.pszText = (LPSTR)_T("Путь к файлу");
    ListView_InsertColumn(hListViewWnd, 2, &lvc);

    lvc.cx = 70;
    lvc.pszText = (LPSTR)_T("CRC32");
    ListView_InsertColumn(hListViewWnd, 3, &lvc);

    lvc.cx = 110;
    lvc.pszText = (LPSTR)_T("Статус");
    ListView_InsertColumn(hListViewWnd, 4, &lvc);

    lvc.cx = 240;
    lvc.pszText = (LPSTR)_T("Результат");
    ListView_InsertColumn(hListViewWnd, 5, &lvc);


    CreateWindowEx(0, _T("button"), _T(""), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP,
        iWindowWidth - 102, 8, 24, 24, hWnd, (HMENU)IDC_LIST_ADD, lpcs->hInstance, NULL);
    CreateToolTip(lpcs->hInstance, GetDlgItem(hWnd, IDC_LIST_ADD), _T("Добавить"));
    SendMessage(GetDlgItem(hWnd, IDC_LIST_ADD), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_LIST_ADD)));


    CreateWindowEx(0, _T("button"), _T(""), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP,
        iWindowWidth - 68, 8, 24, 24, hWnd, (HMENU)IDC_LIST_DEL, lpcs->hInstance, NULL);
    CreateToolTip(lpcs->hInstance, GetDlgItem(hWnd, IDC_LIST_DEL), _T("Удалить"));
    SendMessage(GetDlgItem(hWnd, IDC_LIST_DEL), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_LIST_DEL)));


    CreateWindowEx(0, _T("button"), _T(""), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP,
        iWindowWidth - 34, 8, 24, 24, hWnd, (HMENU)IDC_LIST_CLEAR, lpcs->hInstance, NULL);
    CreateToolTip(lpcs->hInstance, GetDlgItem(hWnd, IDC_LIST_CLEAR), _T("Очистить"));
    SendMessage(GetDlgItem(hWnd, IDC_LIST_CLEAR), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_LIST_CLEAR)));


    CreateWindowEx(0, _T("button"), _T("Игнорировать пустые строки"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, 350, 330/*iWindowWidth - 20*/, 20, hWnd, (HMENU)IDC_IGNORE_EMPTY_LINE, lpcs->hInstance, NULL);
    SendMessage(GetDlgItem(hWnd, IDC_IGNORE_EMPTY_LINE), WM_SETFONT, (WPARAM)g_hDefaultFont, 0L);
    Button_SetCheck(GetDlgItem(hWnd, IDC_IGNORE_EMPTY_LINE), TRUE);

    //CreateWindowEx(0, _T("button"), _T("Игнорировать строки содержащие лишь комментарий"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
    //    10, 380, 330/*iWindowWidth - 50*/, 20, hWnd, (HMENU)IDC_IGNORE_COMMENT_LINE, lpcs->hInstance, NULL);
    //SendMessage(GetDlgItem(hWnd, IDC_IGNORE_COMMENT_LINE), WM_SETFONT, (WPARAM)g_hDefaultFont, 0L);
    //Button_SetCheck(GetDlgItem(hWnd, IDC_IGNORE_COMMENT_LINE), TRUE);
    //CreateToolTip(lpcs->hInstance, GetDlgItem(hWnd, IDC_IGNORE_COMMENT_LINE),
    //    _T("Нажмите \"Настроить\" для редактирования списка комментариев языков программирования"));

    //CreateWindowEx(0, _T("button"), _T("Игнорировать файлы с одинаковым содержимым"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
    //    10, 410, 330/*iWindowWidth - 20*/, 20, hWnd, (HMENU)IDC_IGNORE_DUPLICATE_FILE, lpcs->hInstance, NULL);
    //SendMessage(GetDlgItem(hWnd, IDC_IGNORE_DUPLICATE_FILE), WM_SETFONT, (WPARAM)g_hDefaultFont, 0L);
    //Button_SetCheck(GetDlgItem(hWnd, IDC_IGNORE_DUPLICATE_FILE), TRUE);
    //CreateToolTip(lpcs->hInstance, GetDlgItem(hWnd, IDC_IGNORE_DUPLICATE_FILE), _T("Сравнение по контрольной сумме."));


    //CreateWindowEx(0, _T("button"), _T(""), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP,
    //    350/*iWindowWidth - 34*/, 378, 24, 24, hWnd, (HMENU)IDC_COMMENTS_SETTINGS, lpcs->hInstance, NULL);
    //CreateToolTip(lpcs->hInstance, GetDlgItem(hWnd, IDC_COMMENTS_SETTINGS), _T("Настроить"));
    //SendMessage(GetDlgItem(hWnd, IDC_COMMENTS_SETTINGS), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_COMMENTS_SETTINGS)));


    CreateWindowEx(0, _T("button"), _T("Запуск"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        (iWindowWidth >> 1) - 75, iWindowHeight - 35, 150, 25, hWnd, (HMENU)IDC_RUN, lpcs->hInstance, NULL);
    SendMessage(GetDlgItem(hWnd, IDC_RUN), WM_SETFONT, (WPARAM)g_hDefaultFont, 0L);

    return TRUE;
}
// [/OnCreate]


// [OnCommand]: WM_COMMAND
static void OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_LIST_ADD:
    {
        TCHAR szFileName[1024] = { 0 };
        memset(szFileName, 0, sizeof(szFileName));

        OPENFILENAME ofn;
        memset(&ofn, 0, sizeof(OPENFILENAME));

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = sizeof(szFileName);
        ofn.lpstrFilter = _T("Все файлы (*.*)\0*.*\0\0");
        ofn.nFilterIndex = 0;
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY;

        if (GetOpenFileName(&ofn))
        {
            if (ofn.nFileExtension == 0)
            {
                //String sPath = ofn.lpstrFile;

                TCHAR* pStr = ofn.lpstrFile + ofn.nFileOffset;
                do {
                    //AddFile(BuildPath(sPath, pStr));
                    pStr += lstrlen(pStr) + 1;
                } while (*pStr != '\0');
            }
            else
            {
                //AddFile(ofn.lpstrFile);
            }
        }
    }
    break;

    case IDC_LIST_DEL:
    {
        HWND hListViewWnd = GetDlgItem(hWnd, IDC_LIST);

        int i = -1;
        while ((i = ListView_GetNextItem(hListViewWnd, -1, LVNI_SELECTED)) != -1)
        {
            ListView_DeleteItem(hListViewWnd, i);

            //delete (*(g_data.begin() + i));

            //g_data.erase(g_data.begin() + i);
        }
    }
    break;

    case IDC_LIST_CLEAR:
    {
        ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LIST));

        //ClearData();
    }
    break;

    //case IDC_COMMENTS_SETTINGS:
    //{
    //    //DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_COMMENTS_SETTINGS), hWnd, (DLGPROC)CommentsSettings_DialogProcedure);
    //}
    //break;

    case IDC_RUN:
    {
        //if (g_data.size() > 0)
        //{
        //    DWORD dwThreadID;
        //    HANDLE hThread = CreateThread(NULL, 0, Thread0, (LPVOID)hWnd, 0, &dwThreadID);
        //    if (hThread)
        //    {
        //        CloseHandle(hThread);
        //    }
        //}
    }
    break;
    }
}
// [/OnCommand]


// [OnPaint]: WM_PAINT
static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);
    RECT rc;
    GetClientRect(hWnd, &rc);
    const int iWindowWidth = rc.right - rc.left;
    const int iWindowHeight = rc.bottom - rc.top;
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hDC, iWindowWidth, iWindowHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

    FillRect(hMemDC, &ps.rcPaint, g_hBackgroundBrush);

    int iOldBkMode = SetBkMode(hMemDC, TRANSPARENT);
    HFONT hOldFont = (HFONT)GetCurrentObject(hMemDC, OBJ_FONT);

    TCHAR szText[1024] = { 0 };
    SIZE size;

    SetTextColor(hMemDC, RGB(0, 0, 0));

    SelectObject(hMemDC, g_hTitleFont);

    lstrcpy(szText, _T("Файлы проекта"));
    TextOut(hMemDC, 10, 10, szText, lstrlen(szText));

    DrawLine(hMemDC, 10, 320, iWindowWidth - 10, 320);

    lstrcpy(szText, _T("Фильтры"));
    TextOut(hMemDC, 10, 330, szText, lstrlen(szText));

    DrawLine(hMemDC, 10, 440, iWindowWidth - 10, 440);


    SelectObject(hMemDC, g_hDefaultFont);

    //wsprintf(szText, _T("Общее количество строк кода: %d"), g_AllLines);
    //TextOut(hMemDC, 10, 270, szText, lstrlen(szText));

    //wsprintf(szText, _T("Общее количество строк кода (без фильтров): %d"), g_AllLinesNoFilters);
    //TextOut(hMemDC, 10, 290, szText, lstrlen(szText));


    lstrcpy(szText, _T("Copyright \251 fastb1t, 2026"));
    GetTextExtentPoint32(hMemDC, szText, lstrlen(szText), &size);
    TextOut(hMemDC, iWindowWidth - size.cx - 10, iWindowHeight - size.cy - 10, szText, lstrlen(szText));


    SelectObject(hMemDC, hOldFont);
    SetBkMode(hMemDC, iOldBkMode);

    BitBlt(hDC, 0, 0, iWindowWidth, iWindowHeight, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    EndPaint(hWnd, &ps);
}
// [/OnPaint]


// [OnDestroy]: WM_DESTROY
static void OnDestroy(HWND hWnd)
{
    DeleteObject(g_hBackgroundBrush);
    DeleteObject(g_hTitleFont);
    DeleteObject(g_hDefaultFont);

    PostQuitMessage(0);
}
// [/OnDestroy]
