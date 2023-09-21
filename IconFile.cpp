#include "IconFile.h"

#include "Utils.h"
#include <tchar.h>

IconFile IconFile::Load(LPCTSTR lpFilename)
{
    IconFile IconData;

    const HANDLE hFile = CreateFile(lpFilename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, NULL);
    CHECK(hFile != INVALID_HANDLE_VALUE);

    try
    {
        CheckReadFile(hFile, &IconData.Header, sizeof(ICONHEADER));

        IconData.entry.resize(IconData.Header.idCount);
        for (Entry& entry : IconData.entry)
        {
            CheckReadFile(hFile, &entry.dir, sizeof(ICONDIR));
        }

        for (Entry& entry : IconData.entry)
        {
            entry.LoadData(hFile);
        }

        CloseHandle(hFile);
    }
    catch (...)
    {
        CloseHandle(hFile);
        throw;
    }

    IconData.Validate();
    return IconData;
}

IconFile::~IconFile()
{
}

#define VALIDATE(x) if (!(x)) { valid = false; _ftprintf(stderr, TEXT("Invalid: %s\n"), TEXT(#x)); }

void IconFile::Validate() const
{
    bool valid = true;
    VALIDATE(Header.idReserved == 0);
    VALIDATE(GetType() == TYPE_ICON || GetType() == TYPE_CURSOR);
    VALIDATE(Header.idCount == entry.size());
    DWORD dwImageOffset = sizeof(ICONHEADER) + static_cast<DWORD>(entry.size()) * sizeof(ICONDIR);
    for (const Entry& entry : entry)
    {
        if (!entry.IsPNG())
        {
            const BITMAPINFOHEADER* header = entry.GetBITMAPINFOHEADER();
            VALIDATE(header->biPlanes == 1);

            VALIDATE(header->biWidth == entry.dir.bWidth);
            VALIDATE(header->biHeight == entry.dir.bHeight * 2);
            VALIDATE(header->biBitCount == entry.dir.wBitCount);

            VALIDATE(entry.dir.dwImageOffset == dwImageOffset);

            const DWORD dwBytesInXOR = entry.GetBytesPerLineXOR() * header->biHeight / 2;
            const DWORD dwBytesInAND = entry.GetBytesPerLineAND() * header->biHeight / 2;

            VALIDATE(sizeof(BITMAPINFOHEADER) + (entry.GetColorSize() * sizeof(RGBQUAD)) + dwBytesInXOR + dwBytesInAND == entry.dir.dwBytesInRes);
        }
        else
        {
            VALIDATE(entry.dir.bWidth == 0);
            VALIDATE(entry.dir.bHeight == 0);
            VALIDATE(entry.dir.wBitCount == 32);
        }
        dwImageOffset += entry.dir.dwBytesInRes;
    }
    if (!valid) throw Error(TEXT("Invalid icon"));
}

void IconFile::Save(LPCTSTR lpFilename) const
{
    Validate();

    const HANDLE hFile = CreateFile(lpFilename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, NULL);
    CHECK(hFile != INVALID_HANDLE_VALUE);

    try
    {
        CheckWriteFile(hFile, &Header, sizeof(ICONHEADER));

        for (const Entry& entry : entry)
        {
            CheckWriteFile(hFile, &entry.dir, sizeof(ICONDIR));
        }

        for (const Entry& entry : entry)
        {
            entry.SaveData(hFile);
        }

        CloseHandle(hFile);
    }
    catch (...)
    {
        CloseHandle(hFile);
        throw;
    }
}

void IconFile::Entry::LoadData(const HANDLE hFile)
{
    data.resize(dir.dwBytesInRes);

    const DWORD offset = SetFilePointer(hFile, 0, nullptr, FILE_CURRENT);
    if (offset != dir.dwImageOffset)
        SetFilePointer(hFile, dir.dwImageOffset, nullptr, FILE_BEGIN);

    CheckReadFile(hFile, data.data(), static_cast<DWORD>(data.size()));
}

void IconFile::Entry::SaveData(const HANDLE hFile) const
{
    DWORD offset = SetFilePointer(hFile, 0, nullptr, FILE_CURRENT);
    if (offset != dir.dwImageOffset)
        throw "Offset incorrect";

    CheckWriteFile(hFile, data.data(), static_cast<DWORD>(data.size()));
}

bool IconFile::Entry::IsPNG() const
{
    return ::IsPNG(data.data());
}
