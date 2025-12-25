#include <string>
#include <iostream>
#include <windows.h>

FILETIME MakeFileTime(int year, int month, int day, int hour, int minute, int second)
{
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));

    st.wYear    = year;
    st.wMonth   = month;
    st.wDay     = day;
    st.wHour    = hour;
    st.wMinute  = minute;
    st.wSecond  = second;

    FILETIME ftLocal, ftUTC;
    memset(&ftLocal, 0, sizeof(FILETIME));
    memset(&ftUTC, 0, sizeof(FILETIME));

    SystemTimeToFileTime(&st, &ftLocal);
    LocalFileTimeToFileTime(&ftLocal, &ftUTC);
    
    return ftUTC;
}

// Entry point.
int main(int argc, char* argv[])
{
    const std::string filename = "test.txt";

    HANDLE hFile = CreateFileA(
        filename.c_str(),
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD dwLastError = GetLastError();
        std::cerr << "Error! CreateFile() failed with error code: " << dwLastError << "\n";
        return 1;
    }


    FILETIME creationTime = MakeFileTime(2023, 1, 1, 12, 0, 0);
    FILETIME lastAccessTime = MakeFileTime(2023, 1, 2, 13, 30, 0);
    FILETIME lastWriteTime = MakeFileTime(2023, 1, 3, 15, 45, 0);

    if (!SetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) {
        DWORD dwLastError = GetLastError();
        std::cerr << "Error! SetFileTime() failed with error code: " << dwLastError << "\n";
        CloseHandle(hFile);
        return 2;
    }

    CloseHandle(hFile);
    
    std::cout << "Done!\n";

    return EXIT_SUCCESS;
}
