#include "IconImage.h"

#include "IconFile.h"
#include "Utils.h"
#include "Format.h"

inline BYTE getNybble(BYTE b, int i)
{
    return (b >> (i * 4)) & 0xF;
}

inline long ColourDistanceSq(RGBQUAD e1, RGBQUAD e2)
{
    const long rmean = ((long) e1.rgbRed + (long) e2.rgbRed) / 2;
    const long r = (long) e1.rgbRed - (long) e2.rgbRed;
    const long g = (long) e1.rgbGreen - (long) e2.rgbGreen;
    const long b = (long) e1.rgbBlue - (long) e2.rgbBlue;
    const long s = 0;// (long) e1.rgbReserved - (long) e2.rgbReserved;
    return (((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8) + s * s;
}

int IconImage::GetNearestColour(const RGBQUAD c) const
{
    const RGBQUAD* it = std::min_element(pColor, pColor + iColorCount, [c](RGBQUAD x, RGBQUAD y)
        {
            return ColourDistanceSq(x, c) < ColourDistanceSq(y, c);
        });
    return int(it - pColor);
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
    switch (biBitCount)
    {
    case 1:
    {
        _ASSERTE(iColorCount == 2);
        const BYTE v = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        const BYTE n = GetBit(v, 8 - 1 - x % 8);
        c = GetColour(n);
        c.rgbReserved = GetMask(x, y) ? 0 : 255;
        break;
    }

    case 4:
    {
        _ASSERTE(iColorCount == 16);
        const BYTE v = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        const BYTE n = getNybble(v, 2 - 1 - x % 2);
        c = GetColour(n);
        c.rgbReserved = GetMask(x, y) ? 0 : 255;
        break;
    }

    case 8:
    {
        _ASSERTE(iColorCount == 256);
        const BYTE n = *reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        c = GetColour(n);
        c.rgbReserved = GetMask(x, y) ? 0 : 255;
        break;
    }

    case 24:
    {
        _ASSERTE(iColorCount == 0);
        const UINT32 v = *reinterpret_cast<UINT32*>(GetColourPtr(x, y)) & 0xFFFFFF;
        c.rgbRed = (v >> 16) & 0xFF;
        c.rgbGreen = (v >> 8) & 0xFF;
        c.rgbBlue = (v >> 0) & 0xFF;
        c.rgbReserved = GetMask(x, y) ? 0 : 255;
        break;
    }

    case 32:
    {
        _ASSERTE(iColorCount == 0);
        const UINT32 v = *reinterpret_cast<UINT32*>(GetColourPtr(x, y));
        c.rgbReserved = (v >> 24) & 0xFF;
        c.rgbRed = (v >> 16) & 0xFF;
        c.rgbGreen = (v >> 8) & 0xFF;
        c.rgbBlue = (v >> 0) & 0xFF;
        if (GetMask(x, y))
            c.rgbReserved = 0;
        break;
    }

    default:
        throw Error(Format(TEXT("biBitCount %d not supported"), biBitCount));
        break;
    }
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
        throw Error(Format(TEXT("TODo support biBitCount %d"), biBitCount));
        //const BYTE n = GetNearestColour(c);
        //const BYTE* p = reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        //*p = SetBit(*p, 8 - 1 - x % 8, n);
        break;
    }

    case 4:
    {
        _ASSERTE(iColorCount == 16);
        throw Error(Format(TEXT("TODo support biBitCount %d"), biBitCount));
        //const BYTE n = GetNearestColour(c);
        //const BYTE* p = reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        //*p = setNybble(*p, 8 - 1 - x % 8, n);
        break;
    }

    case 8:
    {
        _ASSERTE(iColorCount == 256);
        const BYTE n = GetNearestColour(c);
        BYTE* p = reinterpret_cast<BYTE*>(GetColourPtr(x, y));
        *p = n;
        break;
    }

    case 24:
    {
        _ASSERTE(iColorCount == 0);
        const UINT32 v = (c.rgbRed << 16) | (c.rgbGreen << 8) | (c.rgbBlue << 0);
        UINT32* p = reinterpret_cast<UINT32*>(GetColourPtr(x, y));
        *p = (*p & 0xFF000000) | v;
        break;
    }

    case 32:
    {
        _ASSERTE(iColorCount == 0);
        const UINT32 v = (c.rgbReserved << 24) | (c.rgbRed << 16) | (c.rgbGreen << 8) | (c.rgbBlue << 0);
        *reinterpret_cast<UINT32*>(GetColourPtr(x, y)) = v;
        break;
    }

    default:
        throw Error(Format(TEXT("biBitCount %d not supported"), biBitCount));
        break;
    }
    SetMask(x, y, c.rgbReserved == 0);
}
