#include "IconImage.h"

#include "IconFile.h"

inline BYTE getNybble(BYTE b, int i)
{
    return (b >> (i * 4)) & 0xF;
}

IconImage::IconImage(const IconFile::Entry& entry)
{
    _ASSERTE(!entry.IsPNG());

    const BITMAPINFOHEADER* header = entry.GetBITMAPINFOHEADER();
    _ASSERTE(header->biPlanes == 1);

    biWidth = header->biWidth;
    biHeight = header->biHeight / 2;
    biBitCount = header->biBitCount;
    iColorCount = entry.GetColorSize();

    dwBytesPerLineXOR = entry.GetBytesPerLineXOR();
    dwBytesPerLineAND = entry.GetBytesPerLineAND();

    pColor = const_cast<RGBQUAD*>(entry.GetColors());
    pXOR = reinterpret_cast<BYTE*>(pColor + iColorCount);
    pAND = pXOR + (dwBytesPerLineXOR * biHeight);
}

RGBQUAD IconImage::GetColour(int x, int y) const
{
    _ASSERTE(x >= 0 && x < GetWidth());
    _ASSERTE(y >= 0 && y < GetHeight());

    RGBQUAD c = {};
    c.rgbReserved = 255;
    switch (biBitCount)
    {
    case 1:
    {
        _ASSERTE(iColorCount == 2);
        const BYTE v = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        const BYTE n = GetBit(v, 8 - 1 - x % 8);
        c = GetColour(n);
        break;
    }

    case 4:
    {
        _ASSERTE(iColorCount == 16);
        const BYTE v = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        const BYTE n = getNybble(v, 2 - 1 - x % 2);
        c = GetColour(n);
        break;
    }

    case 8:
    {
        _ASSERTE(iColorCount == 256);
        const BYTE n = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        c = GetColour(n);
        break;
    }

    case 24:
    {
        const UINT32 v = *reinterpret_cast<UINT32*>(GetColourPtr(x, y)) & 0xFFFFFF;
        c.rgbRed = (v >> 16) & 0xFF;
        c.rgbGreen = (v >> 8) & 0xFF;
        c.rgbBlue = (v >> 0) & 0xFF;
        break;
    }

    case 32:
    {
        const UINT32 v = *reinterpret_cast<UINT32*>(GetColourPtr(x, y));
        c.rgbReserved = (v >> 24) & 0xFF;
        c.rgbRed = (v >> 16) & 0xFF;
        c.rgbGreen = (v >> 8) & 0xFF;
        c.rgbBlue = (v >> 0) & 0xFF;
        break;
    }

    default:
        _RPTFWN(_CRT_ASSERT, TEXT("biBitCount %d not supported"), biBitCount);
        break;
    }
    if (GetMask(x, y))
        c.rgbReserved = 0;
    return c;
}

void IconImage::PutColour(int x, int y, const RGBQUAD c) const
{
    _ASSERTE(x >= 0 && x < GetWidth());
    _ASSERTE(y >= 0 && y < GetHeight());

    switch (biBitCount)
    {
    case 1:
    {
        _ASSERTE(iColorCount == 2);
        _ASSERT_EXPR(FALSE, TEXT("TODO"));
        //const BYTE v = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        //const BYTE n = GetBit(v, 8 - 1 - x % 8);
        //c = GetColour(n);
        break;
    }

    case 4:
    {
        _ASSERTE(iColorCount == 16);
        _ASSERT_EXPR(FALSE, TEXT("TODO"));
        //const BYTE v = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        //const BYTE n = getNybble(v, 2 - 1 - x % 2);
        //c = GetColour(n);
        break;
    }

    case 8:
    {
        _ASSERTE(iColorCount == 256);
        _ASSERT_EXPR(FALSE, TEXT("TODO"));
        //const BYTE n = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        //c = GetColour(n);
        break;
    }

    case 24:
    {
        _ASSERT_EXPR(FALSE, TEXT("TODO"));
        //const UINT32 v = *reinterpret_cast<UINT32*>(GetColourPtr(x, y)) & 0xFFFFFF;
        //c.rgbRed = (v >> 16) & 0xFF;
        //c.rgbGreen = (v >> 8) & 0xFF;
        //c.rgbBlue = (v >> 0) & 0xFF;
        break;
    }

    case 32:
    {
        const UINT32 v = (c.rgbReserved << 24) | (c.rgbRed << 16) | (c.rgbGreen << 8) | (c.rgbBlue << 0);
        *reinterpret_cast<UINT32*>(GetColourPtr(x, y)) = v;
        break;
    }

    default:
        _RPTFWN(_CRT_ASSERT, TEXT("biBitCount %d not supported"), biBitCount);
        break;
    }
    SetMask(x, y, c.rgbReserved == 255);
}