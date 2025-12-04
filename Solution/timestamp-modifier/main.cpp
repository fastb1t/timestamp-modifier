#include <windows.h>
#include <iostream>

FILETIME MakeFileTime(int year, int month, int day, int hour, int minute, int second)
{
    SYSTEMTIME st = {};
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = hour;
    st.wMinute = minute;
    st.wSecond = second;

    FILETIME ftLocal, ftUTC;
    SystemTimeToFileTime(&st, &ftLocal);
    LocalFileTimeToFileTime(&ftLocal, &ftUTC);
    return ftUTC;
}

int main()
{
    const char* filename = "test.txt";

    HANDLE hFile = CreateFileA(
        filename,
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Не можу відкрити файл\n";
        return 1;
    }

    // Встановимо новий час
    FILETIME creationTime = MakeFileTime(2023, 1, 1, 12, 0, 0);
    FILETIME lastAccessTime = MakeFileTime(2023, 1, 2, 13, 30, 0);
    FILETIME lastWriteTime = MakeFileTime(2023, 1, 3, 15, 45, 0);

    if (!SetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) {
        std::cerr << "Помилка SetFileTime\n";
        CloseHandle(hFile);
        return 2;
    }

    CloseHandle(hFile);
    std::cout << "Дати успішно змінені!\n";
    return 0;
}
