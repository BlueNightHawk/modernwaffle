/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"

#include "particleman.h"

// RENDERERS START
#include "bsprenderer.h"
#include "propmanager.h"
#include "watershader.h"

#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

extern CGameStudioModelRenderer g_StudioRenderer;
// RENDERERS END

extern IParticleMan* g_pParticleMan;

extern int giTeamplay;

extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare; // Vit_amiN


/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

// RENDERERS START
	gHUD.m_pFogSettings.end = 0.0;
	gHUD.m_pFogSettings.start = 0.0;
	gHUD.m_pFogSettings.active = false;
	gHUD.m_pSkyFogSettings.end = 0.0;
	gHUD.m_pSkyFogSettings.start = 0.0;
	gHUD.m_pSkyFogSettings.active = false;
	// RENDERERS END

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return true;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

// RENDERERS START
	gHUD.m_pFogSettings.end = 0.0;
	gHUD.m_pFogSettings.start = 0.0;
	gHUD.m_pFogSettings.active = false;
	gHUD.m_pSkyFogSettings.end = 0.0;
	gHUD.m_pSkyFogSettings.start = 0.0;
	gHUD.m_pSkyFogSettings.active = false;
	// RENDERERS END

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}


	//TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL; // Vit_amiN: clear egon's beam flare
}


bool CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	//Note: this user message could be updated to include multiple gamemodes
	//See CHalfLifeTeamplay::UpdateGameMode
	//TODO: define game mode constants
	m_Teamplay = giTeamplay = READ_BYTE();

	if (gViewPort && !gViewPort->m_pScoreBoard)
	{
		gViewPort->CreateScoreBoard();
		gViewPort->m_pScoreBoard->Initialize();

		if (!gHUD.m_iIntermission)
		{
			gViewPort->HideScoreBoard();
		}
	}

	return true;
}


bool CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int armor, blood;
	Vector from;
	int i;
	float count;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return true;
}

bool CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (0 != m_iConcussionEffect)
	{
		this->m_StatusIcons.EnableIcon("dmg_concuss", giR, giG, giB);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return true;
}

bool CHud::MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const std::uint64_t lowerBits = READ_LONG();
	const std::uint64_t upperBits = READ_LONG();

	m_iWeaponBits = (lowerBits & 0XFFFFFFFF) | ((upperBits & 0XFFFFFFFF) << 32ULL);

	return true;
}

bool CHud::MsgFunc_SendAnim(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	auto p = gEngfuncs.GetViewModel();
	int iAnim = READ_SHORT();
	int iBody = READ_SHORT();
	int iBlend = READ_BYTE();

	p->latched.prevsequence = p->curstate.sequence;
	gEngfuncs.pfnWeaponAnim(iAnim, iBody);

	// doesnt do anything rn
	if (iBlend == 0)
		p->latched.prevsequence = iAnim;
	return true;
}

bool CHud::MsgFunc_DOF(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	flMaxDofBlurAlpha = READ_BYTE() / 100.0f;
	if (flMaxDofBlurAlpha > 1.0f)
		flMaxDofBlurAlpha = 1.0f;

	return true;
}


// RENDERERS START
int CHud ::MsgFunc_SetFog(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	gHUD.m_pFogSettings.color.x = (float)READ_SHORT() / 255;
	gHUD.m_pFogSettings.color.y = (float)READ_SHORT() / 255;
	gHUD.m_pFogSettings.color.z = (float)READ_SHORT() / 255;
	gHUD.m_pFogSettings.start = READ_SHORT();
	gHUD.m_pFogSettings.end = READ_SHORT();
	gHUD.m_pFogSettings.affectsky = (READ_SHORT() == 1) ? false : true;

	if (gHUD.m_pFogSettings.end < 1 && gHUD.m_pFogSettings.start < 1)
		gHUD.m_pFogSettings.active = false;
	else
		gHUD.m_pFogSettings.active = true;

	return 1;
}
int CHud ::MsgFunc_LightStyle(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int m_iStyleNum = READ_BYTE();
	char* szStyle = READ_STRING();
	gBSPRenderer.AddLightStyle(m_iStyleNum, szStyle);

	return 1;
}
int CHud ::MsgFunc_StudioDecal(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Vector pos, normal;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();
	normal.x = READ_COORD();
	normal.y = READ_COORD();
	normal.z = READ_COORD();
	int entindex = READ_SHORT();

	if (!entindex)
		return 1;

	cl_entity_t* pEntity = gEngfuncs.GetEntityByIndex(entindex);

	if (!pEntity)
		return 1;

	if (pEntity == gEngfuncs.GetLocalPlayer())
	{
		pEntity = gEngfuncs.GetViewModel();
	}
	g_StudioRenderer.StudioDecalForEntity(pos, normal, READ_STRING(), pEntity);

	return 1;
}
int CHud ::MsgFunc_FreeEnt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iEntIndex = READ_SHORT();

	if (!iEntIndex)
		return 1;


	cl_entity_t* pEntity = gEngfuncs.GetEntityByIndex(iEntIndex);

	if (!pEntity)
		return 1;

	pEntity->efrag = NULL;
	return 1;
}
// RENDERERS END