/*
** c_dispatch.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#ifndef __C_DISPATCH_H__
#define __C_DISPATCH_H__

#include <functional>
#include "c_console.h"
#include "zstring.h"
#include "c_commandline.h"
#include "zstring.h"

class FConfigFile;

// Contains the contents of an exec'ed file
struct FExecList
{
	TArray<FString> Commands;

	void AddCommand(const char *cmd, const char *file = NULL);
	void ExecCommands() const;
};


extern bool CheckCheatmode (bool printmsg = true);

FExecList *C_ParseCmdLineParams(FExecList *exec);

// Add commands to the console as if they were typed in. Can handle wait
// and semicolon-separated commands. This function may modify the source
// string, but the string will be restored to its original state before
// returning. Therefore, commands passed must not be in read-only memory.
void AddCommandString (const char *text, int keynum=0);

void C_RunDelayedCommands();
void C_ClearDelayedCommands();

// Process a single console command. Does not handle wait.
void C_DoCommand (const char *cmd, int keynum=0);

FExecList *C_ParseExecFile(const char *file, FExecList *source);
bool C_ExecFile(const char *file);
void C_ClearDynCCmds();

// Write out alias commands to a file for all current aliases.
void C_ArchiveAliases (FConfigFile *f);

void C_SetAlias (const char *name, const char *cmd);
void C_ClearAliases ();

// build a single string out of multiple strings
FString BuildString (int argc, FString *argv);

typedef std::function<void(FCommandLine & argv, void *, int key)> CCmdRun;;

class FConsoleCommand
{
public:
	FConsoleCommand (const char *name, CCmdRun RunFunc);
	virtual ~FConsoleCommand ();
	virtual bool IsAlias ();
	void PrintCommand();

	virtual void Run (FCommandLine &args, void *instigator, int key);
	static FConsoleCommand* FindByName (const char* name);

	FConsoleCommand *m_Next, **m_Prev;
	FString m_Name;

	enum { HASH_SIZE = 251 };	// Is this prime?

protected:
	FConsoleCommand ();
	bool AddToHash (FConsoleCommand **table);

	CCmdRun m_RunFunc;

};

#define CCMD(n) \
	void Cmd_##n (FCommandLine &, void *, int key); \
	FConsoleCommand Cmd_##n##_Ref (#n, Cmd_##n); \
	void Cmd_##n (FCommandLine &argv, void *who, int key)

const int KEY_DBLCLICKED = 0x8000;

class FConsoleAlias : public FConsoleCommand
{
public:
	FConsoleAlias (const char *name, const char *command, bool noSave);
	~FConsoleAlias ();
	void Run (FCommandLine &args, void *instigator, int key);
	bool IsAlias ();
	void PrintAlias ();
	void Archive (FConfigFile *f);
	void Realias (const char *command, bool noSave);
	void SafeDelete ();
protected:
	FString m_Command[2];	// Slot 0 is saved to the ini, slot 1 is not.
	bool bDoSubstitution;
	bool bRunning;
	bool bKill;
};

class FUnsafeConsoleAlias : public FConsoleAlias
{
public:
	FUnsafeConsoleAlias (const char *name, const char *command)
	: FConsoleAlias (name, command, true)
	{
	}

	virtual void Run (FCommandLine &args, void *instigator, int key) override;
};


void execLogfile(const char *fn, bool append = false);

#endif //__C_DISPATCH_H__
