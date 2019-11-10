#pragma once

#include "drawparms.h"
#include "c_cvars.h"
// Undo Windows's forced #defines
#ifdef DrawText
#undef DrawText
#endif

extern int32_t xdim, ydim;
struct ScreenDummy
{
	static int GetWidth() { return xdim; }
	static int GetHeight() { return ydim; }
};
extern ScreenDummy* screen;

int GetUIScale(int altval);
int GetConScale(int altval);


// [RH] Stretch values to make a 320x200 image best fit the screen
// without using fractional steppings
extern int CleanXfac, CleanYfac;

// [RH] Effective screen sizes that the above scale values give you
extern int CleanWidth, CleanHeight;

// Above minus 1 (or 1, if they are already 1)
extern int CleanXfac_1, CleanYfac_1, CleanWidth_1, CleanHeight_1;


bool SetTextureParms(DrawParms *parms, FTexture *img, double xx, double yy);
bool ParseDrawTextureTags(FTexture *img, double x, double y, uint32_t tag, Va_List& tags, DrawParms *parms, bool fortext);
void VirtualToRealCoords(double &x, double &y, double &w, double &h, double vwidth, double vheight, bool vbottom, bool handleaspect);
int ActiveFakeRatio(int width, int height);
float ActiveRatio(int width, int height, float* trueratio);
int CheckRatio(int width, int height, int* trueratio);
int AspectBaseWidth(float aspect);;
int AspectBaseHeight(float aspect);
double AspectPspriteOffset(float aspect);
int AspectMultiplier(float aspect);
bool AspectTallerThanWide(float aspect);
void ScaleWithAspect(int& w, int& h, int Width, int Height);

void DrawTexture(F2DDrawer *drawer, FTexture* img, double x, double y, int tags_first, ...);
void DrawChar (F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, int character, int tag_first, ...);
void DrawText(F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, const char *string, int tag_first, ...);
void DrawText(F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, const char32_t *string, int tag_first, ...);

EXTERN_CVAR(Int, con_scaletext)		// Scale notify text at high resolutions?
EXTERN_CVAR(Int, con_scale)

inline int active_con_scaletext(bool newconfont = false)
{
	return newconfont ? GetConScale(con_scaletext) : GetUIScale(con_scaletext);
}

inline int active_con_scale()
{
	return GetConScale(con_scale);
}

