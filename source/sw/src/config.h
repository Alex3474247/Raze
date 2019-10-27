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

#ifndef config_public_
#define config_public_

#include "compat.h"

#include "gamecontrol.h"
#include "keyboard.h"
#include "control.h"

BEGIN_SW_NS

#define SETUPNAMEPARM "SETUPFILE"

// screen externs
extern int32_t ScreenBufferMode;
extern int32_t VesaBufferMode;


#if 0
// comm externs
extern int32_t ComPort;
extern int32_t IrqNumber;
extern int32_t UartAddress;
extern int32_t PortSpeed;


extern int32_t ToneDial;
extern char  ModemName[MAXMODEMSTRING];
extern char  InitString[MAXMODEMSTRING];
extern char  HangupString[MAXMODEMSTRING];
extern char  DialoutString[MAXMODEMSTRING];
extern int32_t SocketNumber;
extern char  PhoneNames[MAXPHONEENTRIES][PHONENAMELENGTH];
extern char  PhoneNumbers[MAXPHONEENTRIES][PHONENUMBERLENGTH];
extern char  PhoneNumber[PHONENUMBERLENGTH];
extern int32_t NumberPlayers;
extern int32_t ConnectType;
extern char  PlayerName[MAXPLAYERNAMELENGTH];
extern char  RTSName[MAXRTSNAMELENGTH];
extern char  UserLevel[MAXUSERLEVELNAMELENGTH];
extern char  RTSPath[MAXRTSPATHLENGTH];
extern char  UserPath[MAXUSERLEVELPATHLENGTH];

#endif
// controller externs
extern int32_t UseMouse, UseJoystick;

extern int32_t EnableRudder;

extern char setupfilename[BMAX_PATH];
extern char ExternalControlFilename[64];


int32_t CONFIG_ReadSetup(void);
void WriteCommitFile(int32_t gametype);
void CONFIG_GetSetupFilename(void);


END_SW_NS
#endif
