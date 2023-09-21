#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

#ifdef UNICODE
#define tstring wstring
#else
#define tstring string
#endif

class WinError
{
public:
    WinError(DWORD error = GetLastError())
        : m_error(error)
    {
    }

    DWORD GetError() const { return m_error; }

private:
    DWORD m_error;
};

class Error
{
public:
    Error(std::tstring msg)
        : m_msg(std::move(msg))
    {
    }

    const std::tstring& GetMsg() const { return m_msg; }

private:
    std::tstring m_msg;
};

#define CHECK(expr) if (!(expr)) throw WinError();

inline void CheckReadFile(
    _In_ HANDLE hFile,
    _Out_writes_bytes_(nNumberOfBytesToRead) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead)
{
    DWORD dwRead = 0;
    CHECK(ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, &dwRead, nullptr) && dwRead == nNumberOfBytesToRead);
}

inline void CheckWriteFile(
    _In_ HANDLE hFile,
    _In_reads_bytes_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite)
{
    DWORD dwWrite = 0;
    CHECK(WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, &dwWrite, nullptr) && dwWrite == nNumberOfBytesToWrite);
}
