//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#ifndef savegame_h_
#define savegame_h_

#include "game.h"

BEGIN_RR_NS

# define SV_MAJOR_VER 1
#define SV_MINOR_VER 7

#pragma pack(push,1)
typedef struct
{
    char headerstr[11];
    uint8_t majorver, minorver, ptrsize;
    uint16_t bytever;
    // 16 bytes

    uint32_t userbytever;
    uint32_t scriptcrc;

    uint8_t comprthres;
    uint8_t recdiffsp, diffcompress, synccompress;
    // 4 bytes

    int32_t reccnt, snapsiz;
    // 8 bytes

    char savename[MAXSAVEGAMENAMESTRUCT];
    uint8_t numplayers, volnum, levnum, skill;
    char boardfn[BMAX_PATH];
    // 286 bytes

    uint8_t getPtrSize() const { return ptrsize; }
} savehead_t;
#pragma pack(pop)

int32_t sv_updatestate(int32_t frominit);
int32_t sv_readdiff(FileReader& fil);
uint32_t sv_writediff(FileWriter *fil);
int32_t sv_loadheader(FileReader &fil, int32_t spot, savehead_t *h);
int32_t sv_loadsnapshot(FileReader &fil, int32_t spot, savehead_t *h);
int32_t sv_saveandmakesnapshot(FileWriter &fil, char const *name, int8_t spot, int8_t recdiffsp, int8_t diffcompress, int8_t synccompress, bool isAutoSave = false);
void sv_freemem();
int32_t G_LoadSaveHeaderNew(char const *fn, savehead_t *saveh);
void ReadSaveGameHeaders(void);

#ifdef YAX_ENABLE
extern void sv_postyaxload(void);
#endif

// XXX: The 'bitptr' decl really belongs into gamedef.h, but we don't want to
// pull all of it in savegame.c?
extern char *bitptr;

enum
{
    P2I_BACK_BIT = 1,
    P2I_ONLYNON0_BIT = 2,

    P2I_FWD = 0,
    P2I_BACK = 1,

    P2I_FWD_NON0 = 0+2,
    P2I_BACK_NON0 = 1+2,
};
void G_Util_PtrToIdx(void *ptr, int32_t count, const void *base, int32_t mode);
void G_Util_PtrToIdx2(void *ptr, int32_t count, size_t stride, const void *base, int32_t const mode);

END_RR_NS

#endif
