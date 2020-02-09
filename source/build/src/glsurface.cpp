/*
 * glsurface.cpp
 *  A 32-bit rendering surface that can quickly blit 8-bit paletted buffers implemented in OpenGL.
 *
 * Copyright � 2018, Alex Dawson. All rights reserved.
 */

#ifdef NO_IBUILD

#include "glsurface.h"

#include "baselayer.h"
#include "build.h"
#include "tarray.h"
#include "flatvertices.h"
#include "../../glbackend/glbackend.h"

static TArray<uint8_t> buffer;
static FHardwareTexture* bufferTexture;
static vec2_t bufferRes;

static FHardwareTexture* paletteTexture;

bool glsurface_initialize(vec2_t bufferResolution)
{
    if (buffer.Size())
        glsurface_destroy();

    bufferRes = bufferResolution;
	buffer.Resize(bufferRes.x * bufferRes.y);

	bufferTexture = GLInterface.NewTexture();
	bufferTexture->CreateTexture(bufferRes.x, bufferRes.y, FHardwareTexture::Indexed, false);

    glsurface_setPalette(curpalettefaded);
	GLInterface.SetSurfaceShader();
    return true;
}

void glsurface_destroy()
{
	if (bufferTexture) delete bufferTexture;
	bufferTexture = nullptr;
	if (paletteTexture) delete paletteTexture;
	paletteTexture = nullptr;
}

void glsurface_setPalette(void* pPalette)
{
    if (!buffer.Size())
        return;
    if (!pPalette)
        return;

	if (!paletteTexture)
	{
		paletteTexture = GLInterface.NewTexture();
		paletteTexture->CreateTexture(256, 1, FHardwareTexture::TrueColor, false);
	}
	paletteTexture->LoadTexture(palette);
	GLInterface.BindTexture(1, paletteTexture, SamplerNoFilterClampXY);
}

void* glsurface_getBuffer()
{
    return buffer.Data();
}

vec2_t glsurface_getBufferResolution()
{
    return bufferRes;
}

void glsurface_blitBuffer()
{
	if (!buffer.Size())
		return;

	bufferTexture->LoadTexture(buffer.Data());
	GLInterface.BindTexture(0, bufferTexture, SamplerNoFilterClampXY);
	GLInterface.Draw(DT_TRIANGLE_STRIP, FFlatVertexBuffer::PRESENT_INDEX, 4);
}

#endif
