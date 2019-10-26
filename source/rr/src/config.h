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

#ifndef config_public_h_
#define config_public_h_

#include "gamecontrol.h"

BEGIN_RR_NS

#define SETUPNAMEPARM "SETUPFILE"

int32_t CONFIG_ReadSetup( void );
void CONFIG_GetSetupFilename( void );
void CONFIG_WriteSetup(uint32_t flags);
void CONFIG_SetupMouse( void );
void CONFIG_SetupJoystick( void );

int32_t CONFIG_GetMapBestTime(char const * mapname, uint8_t const * mapmd4);
int32_t CONFIG_SetMapBestTime(uint8_t const * mapmd4, int32_t tm);

void CONFIG_SetDefaults(void);

END_RR_NS

#endif
