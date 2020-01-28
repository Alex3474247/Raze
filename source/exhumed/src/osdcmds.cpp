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
#include "ns.h"
#include "compat.h"
#include "build.h"
#include "common.h"
#include "exhumed.h"
#include "osdcmds.h"
#include "view.h"
#include "mapinfo.h"

BEGIN_PS_NS


static int osdcmd_god(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (!nNetPlayerCount && !bInDemo)
    {
        DoPassword(3);
    }
    else
        OSD_Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_noclip(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (!nNetPlayerCount && !bInDemo)
    {
        DoPassword(6);
    }
    else
    {
        OSD_Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int osdcmd_map(osdcmdptr_t parm)
{
	FString mapname;

    if (parm->numparms != 1)
    {
        return OSDCMD_SHOWHELP;
    }
	
    if (!fileSystem.Lookup(mapname, "MAP"))
    {
        OSD_Printf(OSD_ERROR "map: file \"%s\" not found.\n", mapname.GetChars());
        return OSDCMD_OK;
    }
	
	// Check if the map is already defined.
    for (int i = 0; i <= ISDEMOVER? 4 : 32; i++)
    {
        if (mapList[i].labelName.CompareNoCase(mapname) == 0)
        {
			levelnew = i;
			levelnum = i;
			return OSDCMD_OK;
        }
    }
    return OSDCMD_OK;
}

static int osdcmd_changelevel(osdcmdptr_t parm)
{
    char* p;

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    int nLevel = strtol(parm->parms[0], &p, 10);
    if (p[0]) return OSDCMD_SHOWHELP;

    if (nLevel < 0) return OSDCMD_SHOWHELP;

    int nMaxLevels;

    if (!ISDEMOVER) {
        nMaxLevels = 32;
    }
    else {
        nMaxLevels = 4;
    }

    if (nLevel > nMaxLevels)
    {
        OSD_Printf("changelevel: invalid level number\n");
        return OSDCMD_SHOWHELP;
    }

    levelnew = nLevel;
    levelnum = nLevel;

    return OSDCMD_OK;
}


int32_t registerosdcommands(void)
{
    //if (VOLUMEONE)
    OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    OSD_RegisterFunction("map","map <mapname>: loads the given map", osdcmd_map);
    //    OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
    //}

    //OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);


    //OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    //OSD_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);

    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);
    //OSD_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);
    //OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);

    //OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    return 0;
}


END_PS_NS
