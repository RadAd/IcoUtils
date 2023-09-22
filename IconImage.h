#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>

#include "IconFile.h"

inline bool GetBit(BYTE b, int i)
{
    return b & (1 << i);
}

inline BYTE SetBit(BYTE b, int i, bool s)
{
    return b & ~(1 << i) | ((s ? 1 : 0) << i);
}

class IconImage
{
public:
    IconImage(const IconFile::Entry& entry);

    LONG GetWidth() const { return biWidth; }
    LONG GetHeight() const { return biHeight; }

    RGBQUAD GetColour(int i) const { _ASSERTE(i >= 0 && i < iColorCount); return pColor[i]; }
    int GetNearestColour(const RGBQUAD c) const;

    bool GetMask(int x, int y) const
    {
        _ASSERTE(x >= 0 && x < GetWidth());
        _ASSERTE(y >= 0 && y < GetHeight());

        const int o = (biHeight - y - 1) * dwBytesPerLineAND + x / 8;
        return GetBit(pAND[o], 8 - 1 - x % 8);
    }

    void SetMask(int x, int y, bool m) const
    {
        _ASSERTE(x >= 0 && x < GetWidth());
        _ASSERTE(y >= 0 && y < GetHeight());

        const int o = (biHeight - y - 1) * dwBytesPerLineAND + x / 8;
        pAND[o] = SetBit(pAND[o], 8 - 1 - x % 8, m);
    }

    RGBQUAD GetColour(int x, int y) const;

    void PutColour(int x, int y, const RGBQUAD c) const;

private:
    LPVOID GetColourPtr(int x, int y) const
    {
        _ASSERTE(x >= 0 && x < GetWidth());
        _ASSERTE(y >= 0 && y < GetHeight());

        const int o = (biHeight - y - 1) * dwBytesPerLineXOR + x * biBitCount / 8;
        return pXOR + o;
    }

    LONG biWidth;
    LONG biHeight;
    WORD biBitCount;
    int iColorCount;

    DWORD dwBytesPerLineXOR;
    DWORD dwBytesPerLineAND;

    RGBQUAD* pColor;
    BYTE* pXOR;
    BYTE* pAND;
};
