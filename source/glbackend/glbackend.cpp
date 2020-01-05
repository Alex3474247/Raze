/*
** glbackend.cpp
**
** OpenGL API abstraction
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
#include <memory>
#include "gl_load.h"
#include "glbackend.h"
#include "gl_samplers.h"
#include "gl_shader.h"
#include "textures.h"
#include "palette.h"
#include "imgui.h"
#include "gamecontrol.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "baselayer.h"
#include "gl_interface.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "gl_renderer.h"

float shadediv[MAXPALOOKUPS];

static int blendstyles[] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
static int renderops[] = { GL_FUNC_ADD, GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT };
int depthf[] = { GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL };

static TArray<VSMatrix> matrixArray;

FileReader GetResource(const char* fn)
{
	auto fr = fileSystem.OpenFileReader(fn, 0);
	if (!fr.isOpen())
	{
		I_Error("Fatal: '%s' not found", fn);
	}
	return fr;
}

GLInstance GLInterface;

GLInstance::GLInstance()
	:palmanager(this)
{

}

void ImGui_Init_Backend();
ImGuiContext* im_ctx;
TArray<uint8_t> ttf;

void GLInstance::Init(int ydim)
{
	if (!mSamplers)
	{
		mSamplers = new FSamplerManager;
	}

	//glinfo.bufferstorage =  !!strstr(glinfo.extensions, "GL_ARB_buffer_storage");
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);

	new(&renderState) PolymostRenderState;	// reset to defaults.
	LoadSurfaceShader();
	LoadVPXShader();
	LoadPolymostShader();
#if 0
	IMGUI_CHECKVERSION();
	im_ctx = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGui_Init_Backend();
	ImGui_ImplOpenGL3_Init();
	if (!ttf.Size())
	{
		//ttf = fileSystem.LoadFile("engine/Capsmall_clean.ttf", 0);
		ttf = fileSystem.LoadFile("engine/Roboto-Regular.ttf", 0);
	}
	if (ttf.Size()) io.Fonts->AddFontFromMemoryTTF(ttf.Data(), ttf.Size(), std::clamp(ydim / 40, 10, 30));
#endif
}

void GLInstance::LoadPolymostShader()
{
	auto fr1 = GetResource("engine/shaders/glsl/polymost.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetResource("engine/shaders/glsl/polymost.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	polymostShader = new PolymostShader();
	polymostShader->Load("PolymostShader", (const char*)Vert.Data(), (const char*)Frag.Data());
	SetPolymostShader();
}

void GLInstance::LoadVPXShader()
{
	auto fr1 = GetResource("engine/shaders/glsl/animvpx.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetResource("engine/shaders/glsl/animvpx.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	vpxShader = new FShader();
	vpxShader->Load("VPXShader", (const char*)Vert.Data(), (const char*)Frag.Data());
}

void GLInstance::LoadSurfaceShader()
{
	auto fr1 = GetResource("engine/shaders/glsl/glsurface.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetResource("engine/shaders/glsl/glsurface.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	surfaceShader = new SurfaceShader();
	surfaceShader->Load("SurfaceShader", (const char*)Vert.Data(), (const char*)Frag.Data());
}


void GLInstance::InitGLState(int fogmode, int multisample)
{
	glShadeModel(GL_SMOOTH);  // GL_FLAT
	glEnable(GL_TEXTURE_2D);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (multisample > 0 )
    {
		//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
        glEnable(GL_MULTISAMPLE);
    }
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	// This is a bad place to call this but without deconstructing the entire render loops in all front ends there is no way to have a well defined spot for this stuff.
	// Before doing that the backend needs to work in some fashion, so we have to make sure everything is set up when the first render call is performed.
	screen->BeginFrame();	
	bool useSSAO = (gl_ssao != 0);
    OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(useSSAO);
}

void GLInstance::Deinit()
{
#if 0
	if (im_ctx)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext(im_ctx);
	}
#endif
	if (mSamplers) delete mSamplers;
	mSamplers = nullptr;
	if (polymostShader) delete polymostShader;
	polymostShader = nullptr;
	if (surfaceShader) delete surfaceShader;
	surfaceShader = nullptr;
	if (vpxShader) delete vpxShader;
	vpxShader = nullptr;
	activeShader = nullptr;
	palmanager.DeleteAllTextures();
	lastPalswapIndex = -1;
}

FHardwareTexture* GLInstance::NewTexture()
{
	return new FHardwareTexture;
}

void GLInstance::ResetFrame()
{
	GLState s;
	lastState = s; // Back to defaults.
	lastState.Style.BlendOp = -1;	// invalidate. This forces a reset for the next operation

}

	
std::pair<size_t, BaseVertex *> GLInstance::AllocVertices(size_t num)
{
	Buffer.resize(num);
	return std::make_pair((size_t)0, Buffer.data());
}

static GLint primtypes[] =
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_LINES
};
	
void GLInstance::Draw(EDrawType type, size_t start, size_t count)
{
	// Todo: Based on the current tinting flags and the texture type (indexed texture and APPLYOVERPALSWAP not set)  this may have to reset the palette for the draw call / texture creation.
	bool applied = false;

	if (activeShader == polymostShader)
	{
		glVertexAttrib4fv(2, renderState.Color);
		if (renderState.Color[3] != 1.f) renderState.Flags &= ~RF_Brightmapping;	// The way the colormaps are set up means that brightmaps cannot be used on translucent content at all.
		renderState.Apply(polymostShader, lastState);
		if (renderState.VertexBuffer != LastVertexBuffer || LastVB_Offset[0] != renderState.VB_Offset[0] || LastVB_Offset[1] != renderState.VB_Offset[1])
		{
			if (renderState.VertexBuffer)
			{
				static_cast<OpenGLRenderer::GLVertexBuffer*>(renderState.VertexBuffer)->Bind(renderState.VB_Offset);
			}
			else glBindBuffer(GL_ARRAY_BUFFER, 0);
			LastVertexBuffer = renderState.VertexBuffer;
			LastVB_Offset[0] = renderState.VB_Offset[0];
			LastVB_Offset[1] = renderState.VB_Offset[1];
		}
		if (renderState.IndexBuffer != LastIndexBuffer)
		{
			if (renderState.IndexBuffer)
			{
				static_cast<OpenGLRenderer::GLIndexBuffer*>(renderState.IndexBuffer)->Bind();
			}
			else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			LastIndexBuffer = renderState.IndexBuffer;
		}
	}
	if (!LastVertexBuffer)
	{
		glBegin(primtypes[type]);
		auto p = &Buffer[start];
		for (size_t i = 0; i < count; i++, p++)
		{
			glVertexAttrib2f(1, p->u, p->v);
			glVertexAttrib3f(0, p->x, p->y, p->z);
		}
		glEnd();
	}
	else if (type != DT_LINES)
	{
		glDrawElements(primtypes[type], count, GL_UNSIGNED_INT, (void*)(intptr_t)(start * sizeof(uint32_t)));
	}
	else
	{
		glDrawArrays(primtypes[type], start, count);
	}
	if (MatrixChange)
	{
		if (MatrixChange & 1) SetIdentityMatrix(Matrix_Texture);
		if (MatrixChange & 2) SetIdentityMatrix(Matrix_Detail);
		MatrixChange = 0;
	}
	matrixArray.Resize(1);
}


void GLInstance::SetMatrix(int num, const VSMatrix *mat)
{
	if (num == Matrix_Projection) mProjectionM5 = mat->get()[5];
	renderState.matrixIndex[num] = matrixArray.Size();
	matrixArray.Push(*mat);
}

void GLInstance::ReadPixels(int xdim, int ydim, uint8_t* buffer)
{
	glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, buffer);
}

void GLInstance::SetPolymostShader()
{
	if (activeShader != polymostShader)
	{
		polymostShader->Bind();
		activeShader = polymostShader;
	}
}

void GLInstance::SetSurfaceShader()
{
	if (activeShader != surfaceShader)
	{
		surfaceShader->Bind();
		activeShader = surfaceShader;
	}
}

void GLInstance::SetVPXShader()
{
	if (activeShader != vpxShader)
	{
		vpxShader->Bind();
		activeShader = vpxShader;
	}
}

void GLInstance::SetPalette(int index)
{
	palmanager.BindPalette(index);
}


void GLInstance::SetPalswap(int index)
{
	palmanager.BindPalswap(index);
	renderState.ShadeDiv = shadediv[index] == 0 ? 1.f / (renderState.NumShades - 2) : shadediv[index];
}

void GLInstance::DrawImGui(ImDrawData* data)
{
#if 0
	ImGui_ImplOpenGL3_RenderDrawData(data);
#endif
}


void PolymostRenderState::Apply(PolymostShader* shader, GLState &oldState)
{
	bool reset = false;
	for (int i = 0; i < MAX_TEXTURES; i++)
	{
		if (texIds[i] != oldState.TexId[i] || samplerIds[i] != oldState.SamplerId[i])
		{
			if (i != 0)
			{
				glActiveTexture(GL_TEXTURE0 + i);
				reset = true;
			}
			glBindTexture(GL_TEXTURE_2D, texIds[i]);
			GLInterface.mSamplers->Bind(i, samplerIds[i], -1);
			oldState.TexId[i] = texIds[i];
			oldState.SamplerId[i] = samplerIds[i];
		}
		if (reset) glActiveTexture(GL_TEXTURE0);
	}
	if (StateFlags != oldState.Flags)
	{
		if ((StateFlags ^ oldState.Flags) & STF_DEPTHTEST)
		{
			if (StateFlags & STF_DEPTHTEST) glEnable(GL_DEPTH_TEST);
			else glDisable(GL_DEPTH_TEST);
		}
		if ((StateFlags ^ oldState.Flags) & STF_BLEND)
		{
			if (StateFlags & STF_BLEND) glEnable(GL_BLEND);
			else glDisable(GL_BLEND);
		}
		if ((StateFlags ^ oldState.Flags) & STF_MULTISAMPLE)
		{
			if (StateFlags & STF_MULTISAMPLE) glEnable(GL_MULTISAMPLE);
			else glDisable(GL_MULTISAMPLE);
		}
		if ((StateFlags ^ oldState.Flags) & (STF_STENCILTEST|STF_STENCILWRITE))
		{
			if (StateFlags & STF_STENCILWRITE)
			{
				glEnable(GL_STENCIL_TEST);
				glClear(GL_STENCIL_BUFFER_BIT);
				glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
				glStencilFunc(GL_ALWAYS, 1/*value*/, 0xFF);
			}
			else if (StateFlags & STF_STENCILTEST)
			{
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, 1/*value*/, 0xFF);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			}
			else
			{
				glDisable(GL_STENCIL_TEST);
			}
		}
		if ((StateFlags ^ oldState.Flags) & (STF_CULLCW | STF_CULLCCW))
		{
			if (StateFlags & (STF_CULLCW | STF_CULLCCW))
			{
				glFrontFace(StateFlags & STF_CULLCW ? GL_CW : GL_CCW);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK); // Cull_Front is not being used.
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}
		}
		if ((StateFlags ^ oldState.Flags) & STF_COLORMASK)
		{
			if (StateFlags & STF_COLORMASK) glColorMask(1, 1, 1, 1);
			else glColorMask(0, 0, 0, 0);
		}
		if ((StateFlags ^ oldState.Flags) & STF_DEPTHMASK)
		{
			if (StateFlags & STF_DEPTHMASK) glDepthMask(1);
			else glDepthMask(0);
		}
		if ((StateFlags ^ oldState.Flags) & STF_WIREFRAME)
		{
			glPolygonMode(GL_FRONT_AND_BACK, (StateFlags & STF_WIREFRAME) ? GL_LINE : GL_FILL);
		}
		if (StateFlags & (STF_CLEARCOLOR| STF_CLEARDEPTH))
		{
			glClearColor(ClearColor.r / 255.f, ClearColor.g / 255.f, ClearColor.b / 255.f, 1.f);
			int bit = 0;
			if (StateFlags & STF_CLEARCOLOR) bit |= GL_COLOR_BUFFER_BIT;
			if (StateFlags & STF_CLEARDEPTH) bit |= GL_DEPTH_BUFFER_BIT;
			glClear(bit);
		}
		if (StateFlags & STF_VIEWPORTSET)
		{
			glViewport(vp_x, vp_y, vp_w, vp_h);
		}
		if (StateFlags & STF_SCISSORSET)
		{
			if (sc_x > SHRT_MIN)
			{
				glScissor(sc_x, sc_y, sc_w, sc_h);
				glEnable(GL_SCISSOR_TEST);
			}
			else
				glDisable(GL_SCISSOR_TEST);
		}

		StateFlags &= ~(STF_CLEARCOLOR | STF_CLEARDEPTH | STF_VIEWPORTSET | STF_SCISSORSET);
		oldState.Flags = StateFlags;
	}
	if (Style != oldState.Style)
	{
		glBlendFunc(blendstyles[Style.SrcAlpha], blendstyles[Style.DestAlpha]);
		if (Style.BlendOp != oldState.Style.BlendOp) glBlendEquation(renderops[Style.BlendOp]);
		oldState.Style = Style;
		// Flags are not being checked yet, the current shader has no implementation for them.
	}
	if (DepthFunc != oldState.DepthFunc)
	{
		glDepthFunc(depthf[DepthFunc]);
		oldState.DepthFunc = DepthFunc;
	}
	// Disable brightmaps if non-black fog is used.
	if (!(Flags & RF_FogDisabled) && !FogColor.isBlack()) Flags &= ~RF_Brightmapping;
	shader->Flags.Set(Flags);
	shader->Shade.Set(Shade);
	shader->NumShades.Set(NumShades);
	shader->ShadeDiv.Set(ShadeDiv);
	shader->VisFactor.Set(VisFactor);
	shader->Flags.Set(Flags);
	shader->NPOTEmulationFactor.Set(NPOTEmulationFactor);
	shader->NPOTEmulationXOffset.Set(NPOTEmulationXOffset);
	shader->AlphaThreshold.Set(AlphaTest ? AlphaThreshold : -1.f);
	shader->Brightness.Set(Brightness);
	shader->FogColor.Set(FogColor);
	if (matrixIndex[Matrix_View] != -1)
		shader->RotMatrix.Set(matrixArray[matrixIndex[Matrix_View]].get());
	if (matrixIndex[Matrix_Projection] != -1)
		shader->ProjectionMatrix.Set(matrixArray[matrixIndex[Matrix_Projection]].get());
	if (matrixIndex[Matrix_ModelView] != -1)
		shader->ModelMatrix.Set(matrixArray[matrixIndex[Matrix_ModelView]].get());
	if (matrixIndex[Matrix_Detail] != -1)
		shader->DetailMatrix.Set(matrixArray[matrixIndex[Matrix_Detail]].get());
	if (matrixIndex[Matrix_Texture] != -1)
		shader->TextureMatrix.Set(matrixArray[matrixIndex[Matrix_Texture]].get());
	memset(matrixIndex, -1, sizeof(matrixIndex));
}

