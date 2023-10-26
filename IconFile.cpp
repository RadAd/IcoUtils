#include "IconFile.h"

#include "Utils.h"
#include <tchar.h>

#define VALIDATE(x) if (!(x)) { valid = false; _ftprintf(stderr, TEXT("Invalid: %s\n"), TEXT(#x)); }
#define VALIDATE_OP(x, op, y) if (!((x) op (y))) { valid = false; _ftprintf(stderr, TEXT("Invalid: %s %s %s -> %d %s %d\n"), TEXT(#x), TEXT(#op), TEXT(#y), (int) (x), TEXT(#op), (int) (y)); }

#pragma pack(push, 2)
struct ICONDIRRES
{
    BYTE  bWidth;
    BYTE  bHeight;
    BYTE  bColorCount;
    BYTE  bReserved;
    WORD  wPlanes;
    WORD  wBitCount;
    DWORD dwBytesInRes;
    WORD  nId;
};
#pragma pack(pop)

IconFile IconFile::Load(LPCTSTR lpFilename, bool bIgnoreValidatePng)
{
    const HANDLE hFile = CreateFile(lpFilename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, NULL);
    CHECK(hFile != INVALID_HANDLE_VALUE);

    try
    {
        IconFile IconData;

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

        IconData.Validate(bIgnoreValidatePng);
        return IconData;
    }
    catch (...)
    {
        CloseHandle(hFile);
        throw;
    }
}

IconFile IconFile::FromResource(LPCTSTR strModule, int index, bool bIgnoreValidatePng)
{
    HMODULE hModule = LoadLibraryEx(strModule, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    CHECK(hModule);

    try
    {
        const IconFile IconData = IconFile::FromResource(hModule, index, bIgnoreValidatePng);

        FreeLibrary(hModule);

        return IconData;
    }
    catch (...)
    {
        FreeLibrary(hModule);
        throw;
    }
}

IconFile IconFile::FromResource(HMODULE hModule, int index, bool bIgnoreValidatePng)
{
    HRSRC hResInfo = FindResourceEx(hModule, RT_GROUP_ICON, MAKEINTRESOURCE(index), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)); // TODO Specify lang
    CHECK(hResInfo);

    const DWORD sz = SizeofResource(hModule, hResInfo);

    HGLOBAL hRes = LoadResource(hModule, hResInfo);
    CHECK(hRes);

    IconFile IconData;

    const ICONHEADER* pIconHeader = (ICONHEADER*) LockResource(hRes);
    memcpy(&IconData.Header, pIconHeader, sizeof(ICONHEADER));
    IconData.entry.resize(IconData.Header.idCount);

    bool valid = true;
    VALIDATE_OP(sz, ==, (sizeof(ICONHEADER) + IconData.Header.idCount * sizeof(ICONDIRRES)));

    const ICONDIRRES* pIconDirResArray = (ICONDIRRES*) (pIconHeader + 1);
    DWORD dwImageOffset = sizeof(ICONHEADER) + static_cast<DWORD>(IconData.entry.size()) * sizeof(ICONDIR);
    for (int i = 0; i < IconData.Header.idCount; ++i)
    {
        Entry& entry = IconData.entry[i];

        // Resource ICONDIR is one WORD shorter than file ICONDIR
        const ICONDIRRES* pIconDir = &pIconDirResArray[i];
        memcpy(&entry.dir, pIconDir, sizeof(ICONDIRRES));
        entry.dir.dwImageOffset = dwImageOffset;

        entry.DataFromResource(hModule, pIconDir->nId);

        dwImageOffset += entry.dir.dwBytesInRes;
    }

    IconData.Validate(bIgnoreValidatePng);
    return IconData;
}

IconFile::~IconFile()
{
}

void IconFile::Validate(bool bIgnorePng) const
{
    bool valid = true;
    VALIDATE_OP(Header.idReserved, ==, 0);
    VALIDATE(GetType() == TYPE_ICON || GetType() == TYPE_CURSOR);
    VALIDATE_OP(Header.idCount, ==, entry.size());
    DWORD dwImageOffset = sizeof(ICONHEADER) + static_cast<DWORD>(entry.size()) * sizeof(ICONDIR);
    for (const Entry& entry : entry)
    {
        if (!entry.IsPNG())
        {
            const BITMAPINFOHEADER* header = entry.GetBITMAPINFOHEADER();
            VALIDATE_OP(header->biPlanes, ==, 1);

            VALIDATE_OP(header->biWidth, ==, entry.dir.bWidth);
            VALIDATE_OP(header->biHeight, ==, entry.dir.bHeight * 2);
            VALIDATE_OP(header->biBitCount, ==, entry.dir.wBitCount);

            VALIDATE_OP(entry.dir.dwImageOffset, ==, dwImageOffset);

            const DWORD dwBytesInXOR = entry.GetBytesPerLineXOR() * header->biHeight / 2;
            const DWORD dwBytesInAND = entry.GetBytesPerLineAND() * header->biHeight / 2;

            VALIDATE_OP(sizeof(BITMAPINFOHEADER) + (entry.GetColorSize() * sizeof(RGBQUAD)) + dwBytesInXOR + dwBytesInAND, ==, entry.dir.dwBytesInRes);
        }
        else if (!bIgnorePng)
        {
            VALIDATE_OP(entry.dir.bWidth, ==, 0);
            VALIDATE_OP(entry.dir.bHeight, ==, 0);
            VALIDATE_OP(entry.dir.wBitCount, ==, 32);
        }
        dwImageOffset += entry.dir.dwBytesInRes;
    }
    if (!valid) throw Error(TEXT("Invalid icon"));
}

void IconFile::Save(LPCTSTR lpFilename, bool bIgnoreValidatePng) const
{
    Validate(bIgnoreValidatePng);

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

void IconFile::Entry::DataFromResource(HMODULE hModule, WORD nId)
{
    bool valid = true;

    HRSRC hResInfoIcon = FindResourceEx(hModule, RT_ICON, MAKEINTRESOURCE(nId), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)); // TODO Specify lang
    CHECK(hResInfoIcon);

    const DWORD sz = SizeofResource(hModule, hResInfoIcon);
    VALIDATE_OP(sz, ==, dir.dwBytesInRes);

    HGLOBAL hRes = LoadResource(hModule, hResInfoIcon);
    CHECK(hRes);

    const BYTE* pIconData = (BYTE*) LockResource(hRes);

    data.resize(dir.dwBytesInRes);
    memcpy(data.data(), pIconData, data.size());
}

bool IconFile::Entry::IsPNG() const
{
    return !data.empty() && ::IsPNG(data.data());
}
