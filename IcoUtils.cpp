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

RGBQUAD ParseColor(LPCTSTR str)
{
    const unsigned long i = _tcstoul(str, nullptr, 0);
    const RGBQUAD c = {
        (i >> 0) & 0xFF,
        (i >> 8) & 0xFF,
        (i >> 16) & 0xFF,
        (i >> 24) & 0xFF,
    };
    return c;
}

void IconList(const IconFile& IconData)
{
    int i = 0;
    for (const IconFile::Entry& entry : IconData.entry)
    {
        _tprintf(TEXT("%2d: %3d x %3d x %3d %s\n"), i, entry.dir.bWidth, entry.dir.bHeight, entry.dir.wBitCount,
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

void GrayscaleToAlpha(IconFile& IconData)
{
    for (IconFile::Entry& entry : IconData.entry)
    {
        if (!entry.IsPNG())
        {
            IconImage dest(entry);
            for (int y = 0; y < dest.GetHeight(); ++y)
            {
                for (int x = 0; x < dest.GetWidth(); ++x)
                {
                    RGBQUAD c = dest.GetColour(x, y);
                    if (c.rgbRed != c.rgbGreen || c.rgbRed != c.rgbBlue) throw Error(TEXT("Not grayscale"));
                    c.rgbReserved = 255 - c.rgbRed;
                    c.rgbRed = 0;
                    c.rgbGreen = 0;
                    c.rgbBlue = 0;
                    dest.PutColour(x, y, c);
                }
            }
        }
    }
}

void Recolor(IconFile::Entry& entry, int iconum, const RGBQUAD srccolor, const RGBQUAD dstcolor)
{
    IconImage dest(entry);
    for (int y = 0; y < dest.GetHeight(); ++y)
    {
        for (int x = 0; x < dest.GetWidth(); ++x)
        {
            RGBQUAD c = dest.GetColour(x, y);

            if (c.rgbReserved != 0 && c.rgbRed == srccolor.rgbRed && c.rgbGreen == srccolor.rgbGreen && c.rgbBlue == srccolor.rgbBlue)
            {
                c.rgbRed = dstcolor.rgbRed;
                c.rgbGreen = dstcolor.rgbGreen;
                c.rgbBlue = dstcolor.rgbBlue;
                dest.PutColour(x, y, c);
            }
        }
    }
}

BOOL HandleResourceName(
    HMODULE hModule,
    LPCTSTR lpType,
    LPTSTR lpName,
    LONG_PTR lParam
)
{
    if (IS_INTRESOURCE(lpName))
        _tprintf(TEXT("%u\n"), (USHORT) (INT_PTR) lpName);
    else
        _tprintf(TEXT("%s\n"), lpName);
    return TRUE;
}

bool ParseIconIndex(LPTSTR arg, int* index)
{
    LPTSTR x;
    if ((x = _tcsstr(arg, TEXT(".exe,"))) || (x = _tcsstr(arg, TEXT(".dll,"))))
    {
        x += 4;
        *index = _tstoi(x + 1);
        *x = TEXT('\0');
        return true;
    }
    else
        return false;
}

void ShowUsage()
{
    _tprintf(TEXT("Usage %s <options> [command] <command args>\n"), argapp());
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Options:\n"));
    _tprintf(TEXT("\t/IgnoreValidatePng\t\t\t\t- do note validate png entries\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Command:\n"));
    _tprintf(TEXT("\tlist [ico file]\t\t\t\t- list icon sizes in file\n"));
    _tprintf(TEXT("\tshow [ico file] [icon num]\t\t- display icon in terminal\n"));
    _tprintf(TEXT("\tcopy [dest ico file] [src ico file]\t- copy icon\n"));
    _tprintf(TEXT("\talphablend [dest ico file] [src ico file] <blend ico file>...\t- alpha blend individual icons in file\n"));
    _tprintf(TEXT("\tgrayscalealpha [dest ico file] [src ico file]\t- convert grayscale into the alpha channel\n"));
    _tprintf(TEXT("\trecolor [ico file] [ico num] [src color] [dst color]\t- convert grayscale into the alpha channel\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Where:\n"));
    _tprintf(TEXT("\t[src ico file]\t- can be an icon file (.ico) or an exe/dll resource (.exe,n) (.dll,n)\n"));
}

int _tmain(const int argc, const TCHAR* argv[])
{
    try
    {
        arginit(argc, argv);
        int arg = 1;
        LPCTSTR cmd = argnum(arg++);
        const bool bIgnoreValidatePng = argswitch(TEXT("/IgnoreValidatePng"));

        if (cmd == nullptr)
        {
            ShowUsage();
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("list")) == 0)
        {
            LPCTSTR icofilearg = argnum(arg++);
            if (!argcleanup() || icofilearg == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            WCHAR icofile[MAX_PATH];
            ExpandEnvironmentStrings(icofilearg, icofile, ARRAYSIZE(icofile));

            size_t len = _tcslen(icofile);
            if ((_tcscmp(icofile + len - 4, TEXT(".exe")) == 0) || (_tcscmp(icofile + len - 4, TEXT(".dll")) == 0))
            {
                HMODULE hModule = LoadLibraryEx(icofile, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
                CHECK(hModule);
                EnumResourceNames(hModule, RT_GROUP_ICON, HandleResourceName, 0);
                FreeLibrary(hModule);
                _tprintf(TEXT("\n"));
            }
            else
            {
                int index = 0;
                const IconFile IconData = ParseIconIndex(icofile, &index)
                    ? IconFile::FromResource(icofile, index, bIgnoreValidatePng)
                    : IconFile::Load(icofile, bIgnoreValidatePng);
                IconList(IconData);
            }
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("show")) == 0)
        {
            LPCTSTR icofilearg = argnum(arg++);
            int iconum = _tstoi(argnum(arg++, TEXT("0")));
            if (!argcleanup() || icofilearg == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            WCHAR icofile[MAX_PATH];
            ExpandEnvironmentStrings(icofilearg, icofile, ARRAYSIZE(icofile));

            int index = 0;
            const IconFile IconData = ParseIconIndex(icofile, &index)
                ? IconFile::FromResource(icofile, index, bIgnoreValidatePng)
                : IconFile::Load(icofile, bIgnoreValidatePng);

            if (iconum < 0 || iconum >= IconData.entry.size())
            {
                _tprintf(TEXT("Invalid icon index\n"));
                return EXIT_FAILURE;
            }

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
            LPCTSTR outicofilearg = argnum(arg++);
            LPCTSTR inicofilearg = argnum(arg++);
            if (outicofilearg == nullptr || inicofilearg == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            WCHAR outicofile[MAX_PATH];
            WCHAR inicofile[MAX_PATH];
            ExpandEnvironmentStrings(outicofilearg, outicofile, ARRAYSIZE(outicofile));
            ExpandEnvironmentStrings(inicofilearg, inicofile, ARRAYSIZE(inicofile));

            int index = 0;
            IconFile IconData = ParseIconIndex(inicofile, &index)
                ? IconFile::FromResource(inicofile, index, bIgnoreValidatePng)
                : IconFile::Load(inicofile, bIgnoreValidatePng);

            LPCTSTR blendicofile;
            while ((blendicofile = argnum(arg++)) != nullptr)
            {
                const IconFile IconDataBlend = IconFile::Load(blendicofile, bIgnoreValidatePng);
                AlphaBlendImages(IconData, IconDataBlend);
            }
            if (!argcleanup())
            {
                ShowUsage();
                return EXIT_FAILURE;
            }
            IconData.Save(outicofile, bIgnoreValidatePng);
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("grayscalealpha")) == 0)
        {
            LPCTSTR outicofilearg = argnum(arg++);
            LPCTSTR inicofilearg = argnum(arg++);
            if (!argcleanup() || outicofilearg == nullptr || inicofilearg == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            WCHAR outicofile[MAX_PATH];
            WCHAR inicofile[MAX_PATH];
            ExpandEnvironmentStrings(outicofilearg, outicofile, ARRAYSIZE(outicofile));
            ExpandEnvironmentStrings(inicofilearg, inicofile, ARRAYSIZE(inicofile));

            int index = 0;
            IconFile IconData = ParseIconIndex(inicofile, &index)
                ? IconFile::FromResource(inicofile, index, bIgnoreValidatePng)
                : IconFile::Load(inicofile, bIgnoreValidatePng);

            GrayscaleToAlpha(IconData);
            IconData.Save(outicofile, bIgnoreValidatePng);
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("copy")) == 0)
        {
            LPCTSTR outicofilearg = argnum(arg++);
            LPCTSTR inicofilearg = argnum(arg++);
            if (!argcleanup() || outicofilearg == nullptr || inicofilearg == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            WCHAR outicofile[MAX_PATH];
            WCHAR inicofile[MAX_PATH];
            ExpandEnvironmentStrings(outicofilearg, outicofile, ARRAYSIZE(outicofile));
            ExpandEnvironmentStrings(inicofilearg, inicofile, ARRAYSIZE(inicofile));

            int index = 0;
            const IconFile IconData = ParseIconIndex(inicofile, &index)
                ? IconFile::FromResource(inicofile, index, bIgnoreValidatePng)
                : IconFile::Load(inicofile, bIgnoreValidatePng);

            IconData.Save(outicofile, bIgnoreValidatePng);
            return EXIT_SUCCESS;
        }
        else if (_tcsicmp(cmd, TEXT("recolor")) == 0)
        {
            LPCTSTR icofilearg = argnum(arg++);
            int iconum = _tstoi(argnum(arg++, TEXT("0")));
            RGBQUAD srccolor = ParseColor(argnum(arg++, TEXT("0")));
            RGBQUAD dstcolor = ParseColor(argnum(arg++, TEXT("0")));
            if (!argcleanup() || icofilearg == nullptr)
            {
                ShowUsage();
                return EXIT_FAILURE;
            }

            WCHAR icofile[MAX_PATH];
            ExpandEnvironmentStrings(icofilearg, icofile, ARRAYSIZE(icofile));

            int index = 0;
            IconFile IconData = IconFile::Load(icofile, bIgnoreValidatePng);

            if (iconum < 0 || iconum >= IconData.entry.size())
            {
                _tprintf(TEXT("Invalid icon index\n"));
                return EXIT_FAILURE;
            }

            IconFile::Entry& entry = IconData.entry[iconum];
            if (entry.IsPNG())
            {
                _tprintf(TEXT("PNG not supported\n"));
                return EXIT_FAILURE;
            }

            Recolor(entry, iconum, srccolor, dstcolor);

            IconData.Save(icofile, bIgnoreValidatePng);
            return EXIT_SUCCESS;
            }
        else
        {
            _ftprintf(stderr, TEXT("Unknown command: %s\n"), cmd);
            return EXIT_FAILURE;
        }
    }
    catch (const WinError& e)
    {
        _ftprintf(stderr, TEXT("Error: 0x%08x\n"), e.GetError());
        return EXIT_FAILURE;
    }
    catch (const Error& e)
    {
        _ftprintf(stderr, TEXT("%s\n"), e.GetMsg().c_str());
        return EXIT_FAILURE;
    }
}
