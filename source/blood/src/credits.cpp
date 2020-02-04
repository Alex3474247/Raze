//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "compat.h"
#include "SmackerDecoder.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "resource.h"
#include "screen.h"
#include "sound.h"
#include "view.h"
#include "../glbackend/glbackend.h"
#include "sound/s_soundinternal.h"

BEGIN_BLD_NS

char exitCredits = 0;

char Wait(int nTicks)
{
    totalclock = 0;
    while (totalclock < nTicks)
    {
        gameHandleEvents();
        auto key = inputState.keyGetScan();
        if (key)
        {
            if (key == sc_Escape) // sc_Escape
                exitCredits = 1;
            return 0;
        }
    }
    return 1;
}

char DoFade(char r, char g, char b, int nTicks)
{
#if 0
    dassert(nTicks > 0);
    scrSetupFade(r, g, b);
    totalclock = gFrameClock = 0;
    do
    {
        while (totalclock < gFrameClock) { gameHandleEvents();};
        gFrameClock += 2;
        scrNextPage();
        scrFadeAmount(divscale16(ClipHigh((int)totalclock, nTicks), nTicks));
        if (inputState.keyGetScan())
            return 0;
    } while (totalclock <= nTicks);
#endif
    return 1;
}

char DoUnFade(int nTicks)
{
#if 0
    dassert(nTicks > 0);
    scrSetupUnfade();
    totalclock = gFrameClock = 0;
    do
    {
        while (totalclock < gFrameClock) { gameHandleEvents(); };
        gFrameClock += 2;
        scrNextPage();
        scrFadeAmount(0x10000-divscale16(ClipHigh((int)totalclock, nTicks), nTicks));
        if (inputState.keyGetScan())
            return 0;
    } while (totalclock <= nTicks);
#endif
    return 1;
}

void credPlaySmk(const char* _pzSMK, const char* _pzWAV, int nWav);

void credLogosDos(void)
{
    char bShift = inputState.ShiftPressed();
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    DoUnFade(1);
    videoClearScreen(0);
    if (bShift)
        return;
    {
        if (fileSystem.FindFile("logo.smk"))
        {
            credPlaySmk("logo.smk", "logo.wav", -1);
        }
        else
        {
            rotatesprite(160<<16, 100<<16, 65536, 0, 2050, 0, 0, 0x4a, 0, 0, xdim-1, ydim-1);
            sndStartSample("THUNDER2", 128, -1);
            scrNextPage();
            if (!Wait(360))
                return;
            if (!DoFade(0, 0, 0, 60))
                return;
        }
        if (fileSystem.FindFile("gti.smk"))
        {
            credPlaySmk("gti.smk", "gt.wav", -1);
        }
        else
        {
            videoClearScreen(0);
            rotatesprite(160<<16, 100<<16, 65536, 0, 2052, 0, 0, 0x0a, 0, 0, xdim-1, ydim-1);
            scrNextPage();
            DoUnFade(1);
            sndStartSample("THUNDER2", 128, -1);
            if (!Wait(360))
                return;
        }
    }
    sndPlaySpecialMusicOrNothing(MUS_INTRO);
    sndStartSample("THUNDER2", 128, -1);
    if (!DoFade(0, 0, 0, 60))
        return;
    videoClearScreen(0);
    scrNextPage();
    if (!DoUnFade(1))
        return;
    videoClearScreen(0);
    rotatesprite(160<<16, 100<<16, 65536, 0, 2518, 0, 0, 0x4a, 0, 0, xdim-1, ydim-1);
    scrNextPage();
    Wait(360);
    //Mus_Fade(4000);
}

void credReset(void)
{
    videoClearScreen(0);
    scrNextPage();
    DoFade(0,0,0,1);
    scrSetupUnfade();
    DoUnFade(1);
}

bool credKOpen4Load(FString &pzFile)
{
    int nLen = strlen(pzFile);
    FixPathSeperator(pzFile);
    auto nHandle = fileSystem.FindFile(pzFile);
    if (nHandle < 0)
    {
        // Strip the drive letter and retry.
        if (nLen >= 3 && isalpha(pzFile[0]) && pzFile[1] == ':' && pzFile[2] == '/')
        {
            pzFile = pzFile.Mid(3);
            nHandle = fileSystem.FindFile(pzFile);
        }
    }
    return nHandle;
}

#define kSMKPal 5
#define kSMKTile (MAXTILES-1)

void credPlaySmk(const char *_pzSMK, const char *_pzWAV, int nWav)
{
#if 0
    CSMKPlayer smkPlayer;
    if (dword_148E14 >= 0)
    {
        if (toupper(*pzSMK) == 'A'+dword_148E14)
        {
            if (Redbook.sub_82258() == 0 || Redbook.sub_82258() > 20)
                return;
        }
        Redbook.sub_82554();
    }
    smkPlayer.sub_82E6C(pzSMK, pzWAV);
#endif
    if (!_pzSMK || !*_pzSMK)
        return;
    FString pzSMK = _pzSMK;
    FString pzWAV = _pzWAV;
    auto nHandleSMK = credKOpen4Load(pzSMK);
    if (!nHandleSMK)
    {
        return;
    }
    SmackerHandle hSMK = Smacker_Open(pzSMK);
    if (!hSMK.isValid)
    {
        return;
    }
    uint32_t nWidth, nHeight;
    Smacker_GetFrameSize(hSMK, nWidth, nHeight);
    uint8_t palette[768];
	tileDelete(kSMKTile);
	auto pFrame = TileFiles.tileCreate(kSMKTile, nHeight, nWidth);
    if (!pFrame)
    {
        Smacker_Close(hSMK);
        return;
    }
    int nFrameRate = Smacker_GetFrameRate(hSMK);
    int nFrames = Smacker_GetNumFrames(hSMK);

    Smacker_GetPalette(hSMK, palette);
    paletteSetColorTable(kSMKPal, palette, true);
    videoSetPalette(0, kSMKPal, Pal_Fullscreen);

    int nScale;

    if ((nWidth / (nHeight * 1.2f)) > (1.f * xdim / ydim))
        nScale = divscale16(320 * xdim * 3, nWidth * ydim * 4);
    else
        nScale = divscale16(200, nHeight);

    if (nWav > 0)
        sndStartWavID(nWav, 255);
    else
    {
        auto nHandleWAV = credKOpen4Load(pzWAV);
        if (nHandleWAV)
        {
            sndStartWavDisk(pzWAV, 255);
        }
    }

    UpdateDacs(0, true);

    gameHandleEvents();
    ClockTicks nStartTime = totalclock;

    inputState.ClearAllInput();
    
    int nFrame = 0;
    hw_int_useindexedcolortextures = false;
    do
    {
        gameHandleEvents();
        if (scale((int)(totalclock-nStartTime), nFrameRate, kTicRate) < nFrame)
            continue;

        if (inputState.CheckAllInput())
            break;

        videoClearScreen(0);
        Smacker_GetPalette(hSMK, palette);
        paletteSetColorTable(kSMKPal, palette, true);
        videoSetPalette(0, kSMKPal, Pal_Fullscreen);
        tileInvalidate(kSMKTile, 0, 1 << 4);  // JBF 20031228
        Smacker_GetFrame(hSMK, pFrame);
        rotatesprite_fs(160<<16, 100<<16, nScale, 512, kSMKTile, 0, 0, 2|4|8|64);

        videoNextPage();

        nFrame++;
        Smacker_GetNextFrame(hSMK);
    } while(nFrame < nFrames);
    hw_int_useindexedcolortextures = hw_useindexedcolortextures;

    Smacker_Close(hSMK);
    inputState.ClearAllInput();
    soundEngine->StopAllChannels();
    videoSetPalette(0, 0, 0);
	tileDelete(kSMKTile);
}

END_BLD_NS
