/*
** i_music.cpp
** Plays music
**
**---------------------------------------------------------------------------
** Copyright 1998-2010 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#ifndef _WIN32
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <zlib.h>

#include "m_argv.h"
#include "filesystem.h"
#include "c_dispatch.h"
#include "templates.h"
#include "stats.h"
#include "cmdlib.h"
#include "c_cvars.h"
#include "c_console.h"
#include "v_text.h"
#include "i_soundfont.h"
#include "s_music.h"
#include "printf.h"
#include "timer.h"
#include "backend/i_sound.h"
#include "zmusic/zmusic.h"
#include "streamsources/streamsource.h"
#include "filereadermusicinterface.h"
#include "midisources/midisource.h"



void I_InitSoundFonts();
void S_SetStreamVolume(float);
void S_ParseSndInfo();

EXTERN_CVAR (Int, snd_samplerate)
EXTERN_CVAR (Int, snd_mididevice)

float relative_volume = 1.f, saved_relative_volume = 1.f;

#ifdef _WIN32

void I_InitMusicWin32();

#include "musicformats/win32/i_cd.h"
//==========================================================================
//
// CVAR: cd_drive
//
// Which drive (letter) to use for CD audio. If not a valid drive letter,
// let the operating system decide for us.
//
//==========================================================================
EXTERN_CVAR(Bool, cd_enabled); 
EXTERN_CVAR(Float, snd_mastervolume)

CUSTOM_CVAR(String, cd_drive, "", CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)
{
	if (cd_enabled && !Args->CheckParm("-nocdaudio")) CD_Enable(self);
}

//==========================================================================
//
// CVAR: cd_enabled
//
// Use the CD device? Can be overridden with -nocdaudio on the command line
//
//==========================================================================

CUSTOM_CVAR(Bool, cd_enabled, true, CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)
{
	if (self && !Args->CheckParm("-nocdaudio"))
		CD_Enable(cd_drive);
	else
		CD_Enable(nullptr);
}


#endif

//==========================================================================
//
// CVAR mus_volume
//
// Maximum volume of MOD/stream music.
//==========================================================================

CUSTOM_CVARD(Float, mus_volume, 0.5, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "controls music volume")
{
	if (self < 0) self = 0;
	else if (self > 1) self = 1;
	else
	{
		// Set general music volume.
		ChangeMusicSetting(ZMusic::snd_musicvolume, nullptr, self);
		if (GSnd != nullptr)
		{
			GSnd->SetMusicVolume(clamp<float>(self * relative_volume * snd_mastervolume, 0, 1));
		}
		// For music not implemented through the digital sound system,
		// let them know about the change.
		if (mus_playing.handle != nullptr)
		{
			ZMusic_VolumeChanged(mus_playing.handle);
		}
		else
		{ // If the music was stopped because volume was 0, start it now.
			S_RestartMusic();
		}
	}
}

CUSTOM_CVARD(Bool, mus_enabled, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables music")
{
	if (self) S_RestartMusic();
	else S_StopMusic(true);
}

//==========================================================================
//
// Callbacks for the music system.
//
//==========================================================================

static void tim_printfunc(int type, int verbosity_level, const char* fmt, ...)
{
	if (verbosity_level >= 3/*Timidity::VERB_DEBUG*/) return;	// Don't waste time on diagnostics.

	va_list args;
	va_start(args, fmt);
	FString msg;
	msg.VFormat(fmt, args);
	va_end(args);

	switch (type)
	{
	case 2:// Timidity::CMSG_ERROR:
		Printf(TEXTCOLOR_RED "%s\n", msg.GetChars());
		break;

	case 1://Timidity::CMSG_WARNING:
		Printf(TEXTCOLOR_YELLOW "%s\n", msg.GetChars());
		break;

	case 0://Timidity::CMSG_INFO:
		DPrintf(DMSG_SPAMMY, "%s\n", msg.GetChars());
		break;
	}
}

static void wm_printfunc(const char* wmfmt, va_list args)
{
	Printf(TEXTCOLOR_RED);
	VPrintf(PRINT_HIGH, wmfmt, args);
}

//==========================================================================
//
// other callbacks
//
//==========================================================================

static short* dumb_decode_vorbis_(int outlen, const void* oggstream, int sizebytes)
{
	// This really should be done internally in zmusic...
	return nullptr;// GSnd->DecodeSample(outlen, oggstream, sizebytes, CODEC_Vorbis);
}

static std::string mus_NicePath(const char* str)
{
	FString strv = NicePath(str);
	return strv.GetChars();
}

static const char* mus_pathToSoundFont(const char* sfname, int type)
{
	auto info = sfmanager.FindSoundFont(sfname, type);
	return info ? info->mFilename.GetChars() : nullptr;
}

static MusicIO::SoundFontReaderInterface* mus_openSoundFont(const char* sfname, int type)
{
	return sfmanager.OpenSoundFont(sfname, type);
}



//==========================================================================
//
//
//
//==========================================================================

void Mus_Init(void)
{
	I_InitSound();
    I_InitSoundFonts();
	S_ParseSndInfo();

	mus_volume.Callback ();

#ifdef _WIN32
	I_InitMusicWin32 ();
#endif // _WIN32
	snd_mididevice.Callback();
	
	Callbacks callbacks;

	callbacks.Fluid_MessageFunc = Printf;
	callbacks.Timidity_Messagefunc = tim_printfunc;
	callbacks.NicePath = mus_NicePath;
	callbacks.PathForSoundfont = mus_pathToSoundFont;
	callbacks.OpenSoundFont = mus_openSoundFont;
	//callbacks.DumbVorbisDecode = dumb_decode_vorbis_;

	ZMusic_SetCallbacks(&callbacks);
	timerSetCallback(S_UpdateMusic);
}


//==========================================================================
//
// 
//
//==========================================================================

void I_SetRelativeVolume(float vol)
{
	relative_volume = (float)vol;
	ChangeMusicSetting(ZMusic::relative_volume, nullptr, (float)vol);
	mus_volume.Callback();
}
//==========================================================================
//
// Sets relative music volume. Takes $musicvolume in SNDINFO into consideration
//
//==========================================================================

void I_SetMusicVolume (double factor)
{
	factor = clamp(factor, 0., 2.0);
	I_SetRelativeVolume((float)factor);
}

//==========================================================================
//
// test a relative music volume
//
//==========================================================================

CCMD(testmusicvol)
{
	if (argv.argc() > 1) 
	{
		I_SetRelativeVolume((float)strtod(argv[1], nullptr));
	}
	else
		Printf("Current relative volume is %1.2f\n", relative_volume);
}

//==========================================================================
//
// STAT music
//
//==========================================================================

ADD_STAT(music)
{
	if (mus_playing.handle != nullptr)
	{
		return FString(ZMusic_GetStats(mus_playing.handle).c_str());
	}
	return "No song playing";
}

//==========================================================================
//
// Common loader for the dumpers.
//
//==========================================================================

static MIDISource *GetMIDISource(const char *fn)
{
	FString src = fn;
	if (src.Compare("*") == 0) src = mus_playing.name;

	auto lump = fileSystem.FindFile(src);
	if (lump < 0)
	{
		Printf("Cannot find MIDI lump %s.\n", src.GetChars());
		return nullptr;
	}

	auto wlump = fileSystem.OpenFileReader(lump);

	uint32_t id[32 / 4];

	if (wlump.Read(id, 32) != 32 || wlump.Seek(-32, FileReader::SeekCur) != 0)
	{
		Printf("Unable to read lump %s\n", src.GetChars());
		return nullptr;
	}
	auto type = IdentifyMIDIType(id, 32);
	if (type == MIDI_NOTMIDI)
	{
		Printf("%s is not MIDI-based.\n", src.GetChars());
		return nullptr;
	}

	auto data = wlump.Read();
	auto source = CreateMIDISource(data.Data(), data.Size(), type);

	if (source == nullptr)
	{
		Printf("%s is not MIDI-based.\n", src.GetChars());
		return nullptr;
	}
	return source;
}

//==========================================================================
//
// CCMD writewave
//
// If the current song can be represented as a waveform, dump it to
// the specified file on disk. The sample rate parameter is merely a
// suggestion, and the dumper is free to ignore it.
//
//==========================================================================

CCMD (writewave)
{
	if (argv.argc() >= 3 && argv.argc() <= 7)
	{
		auto source = GetMIDISource(argv[1]);
		if (source == nullptr) return;

		EMidiDevice dev = MDEV_DEFAULT;

		if (argv.argc() >= 6)
		{
			if (!stricmp(argv[5], "Timidity") || !stricmp(argv[5], "Timidity++")) dev = MDEV_TIMIDITY;
			else if (!stricmp(argv[5], "FluidSynth")) dev = MDEV_FLUIDSYNTH;
			else if (!stricmp(argv[5], "OPL")) dev = MDEV_OPL;
			else
			{
				Printf("%s: Unknown MIDI device\n", argv[5]);
				return;
			}
		}
		// We must stop the currently playing music to avoid interference between two synths. 
		auto savedsong = mus_playing;
		S_StopMusic(true);
		if (dev == MDEV_DEFAULT && snd_mididevice >= 0) dev = MDEV_FLUIDSYNTH;	// The Windows system synth cannot dump a wave.
		try
		{
			MIDIDumpWave(source, dev, argv.argc() < 6 ? nullptr : argv[6], argv[2], argv.argc() < 4 ? 0 : (int)strtol(argv[3], nullptr, 10), argv.argc() < 5 ? 0 : (int)strtol(argv[4], nullptr, 10));
		}
		catch (const std::runtime_error& err)
		{
			Printf("MIDI dump failed: %s\n", err.what());
		}

		S_ChangeMusic(savedsong.name, savedsong.baseorder, savedsong.loop, true);
	}
	else
	{
		Printf ("Usage: writewave <midi> <filename> [subsong] [sample rate] [synth] [soundfont]\n"
		" - use '*' as song name to dump the currently playing song\n"
		" - use 0 for subsong and sample rate to play the default\n");
	}
}

//==========================================================================
//
// CCMD writemidi
//
// Writes a given MIDI song to disk. This does not affect playback anymore,
// like older versions did.
//
//==========================================================================

CCMD(writemidi)
{
	if (argv.argc() != 3)
	{
		Printf("Usage: writemidi <midisong> <filename> - use '*' as song name to dump the currently playing song\n");
		return;
	}
	auto source = GetMIDISource(argv[1]);
	if (source == nullptr) return;

	std::vector<uint8_t> midi;
	bool success;

	source->CreateSMF(midi, 1);
	auto f = FileWriter::Open(argv[2]);
	if (f == nullptr)
	{
		Printf("Could not open %s.\n", argv[2]);
		return;
	}
	success = (f->Write(&midi[0], midi.size()) == midi.size());
	delete f;

	if (!success)
	{
		Printf("Could not write to music file %s.\n", argv[2]);
	}
}
