//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"

#include "PlatformHeaders.h"
#include "SDL2/SDL_opengl.h"

#include "com_model.h"
#include "r_studioint.h"

// RENDERERS START
#include "bsprenderer.h"
#include "propmanager.h"
#include "particle_engine.h"
#include "watershader.h"
#include "mirrormanager.h"

#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

extern CGameStudioModelRenderer g_StudioRenderer;
// RENDERERS END

extern IParticleMan* g_pParticleMan;

extern engine_studio_api_s IEngineStudio;

uint g_uiScreenTex = 0, g_uiGlowTex = 0;

uint ScreenToScope = 0;
uint g_iBlankTex = 0;

void GenBlackTex()
{
	GLubyte pixels[3] = {0, 0, 0};

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &g_iBlankTex);
	glBindTexture(GL_TEXTURE_2D, g_iBlankTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}


/*
640 360
640 480
*/
void InitScreenCatch()
{
	if (ScreenToScope != 0)
		return;

	GenBlackTex();

	glGenTextures(1, &ScreenToScope);

	unsigned char* ScreenBlank = new unsigned char[1024 * 1024 * 3];
	memset(ScreenBlank, 0, 1024 * 1024 * 3);
	glBindTexture(GL_TEXTURE_2D, ScreenToScope);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, ScreenBlank);
	glBindTexture(GL_TEXTURE_2D, 0);
	delete[] ScreenBlank;
}


void CatchScreenToScope()
{
	InitScreenCatch();

	int cx, cy;
	int CthTs = ScreenWidth / 4;

	cx = ScreenWidth / 2;
	cy = ScreenHeight / 2;

	glBindTexture(GL_TEXTURE_2D, ScreenToScope);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cx - CthTs / 2, cy - CthTs / 2, CthTs, CthTs, 0);
}

void InitCatch()
{
	if (g_uiScreenTex != 0)
		return;

	glGenTextures(1, &g_uiScreenTex);

// create a load of blank pixels to create textures with
	unsigned char* pBlankTex = new unsigned char[ScreenWidth * ScreenHeight * 3];
	memset(pBlankTex, 0, ScreenWidth * ScreenHeight * 3);

// Create the SCREEN-HOLDING TEXTURE
	glGenTextures(1, &g_uiScreenTex);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB8, ScreenWidth, ScreenHeight, 0, GL_RGB8, GL_UNSIGNED_BYTE, pBlankTex);

// Create the BLURRED TEXTURE
	glGenTextures(1, &g_uiGlowTex);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiGlowTex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB8, ScreenWidth / 2, ScreenHeight / 2, 0, GL_RGB8, GL_UNSIGNED_BYTE, pBlankTex);

     // free the memory
	delete[] pBlankTex;
}

void DrawQuad(int width, int height, int ofsX = 0, int ofsY = 0)
{
	glTexCoord2f(ofsX, ofsY);
	glVertex3f(0, 1, -1);
	glTexCoord2f(ofsX, height + ofsY);
	glVertex3f(0, 0, -1);
	glTexCoord2f(width + ofsX, height + ofsY);
	glVertex3f(1, 0, -1);
	glTexCoord2f(width + ofsX, ofsY);
	glVertex3f(1, 1, -1);
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	//	RecClDrawNormalTriangles();
	// RENDERERS START
	// 2012-02-25
	R_DrawNormalTriangles();
	// RENDERERS END

	gHUD.m_Spectator.DrawOverview();
}

void RenderBlur()
{
	if (gHUD.flDOFBlurAlpha == 0.0f)
		return;

	int i = 0;

	// enable some OpenGL stuff
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glColor3f(1, 1, 1);
	glDisable(GL_DEPTH_TEST);

// STEP 1: Grab the screen and put it into a texture

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth, ScreenHeight, 0);

// STEP 2: Set up an orthogonal projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0.1, 100);

// STEP 3: Render the current scene to a new, lower-res texture, darkening non-bright areas of the scene
	// by multiplying it with itself a few times.

	glViewport(0, 0, ScreenWidth / 2, ScreenHeight / 2);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth, ScreenHeight);
	glEnd();

	glEnable(GL_BLEND);

//	glBegin(GL_QUADS);
//	for (int i = 0; i < (int)gEngfuncs.pfnGetCvarFloat("glow_darken_steps"); i++)
//		DrawQuad(ScreenWidth, ScreenHeight);
//	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiGlowTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth / 2, ScreenHeight / 2, 0);

// STEP 4: Blur the now darkened scene in the horizontal direction.

	float blurAlpha = 1 / ((2.0f) * 2 + 1);

	glColor4f(1, 1, 1, blurAlpha);

	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	glEnd();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glBegin(GL_QUADS);
	for (i = 1; i <= 2; i++)
	{
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, -i, 0);
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, i, 0);
	}
	glEnd();

	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth / 2, ScreenHeight / 2, 0);

// STEP 5: Blur the horizontally blurred image in the vertical direction.

	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	glEnd();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glBegin(GL_QUADS);
	for (i = 1; i <= 2; i++)
	{
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, 0, -i);
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, 0, i);
	}
	glEnd();

	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth / 2, ScreenHeight / 2, 0);

// STEP 6: Combine the blur with the original image.

	glViewport(0, 0, ScreenWidth, ScreenHeight);

	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	for (size_t i = 0; i < 2; i++)
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f - gHUD.flDOFBlurAlpha);
	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth, ScreenHeight);
	glEnd();
	glDisable(GL_BLEND);

     // STEP 7: Restore the original projection and modelview matrices and disable rectangular textures.

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable(GL_TEXTURE_RECTANGLE_NV);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
	//	RecClDrawTransparentTriangles();

// RENDERERS START
	// 2012-02-25
	R_DrawTransparentTriangles();
	// RENDERERS END

	//IEngineStudio.SetupRenderer(kRenderNormal);
	CatchScreenToScope();
	//IEngineStudio.RestoreRenderer();
	RenderBlur();

	if (g_pParticleMan)
		g_pParticleMan->Update();
}
