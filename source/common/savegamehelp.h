#pragma once

#include "filesystem/resourcefile.h"

bool OpenSaveGameForWrite(const char *fname, const char *name);
bool OpenSaveGameForRead(const char *name);

FileWriter *WriteSavegameChunk(const char *name);
FileReader ReadSavegameChunk(const char *name);

bool FinishSavegameWrite();
void FinishSavegameRead();

// Savegame utilities
class FileReader;

FString G_BuildSaveName (const char *prefix);
int G_ValidateSavegame(FileReader &fr, FString *savetitle, bool formenu);

#define SAVEGAME_EXT ".dsave"

