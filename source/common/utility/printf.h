#pragma once

// This header collects all things printf.
// EDuke32 had two totally separate output paths and all the added code from G/ZDoom uses yet another means.
// Everything goes to the console now, but to avoid changing everything, this redirects all output to the console, with the proper settings.
// Changing all this would mean altering over 1000 lines of code which would add a needless complication to merging from upstream.


#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
#endif

// game print flags
enum
{
	PRINT_LOW,		// pickup messages
	PRINT_MEDIUM,	// death messages
	PRINT_HIGH,		// critical messages
	PRINT_CHAT,		// chat messages
	PRINT_TEAMCHAT,	// chat messages from a teammate
	PRINT_LOG,		// only to logfile
	PRINT_BOLD = 200,				// What Printf_Bold used
	PRINT_TYPES = 1023,		// Bitmask.
	PRINT_NONOTIFY = 1024,	// Flag - do not add to notify buffer
	PRINT_NOLOG = 2048,		// Flag - do not print to log file
};

enum
{
	DMSG_OFF,		// no developer messages.
	DMSG_ERROR,		// general notification messages
	DMSG_WARNING,	// warnings
	DMSG_NOTIFY,	// general notification messages
	DMSG_SPAMMY,	// for those who want to see everything, regardless of its usefulness.
};


void I_Error(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));

int PrintString (int iprintlevel, const char *outline);
int Printf (int printlevel, const char *format, ...) ATTRIBUTE((format(printf,2,3)));
int Printf (const char *format, ...) ATTRIBUTE((format(printf,1,2)));
int DPrintf (int level, const char *format, ...) ATTRIBUTE((format(printf,2,3)));


void OSD_Printf(const char *format, ...) ATTRIBUTE((format(printf,1,2)));

template<class... Args>
inline void initprintf(const char *format, Args&&... args) ATTRIBUTE((format(printf,1,2)))
{
	OSD_Printf(format, std::forward<Args>(args)...);
}

// This was a define before - which should be avoided. Used by Shadow Warrior
template<class... Args>
inline void buildprintf(const char *format, Args&&... args) ATTRIBUTE((format(printf,1,2)))
{
	OSD_Printf(format, std::forward<Args>(args)...);
}

inline void initputs(const char *s)
{
	PrintString(PRINT_HIGH, s);
}

inline void buildputs(const char *s)
{
	PrintString(PRINT_HIGH, s);
}

void debugprintf(const char* f, ...);	// Prints to the debugger's log.
