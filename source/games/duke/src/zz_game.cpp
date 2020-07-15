﻿//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#include "ns.h"	// Must come before everything else!

#define game_c_

#include "duke3d.h"
#include "compat.h"
#include "baselayer.h"
#include "savegame.h"

#include "sbar.h"
#include "palette.h"
#include "gamecvars.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "filesystem.h"
#include "statistics.h"
#include "c_dispatch.h"
#include "mapinfo.h"
#include "v_video.h"
#include "glbackend/glbackend.h"
#include "st_start.h"
#include "i_interface.h"

// Uncomment to prevent anything except mirrors from drawing. It is sensible to
// also uncomment ENGINE_CLEAR_SCREEN in build/src/engine_priv.h.
//#define DEBUG_MIRRORS_ONLY

BEGIN_DUKE_NS

void SetDispatcher();
void InitCheats();
void checkcommandline();
int registerosdcommands(void);
int32_t moveloop(void);
int menuloop(void);
void advancequeue(int myconnectindex);
input_t& nextinput(int myconnectindex);

int16_t max_ammo_amount[MAX_WEAPONS];
int32_t spriteqamount = 64;

uint8_t shadedsector[MAXSECTORS];

int32_t cameradist = 0, cameraclock = 0;

int32_t g_Shareware = 0;

int32_t tempwallptr;
int32_t      actor_tog;

static int32_t nonsharedtimer;
weaponhit hittype[MAXSPRITES];
ActorInfo actorinfo[MAXTILES];
player_struct ps[MAXPLAYERS];

static void gameTimerHandler(void)
{
    S_Update();

    // we need CONTROL_GetInput in order to pick up joystick button presses
    if (!(ps[myconnectindex].gm & MODE_GAME))
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
        C_RunDelayedCommands();
    }
}

void G_InitTimer(int32_t ticspersec)
{
    if (g_timerTicsPerSecond != ticspersec)
    {
        timerUninit();
        timerInit(ticspersec);
        g_timerTicsPerSecond = ticspersec;
    }
}



void G_HandleLocalKeys(void)
{
//    CONTROL_ProcessBinds();

    if (ud.recstat == 2)
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    if (!ALT_IS_PRESSED && ud.overhead_on == 0 && (ps[myconnectindex].gm & MODE_TYPE) == 0)
    {
        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
        {
            buttonMap.ClearButton(gamefunc_Enlarge_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(1))
				{
					S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale + 4;
            }
        }

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(-1))
				{
					S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale - 4;
            }
        }
    }

    if ((ps[myconnectindex].gm&(MODE_MENU|MODE_TYPE)) || System_WantGuiCapture())
        return;

    if (buttonMap.ButtonDown(gamefunc_See_Coop_View) && (ud.coop || ud.recstat == 2))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = 0;
    }

    if ((ud.multimode > 1) && buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
        ud.ShowOpponentWeapons = ud.showweapons = 1-ud.showweapons;
        FTA(QUOTE_WEAPON_MODE_OFF-ud.showweapons,&ps[screenpeek]);
    }

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
    {
        buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
        cl_crosshair = !cl_crosshair;
        FTA(QUOTE_CROSSHAIR_OFF-cl_crosshair,&ps[screenpeek]);
    }

    if (ud.overhead_on && buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
    {
        buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
        ud.scrollmode = 1-ud.scrollmode;
        if (ud.scrollmode)
        {
            ud.folx = ps[screenpeek].oposx;
            ud.foly = ps[screenpeek].oposy;
            ud.fola = fix16_to_int(ps[screenpeek].oq16ang);
        }
        FTA(QUOTE_MAP_FOLLOW_OFF+ud.scrollmode,&ps[myconnectindex]);
    }


    if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED || WIN_IS_PRESSED)
    {
        int ridiculeNum = 0;

        // NOTE: sc_F1 .. sc_F10 are contiguous. sc_F11 is not sc_F10+1.
        for (bssize_t j=sc_F1; j<=sc_F10; j++)
            if (inputState.UnboundKeyPressed(j))
            {
                inputState.ClearKeyStatus(j);
                ridiculeNum = j - sc_F1 + 1;
                break;
            }

        if (ridiculeNum)
        {
            if (SHIFTS_IS_PRESSED)
            {
                Printf(PRINT_NOTIFY, *CombatMacros[ridiculeNum-1]);
				//Net_SendTaunt(ridiculeNum);
                return;
            }

            // Not SHIFT -- that is, either some ALT or WIN.
            if (startrts(ridiculeNum, 1))
            {
				//Net_SendRTS(ridiculeNum);
                return;
            }
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
        {
            buttonMap.ClearButton(gamefunc_Third_Person_View);

            if (!isRRRA() || (!ps[myconnectindex].OnMotorcycle && !ps[myconnectindex].OnBoat))
            {
                ps[myconnectindex].over_shoulder_on = !ps[myconnectindex].over_shoulder_on;

                cameradist  = 0;
                cameraclock = (int32_t) totalclock;

                FTA(QUOTE_VIEW_MODE_OFF + ps[myconnectindex].over_shoulder_on, &ps[myconnectindex]);
            }
        }

        if (ud.overhead_on != 0)
        {
            int const timerOffset = ((int) totalclock - nonsharedtimer);
            nonsharedtimer += timerOffset;

            if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
                ps[myconnectindex].zoom += mulscale6(timerOffset, max<int>(ps[myconnectindex].zoom, 256));

            if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
                ps[myconnectindex].zoom -= mulscale6(timerOffset, max<int>(ps[myconnectindex].zoom, 256));

            ps[myconnectindex].zoom = clamp(ps[myconnectindex].zoom, 48, 2048);
        }
    }

#if 0 // fixme: We should not query Esc here, this needs to be done differently
    if (I_EscapeTrigger() && ud.overhead_on && ps[myconnectindex].newowner == -1)
    {
        I_EscapeTriggerClear();
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on   = 0;
        ud.scrollmode    = 0;
    }
#endif

    if (buttonMap.ButtonDown(gamefunc_Map))
    {
        buttonMap.ClearButton(gamefunc_Map);
        if (ud.last_overhead != ud.overhead_on && ud.last_overhead)
        {
            ud.overhead_on = ud.last_overhead;
            ud.last_overhead = 0;
        }
        else
        {
            ud.overhead_on++;
            if (ud.overhead_on == 3) ud.overhead_on = 0;
            ud.last_overhead = ud.overhead_on;
        }
    }
}


static int parsedefinitions_game(scriptfile *, int);


static void G_Cleanup(void)
{
    int32_t i;

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        Xfree(g_player[i].input);
    }
}

/*
===================
=
= G_Startup
=
===================
*/

inline int G_CheckPlayerColor(int color)
{
    static int32_t player_pals[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
    if (color >= 0 && color < 10) return player_pals[color];
    return 0;
}


static void G_Startup(void)
{
    timerInit(TICRATE);
    timerSetCallback(gameTimerHandler);

    loadcons();
    fi.initactorflags();

    if (IsGameEvent(EVENT_INIT))
    {
        SetGameVarID(g_iReturnVarID, -1, -1, -1);
        OnEvent(EVENT_INIT);
    }

    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (engineInit())
        G_FatalEngineError();

    // These depend on having the dynamic tile and/or sound mappings set up:
    setupbackdrop();
    //Net_SendClientInfo();

	if (userConfig.CommandMap.IsNotEmpty())
	{
        if (VOLUMEONE)
        {
            Printf("The -map option is available in the registered version only!\n");
			userConfig.CommandMap = "";
        }
    }

    if (numplayers > 1)
        Printf("Multiplayer initialized.\n");

    if (TileFiles.artLoadFiles("tiles%03i.art") < 0)
        I_FatalError("Failed loading art.");

    fi.InitFonts();

    // Make the fullscreen nuke logo background non-fullbright.  Has to be
    // after dynamic tile remapping (from C_Compile) and loading tiles.
    picanm[TILE_LOADSCREEN].sf |= PICANM_NOFULLBRIGHT_BIT;

//    Printf("Loading palette/lookups...\n");
    genspriteremaps();
    TileFiles.PostLoadSetup();

    screenpeek = myconnectindex;
}

static void P_SetupMiscInputSettings(void)
{
    struct player_struct *pp = &ps[myconnectindex];

    pp->aim_mode = in_mousemode;
    pp->auto_aim = cl_autoaim;
    pp->weaponswitch = cl_weaponswitch;
}

void G_UpdatePlayerFromMenu(void)
{
    if (ud.recstat != 0)
        return;

    if (numplayers > 1)
    {
        //Net_SendClientInfo();
        if (sprite[ps[myconnectindex].i].picnum == TILE_APLAYER && sprite[ps[myconnectindex].i].pal != 1)
            sprite[ps[myconnectindex].i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        P_SetupMiscInputSettings();
        ps[myconnectindex].palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);

        g_player[myconnectindex].pteam = playerteam;

        if (sprite[ps[myconnectindex].i].picnum == TILE_APLAYER && sprite[ps[myconnectindex].i].pal != 1)
            sprite[ps[myconnectindex].i].pal = g_player[myconnectindex].pcolor;
    }
}

void G_BackToMenu(void)
{
    ps[myconnectindex].gm = 0;
	M_StartControlPanel(false);
	M_SetMenu(NAME_Mainmenu);
	inputState.keyFlushChars();
}

void G_MaybeAllocPlayer(int32_t pnum)
{
    if (g_player[pnum].input == NULL)
        g_player[pnum].input = (input_t *)Xcalloc(1, sizeof(input_t));
}

void app_loop();

// TODO: reorder (net)weaponhit to eliminate slop and update assertion
EDUKE32_STATIC_ASSERT(sizeof(weaponhit)%4 == 0);

static const char* actions[] = {
    "Move_Forward",
    "Move_Backward",
    "Turn_Left",
    "Turn_Right",
    "Strafe",
    "Fire",
    "Open",
    "Run",
    "Alt_Fire",	// Duke3D", Blood
    "Jump",
    "Crouch",
    "Look_Up",
    "Look_Down",
    "Look_Left",
    "Look_Right",
    "Strafe_Left",
    "Strafe_Right",
    "Aim_Up",
    "Aim_Down",
    "Weapon_1",
    "Weapon_2",
    "Weapon_3",
    "Weapon_4",
    "Weapon_5",
    "Weapon_6",
    "Weapon_7",
    "Weapon_8",
    "Weapon_9",
    "Weapon_10",
    "Inventory",
    "Inventory_Left",
    "Inventory_Right",
    "Holo_Duke",			// Duke3D", isRR()
    "Jetpack",
    "NightVision",
    "MedKit",
    "TurnAround",
    "SendMessage",
    "Map",
    "Shrink_Screen",
    "Enlarge_Screen",
    "Center_View",
    "Holster_Weapon",
    "Show_Opponents_Weapon",
    "Map_Follow_Mode",
    "See_Coop_View",
    "Mouse_Aiming",
    "Toggle_Crosshair",
    "Steroids",
    "Quick_Kick",
    "Next_Weapon",
    "Previous_Weapon",
    "Dpad_Select",
    "Dpad_Aiming",
    "Last_Weapon",
    "Alt_Weapon",
    "Third_Person_View",
    "Show_DukeMatch_Scores",
    "Toggle_Crouch",	// This is the last one used by EDuke32.
};

int GameInterface::app_main()
{
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        for (int j = 0; j < 10; j++)    
        {
            const char* s = "3457860291";
            ud.wchoice[i][j] = s[j] - '0';
        }
    }

    SetDispatcher();
    buttonMap.SetButtons(actions, NUM_ACTIONS);
    g_skillCnt = 4;
    ud.multimode = 1;
	ud.m_monsters_off = userConfig.nomonsters;

    g_movesPerPacket = 1;
    //bufferjitter = 1;
    //initsynccrc();

    // This needs to happen before G_CheckCommandLine() because G_GameExit()
    // accesses g_player[0].
    G_MaybeAllocPlayer(0);

    checkcommandline();

    ps[0].aim_mode = 1;
    ud.ShowOpponentWeapons = 0;
    ud.camerasprite = -1;
    ud.camera_time = 0;//4;
    playerteam = 0;

    S_InitSound();

    
    if (isRR())
    {
        g_cdTrack = -1;
    }

    InitCheats();

    if (VOLUMEONE)
        g_Shareware = 1;
    else
    {
		if (fileSystem.FileExists("DUKESW.BIN")) // JBF 20030810
        {
            g_Shareware = 1;
        }
    }

    numplayers = 1;
    playerswhenstarted = ud.multimode;

    connectpoint2[0] = -1;

    //Net_GetPackets();

    for (bssize_t i=0; i<MAXPLAYERS; i++)
        G_MaybeAllocPlayer(i);

    G_Startup(); // a bunch of stuff including compiling cons

    ps[myconnectindex].palette = BASEPAL;

    for (int i=1, j=numplayers; j<ud.multimode; j++)
    {
        Bsprintf(g_player[j].user_name,"%s %d", GStrings("PLAYER"),j+1);
        g_player[j].pteam = i;
        ps[j].weaponswitch = 3;
        ps[j].auto_aim = 0;
        i = 1-i;
    }

    const char *defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        Printf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
    }

	userConfig.AddDefs.reset();

    enginePostInit();
    hud_size.Callback();
    hud_scale.Callback();

    tileDelete(TILE_MIRROR);
    skiptile = TILE_W_FORCEFIELD + 1;

    if (isRR())
        tileDelete(0);

    tileDelete(13);

    // getnames();

    if (ud.multimode > 1)
    {
        ud.m_monsters_off = 1;
        ud.m_player_skill = 0;
    }

    playerswhenstarted = ud.multimode;  // XXX: redundant?

    ud.last_level = -1;
    registerosdcommands();

    videoInit();
    V_LoadTranslations();
    videoSetPalette(BASEPAL);

    FX_StopAllSounds();
	app_loop();
	return 0;
}
	
void app_loop()
{

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    ps[myconnectindex].ftq = 0;

    //if (ud.warp_on == 0)
    {
#if 0 // fixme once the game loop has been done.
        if ((ud.multimode > 1) && startupMap.IsNotEmpty())
        {
            auto maprecord = FindMap(startupMap);
            ud.m_respawn_monsters = ud.m_player_skill == 4;

            for (int i = 0; i != -1; i = connectpoint2[i])
            {
                resetweapons(i);
                resetinventory(i);
            }

            StartGame(maprecord);
        }
        else
#endif
        {
            fi.ShowLogo([](bool) {});
        }

        M_StartControlPanel(false);
		M_SetMenu(NAME_Mainmenu);
		if (menuloop())
        {
            FX_StopAllSounds();
            goto MAIN_LOOP_RESTART;
        }
    }

    ud.showweapons = ud.ShowOpponentWeapons;
    P_SetupMiscInputSettings();
    g_player[myconnectindex].pteam = playerteam;

    if (playercolor) ps[myconnectindex].palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);
    else ps[myconnectindex].palookup = g_player[myconnectindex].pcolor;

	inputState.ClearKeyStatus(sc_Pause);   // JBF: I hate the pause key

    do //main loop
    {
		handleevents();
		if (ps[myconnectindex].gm == MODE_DEMO)
		{
			M_ClearMenus();
			goto MAIN_LOOP_RESTART;
		}

        //Net_GetPackets();

        G_HandleLocalKeys();
 
        C_RunDelayedCommands();

        char gameUpdate = false;
        gameupdatetime.Reset();
        gameupdatetime.Clock();
        
        while ((!(ps[myconnectindex].gm & (MODE_MENU|MODE_DEMO))) && (int)(totalclock - ototalclock) >= TICSPERFRAME)
        {
            ototalclock += TICSPERFRAME;

            if (isRRRA() && ps[myconnectindex].OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (isRRRA() && ps[myconnectindex].OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            // this is where we fill the input_t struct that is actually processed by P_ProcessInput()
            auto const pPlayer = &ps[myconnectindex];
            auto const q16ang  = fix16_to_int(pPlayer->q16ang);
            auto& input = nextinput(myconnectindex);

            input = localInput;
            input.fvel = mulscale9(localInput.fvel, sintable[(q16ang + 2560) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 2048) & 2047]) +
                         pPlayer->fric.x;
            input.svel = mulscale9(localInput.fvel, sintable[(q16ang + 2048) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 1536) & 2047]) +
                         pPlayer->fric.y;
            localInput = {};

            advancequeue(myconnectindex);

            if (((!System_WantGuiCapture() && (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || (ud.multimode > 1)) &&
                    (ps[myconnectindex].gm&MODE_GAME))
            {
                moveloop();
            }
        }

        gameUpdate = true;
        gameupdatetime.Unclock();

        if (ps[myconnectindex].gm & (MODE_EOL|MODE_RESTART))
        {
            switch (exitlevel())
            {
                case 1: continue;
                case 2: goto MAIN_LOOP_RESTART;
            }
        }

        
        if (G_FPSLimit())
        {
            if (isRRRA() && ps[myconnectindex].OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (isRRRA() && ps[myconnectindex].OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            int const smoothRatio = calc_smoothratio(totalclock, ototalclock);

            drawtime.Reset();
            drawtime.Clock();
            displayrooms(screenpeek, smoothRatio);
            displayrest(smoothRatio);
            drawtime.Unclock();
            videoNextPage();
        }

        if (ps[myconnectindex].gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);
}

void GameInterface::FreeGameData()
{
    setmapfog(0);
    G_Cleanup();
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}


END_DUKE_NS
