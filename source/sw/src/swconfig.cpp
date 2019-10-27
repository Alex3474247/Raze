//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"

//#include "settings.h"
#include "mytypes.h"
#include "scriplib.h"
#include "fx_man.h"
#include "gamedefs.h"
#include "common_game.h"
#include "config.h"
#include "gamecontrol.h"

#include "rts.h"

BEGIN_SW_NS

short GamePlays = 0;

/*
===================
=
= SetGameDefaults
=
===================
*/

void SetGameDefaults(void)
{
}

extern SWBOOL DrawScreen;

void EncodePassword(char *pw)
{
    int bak_DrawScreen = DrawScreen;
    int bak_randomseed = randomseed;
    int i;
    int len;

    DrawScreen = FALSE;
    randomseed = 1234L;

    Bstrupr(pw);

    len = strlen(pw);
    for (i = 0; i < len; i++)
        pw[i] += RANDOM_RANGE(26);

    randomseed = bak_randomseed;
    DrawScreen = bak_DrawScreen;
}

void DecodePassword(char *pw)
{
    int bak_DrawScreen = DrawScreen;
    int bak_randomseed = randomseed;
    int i;
    int len;

    DrawScreen = FALSE;
    randomseed = 1234L;

    Bstrupr(pw);

    len = strlen(pw);
    for (i = 0; i < len; i++)
        pw[i] -= RANDOM_RANGE(26);

    randomseed = bak_randomseed;
    DrawScreen = bak_DrawScreen;
}


void TermSetup(void)
{
    extern SWBOOL BotMode;
}
END_SW_NS
