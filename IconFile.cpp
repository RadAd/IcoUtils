#include "IconFile.h"

#include "Utils.h"

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

void IconFile::Validate() const
{
    _ASSERTE(Header.idReserved == 0);
    _ASSERTE(GetType() == TYPE_ICON || GetType() == TYPE_CURSOR);
    _ASSERTE(Header.idCount == entry.size());
    DWORD dwImageOffset = sizeof(ICONHEADER) + static_cast<DWORD>(entry.size()) * sizeof(ICONDIR);
    for (const Entry& entry : entry)
    {
        if (!entry.IsPNG())
        {
            const BITMAPINFOHEADER* header = entry.GetBITMAPINFOHEADER();

            _ASSERTE(header->biWidth == entry.dir.bWidth);
            _ASSERTE(header->biHeight == entry.dir.bHeight * 2);
            _ASSERTE(header->biBitCount == entry.dir.wBitCount);

            _ASSERTE(entry.dir.dwImageOffset == dwImageOffset);

            const DWORD dwBytesInXOR = entry.GetBytesPerLineXOR() * header->biHeight / 2;
            const DWORD dwBytesInAND = entry.GetBytesPerLineAND() * header->biHeight / 2;

            _ASSERTE(sizeof(BITMAPINFOHEADER) + (entry.GetColorSize() * sizeof(RGBQUAD)) + dwBytesInXOR + dwBytesInAND == entry.dir.dwBytesInRes);
        }
        else
        {
            _ASSERTE(entry.dir.bWidth == 0);
            _ASSERTE(entry.dir.bHeight == 0);
            _ASSERTE(entry.dir.wBitCount == 32);
        }
        dwImageOffset += entry.dir.dwBytesInRes;
    }
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
