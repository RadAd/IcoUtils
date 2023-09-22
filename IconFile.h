#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

enum IconType { TYPE_NONE, TYPE_ICON, TYPE_CURSOR };

struct ICONHEADER
{
    WORD idReserved; // must be 0
    WORD idType; // 1 = ICON, 2 = CURSOR
    WORD idCount;
    // ICONDIR [idCount]
    // ICONIMAGE [idCount]
};

struct ICONDIR
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes; // for cursors, this field = wXHotSpot
    WORD wBitCount; // for cursors, this field = wYHotSpot
    DWORD dwBytesInRes;
    DWORD dwImageOffset; // file-offset to the start of ICONIMAGE
};

#if 0
struct ICONIMAGE
{
    BITMAPINFOHEADER biHeader; // header for color bitmap (no mask header)
    //RGBQUAD rgbColors[biHeader.bColorCount];
    //BYTE bXOR[]; // DIB bits for color bitmap
    //BYTE bAND[]; // DIB bits for mask bitmap
};
#endif

inline bool IsPNG(LPCVOID pImage)
{
    const LPCSTR PNG = "\x89PNG\r\n\x1A\n";
    return strcmp(PNG, reinterpret_cast<LPCSTR>(pImage)) == 0;
}

inline DWORD pad32(DWORD b)
{
    return (b % 32 != 0) ? (b / 32 + 1) * 32 : b;
}

class IconFile
{
public:
    static IconFile Load(LPCTSTR lpFilename, bool bIgnoreValidatePng);

    IconFile()
        : Header()
    {
    }
    ~IconFile();

    IconType GetType() const { return static_cast<IconType>(Header.idType); }

    void Validate(bool bIgnorePng) const;
    void Save(LPCTSTR lpFilename, bool bIgnoreValidatePng) const;

    class Entry
    {
    public:
        ICONDIR dir;

        void LoadData(HANDLE hFile);
        void SaveData(HANDLE hFile) const;

        bool IsPNG() const;

        BITMAPINFOHEADER* GetBITMAPINFOHEADER()
        {
            _ASSERTE(!IsPNG());
            return reinterpret_cast<BITMAPINFOHEADER*>(data.data());
        }
        const BITMAPINFOHEADER* GetBITMAPINFOHEADER() const
        {
            _ASSERTE(!IsPNG());
            return reinterpret_cast<const BITMAPINFOHEADER*>(data.data());
        }

        int GetColorSize() const
        {
            _ASSERTE(!IsPNG());

            const BITMAPINFOHEADER* header = GetBITMAPINFOHEADER();
            int iColorCount = dir.bColorCount;
            if (iColorCount == 0 && header->biBitCount == 8)
                iColorCount = 256; // 2**8
            return iColorCount;
        }
        RGBQUAD* GetColors()
        {
            _ASSERTE(!IsPNG());
            return reinterpret_cast<RGBQUAD*>(data.data() + sizeof(BITMAPINFOHEADER));
        }
        const RGBQUAD* GetColors() const
        {
            _ASSERTE(!IsPNG());
            return reinterpret_cast<const RGBQUAD*>(data.data() + sizeof(BITMAPINFOHEADER));
        }

        DWORD GetBytesPerLineXOR() const
        {
            _ASSERTE(!IsPNG());
            const BITMAPINFOHEADER* header = GetBITMAPINFOHEADER();
            return pad32(header->biWidth * header->biBitCount) / 8;
        }

        DWORD GetBytesPerLineAND() const
        {
            _ASSERTE(!IsPNG());
            const BITMAPINFOHEADER* header = GetBITMAPINFOHEADER();
            return pad32(header->biWidth) / 8;
        }

    private:
        std::vector<BYTE> data;
    };

    ICONHEADER Header;
    std::vector<Entry> entry;
};
