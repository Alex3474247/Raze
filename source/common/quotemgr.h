#pragma once

#include "zstring.h"
#include "gstrings.h"
// Reimplementation of the Duke quote string buffer.
// Needed because these strings need a level of abstraction from the engine to allow localization
// and because some of the quotes are really status messages that need to be shared between the different games.
// Also a good opportunity to consolidate the data buffers from Duke and Redneck frontends.

enum
{
	MAXQUOTES = 16384,
};

class Quotes
{
	FString quotes[MAXQUOTES];
	FString exquotes[MAXQUOTES];
	
	void MakeStringLabel(FString &quote);
	
public:

	void InitializeQuote(int num, const char *text, bool fromscript = false);
	void InitializeExQuote(int num, const char *text, bool fromscript = false);

	const char *GetQuote(int num)
	{
		return GStrings.localize(quotes[num]);
	}
	
	const char *GetExQuote(int num)
	{
		return GStrings.localize(exquotes[num]);
	}
	
	const char *GetRawQuote(int num)
	{
		return quotes[num];
	}
	
	const char *GetRawExQuote(int num)
	{
		return exquotes[num];
	}
	
	void CopyQuote(int dst, int src)
	{
		quotes[dst] = quotes[src];
	}

	void CopyExQuote(int dst, int src)
	{
		quotes[dst] = exquotes[src];
	}

	void AppendQuote(int dst, int src, int len = -1);
	void AppendExQuote(int dst, int src, int len = -1);
	void FormatQuote(int dst, const char* fmt, ...);
	void Substitute(int num, const char* text, const char* replc);
	void ReadFromSavegame();
	void WriteToSavegame();
};

extern Quotes quoteMgr;
