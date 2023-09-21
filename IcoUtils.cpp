#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
//#include <cmath>
#include <tchar.h>
//#include <strsafe.h>
//#include <crtdbg.h>

#include "IconFile.h"
#include "IconImage.h"
#include "Utils.h"
#include "arg.h"

inline int Fix(BYTE b)
{
    return b == 0 ? 256 : b;
}

void IconList(const IconFile& IconData)
{
    int i = 0;
    for (const IconFile::Entry& entry : IconData.entry)
    {
        _tprintf(TEXT("%2d: %3d x %3d x %3d %s\n"), i, Fix(entry.dir.bWidth), Fix(entry.dir.bHeight), entry.dir.wBitCount,
            entry.IsPNG() ? TEXT("PNG") : TEXT("BMP"));
        ++i;
    }
    _tprintf(TEXT("\n"));
}

void PrintImage(const IconFile::Entry& entry)
{
    IconImage i(entry);
    for (int y = 0; y < i.GetHeight(); ++y)
    {
        for (int x = 0; x < i.GetWidth(); ++x)
        {
            const RGBQUAD c = i.GetColour(x, y);
            if (c.rgbReserved == 0)
                _tprintf(TEXT("\x1b[0m"));
            else
                _tprintf(TEXT("\x1b[38;2;%d;%d;%dm"), c.rgbRed, c.rgbGreen, c.rgbBlue);
            _tprintf(TEXT("\x1b[48;2;%d;%d;%dm"), 128, 128, 128);
            _tprintf(c.rgbReserved == 0 ? TEXT(" ") : TEXT("X"));
        }
        _tprintf(TEXT("\x1b[0m|\n"));
    }
    _tprintf(TEXT("\x1b[0m\n"));
}

auto FindImage(const IconFile& IconData, BYTE bWidth, BYTE bHeight, WORD wBitCount)
{
    for (auto it = IconData.entry.begin(); it != IconData.entry.end(); ++it)
    {
        if (it->dir.bWidth == bWidth && it->dir.bWidth == bHeight && it->dir.wBitCount == wBitCount)
            return it;
    }
    return IconData.entry.end();
}

RGBQUAD AlphaBlend(const RGBQUAD fg, const RGBQUAD bg)
{
    RGBQUAD r;
    r.rgbBlue = fg.rgbReserved * fg.rgbBlue / 255 + (255 - fg.rgbReserved) * bg.rgbBlue / 255;
    r.rgbGreen = fg.rgbReserved * fg.rgbGreen / 255 + (255 - fg.rgbReserved) * bg.rgbGreen / 255;
    r.rgbRed = fg.rgbReserved * fg.rgbRed / 255 + (255 - fg.rgbReserved) * bg.rgbRed / 255;
    r.rgbReserved = fg.rgbReserved + (255 - fg.rgbReserved) * bg.rgbReserved / 255;
    return r;
}

void AlphaBlendImages(IconFile& IconDataDest, const IconFile& IconDataSrc)
{
    for (IconFile::Entry& entry : IconDataDest.entry)
    {
        if (!entry.IsPNG())
        {
            const auto s = FindImage(IconDataSrc, entry.dir.bWidth, entry.dir.bWidth, entry.dir.wBitCount);
            if (s != IconDataSrc.entry.end())
            {
                const IconImage src(*s);
                IconImage dest(entry);
                for (int y = 0; y < dest.GetHeight(); ++y)
                {
                    for (int x = 0; x < dest.GetWidth(); ++x)
                    {
                        const RGBQUAD c = src.GetColour(x, y);
                        if (c.rgbReserved != 0)
                        {
                            const RGBQUAD r = AlphaBlend(c, dest.GetColour(x, y));
                            dest.PutColour(x, y, r);
                        }
                    }
                }
            }
        }
    }
}

void ShowUsage()
{
    _tprintf(TEXT("Usage %s <command> <command args>\n"), argapp());
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Command:\n"));
    _tprintf(TEXT("\tlist <ico file>\t\t\t\t- list icon sizes in file\n"));
    _tprintf(TEXT("\tshow <ico file> <icon num>\t\t- display icon in terminal\n"));
    _tprintf(TEXT("\talphablend <dest ico file> <src ico file> <blend ico file>\t- alpha blend individual icons in file\n"));
}

int _tmain(const int argc, const TCHAR* argv[])
{
    try
    {
        arginit(argc, argv);
        int arg = 1;
        LPCTSTR cmd = argnum(arg++);

        if (cmd == nullptr)
        {
            ShowUsage();
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("list")) == 0)
        {
            LPCTSTR icofile = argnum(arg++);
            if (!argcleanup() || icofile == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            IconFile IconData = IconFile::Load(icofile);
            IconList(IconData);
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("show")) == 0)
        {
            LPCTSTR icofile = argnum(arg++);
            int iconum = _tstoi(argnum(arg++, TEXT("0")));
            if (!argcleanup() || icofile == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            IconFile IconData = IconFile::Load(icofile);
            const IconFile::Entry& entry = IconData.entry[iconum];
            if (entry.IsPNG())
            {
                _tprintf(TEXT("PNG not supported\n"));
                return EXIT_FAILURE;
            }

            PrintImage(IconData.entry[iconum]);
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("alphablend")) == 0)
        {
            LPCTSTR outicofile = argnum(arg++);
            LPCTSTR inicofile = argnum(arg++);
            if (outicofile == nullptr || inicofile == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            IconFile IconData = IconFile::Load(inicofile);
            LPCTSTR blendicofile;
            while ((blendicofile = argnum(arg++)) != nullptr)
            {
                const IconFile IconDataBlend = IconFile::Load(blendicofile);
                AlphaBlendImages(IconData, IconDataBlend);
            }
            if (!argcleanup())
            {
                ShowUsage();
                return EXIT_FAILURE;
            }
            IconData.Save(outicofile);
            return EXIT_SUCCESS;
        }
    }
    catch (const WinError& e)
    {
        _ftprintf(stderr, TEXT("Error: 0x%08x\n"), e.GetError());
        return EXIT_FAILURE;
    }
} 
