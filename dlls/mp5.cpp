/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_mp5, CMP5);
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);


//=========================================================
//=========================================================
void CMP5::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	m_flNextGrenadeLoad = gpGlobals->time;

	m_bFirstDraw = true;

	FallInit(); // get ready to fall down.
}


void CMP5::Precache()
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl"); // grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks3.wav"); // H to the K

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usMP5 = PRECACHE_EVENT(1, "events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "events/mp52.sc");
}

bool CMP5::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return true;
}

void CMP5::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "9mm", _9MM_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}

	if (m_flNextGrenadeLoad < gpGlobals->time)
	{
		pPlayer->GiveAmmo(1, "ARgrenades", M203_GRENADE_MAX_CARRY);
		m_flNextGrenadeLoad = gpGlobals->time + 10;
	}
}

bool CMP5::Deploy()
{
	int iAnim = 0;

	if (m_bFirstDraw)
	{
		m_pPlayer->SetDOF(1.0f, 1.0f);
		iAnim = GLOCK_DRAW_FIRSTDRAW;
		m_flAnimTime = gpGlobals->time + 1.43f;
	}
	else
	{
		m_pPlayer->SetDOF(0.0f, 0.0f);
		m_flAnimTime = gpGlobals->time + 0.4f;
		iAnim = (m_iClip == 0) ? GLOCK_DRAW_EMPTY : GLOCK_DRAW;
	}
	m_bSprintingAnim = m_bWalkingAnim = false;
	m_bAiming = false;

	// pev->body = 1;
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", iAnim, "mp5");
}


void CMP5::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->pev->v_angle[0] -= RANDOM_FLOAT(0.25f, 0.75f);
	m_pPlayer->pev->v_angle[1] += RANDOM_FLOAT(-0.25f, 0.25f);
	m_pPlayer->pev->fixangle = 1;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usMP5, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, m_bAiming ? 1 : 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	m_bSprintingAnim = m_bWalkingAnim = false;
	m_flAnimTime = gpGlobals->time + 0.4f;

//	if (m_bAiming)
//		m_pPlayer->SetDOF(1.0f, -.1f);
//	else
		m_pPlayer->SetDOF(0.0f, 0.0f);
}



void CMP5::SecondaryAttack()
{
	m_bAiming = !m_bAiming;

	if (m_bAiming)
	{
		//m_pPlayer->SetDOF(1.0f, -.1f);
		m_pPlayer->SetDOF(0.0f, 0.0f);
		SendWeaponAnim((m_iClip != 0) ? GLOCK_AIM_IN : GLOCK_AIM_IN_EMPTY);
		m_flAnimTime = gpGlobals->time + 0.35f;
		m_pPlayer->m_iFOV = 40;
	}
	else
	{
		m_pPlayer->SetDOF(0.0f, 0.0f);
		SendWeaponAnim((m_iClip != 0) ? GLOCK_AIM_OUT : GLOCK_AIM_OUT_EMPTY);
		m_flAnimTime = gpGlobals->time + 0.2f;
		m_pPlayer->m_iFOV = 0;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.35f);
}

void CMP5::Reload()
{
	bool iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(iMaxClip(), GLOCK_RELOAD, 4.32);
	else
		iResult = DefaultReload(iMaxClip(), GLOCK_RELOAD_NOT_EMPTY, 2.92);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

		m_bSprintingAnim = m_bWalkingAnim = false;
		m_flAnimTime = gpGlobals->time + 1.5f;
		if (m_iClip == 0)
			m_pPlayer->SetDOF(0.45f, 4.32);
		else
			m_pPlayer->SetDOF(0.45f, 2.9f);

		m_pPlayer->m_iFOV = 0;
		m_bAiming = false;
	}
	else
	{
		if ((m_pPlayer->m_afButtonPressed & IN_RELOAD) == 0 || m_flInspectDelay > gpGlobals->time)
		{
			return;
		}

		m_pPlayer->SetDOF(1.0f, 2.9f);

		SendWeaponAnim((m_iClip != 0) ? GLOCK_AMMO_CHECK : GLOCK_AMMO_CHECK_EMPTY);

		m_bSprintingAnim = m_bWalkingAnim = false;
		m_flAnimTime = gpGlobals->time + 2.9f;
		m_flInspectDelay = gpGlobals->time + 2.9f;

		m_pPlayer->m_iFOV = 0;
		m_bAiming = false;
	}
}


void CMP5::DynamicAnims()
{
	if (m_bAiming)
	{
		m_flAnimTime = 0.0f;
		m_bWalkingAnim = false;
		m_bSprintingAnim = false;
		if (m_flAnimTime != 0.0f && m_flAnimTime < gpGlobals->time)
		{
			SendWeaponAnim((m_iClip != 0) ? GLOCK_AIM_IDLE : GLOCK_AIM_IDLE_EMPTY);
			m_flAnimTime = gpGlobals->time + 0.1f;
		}
		return;
	}

	if ((m_pPlayer->m_afButtonPressed & IN_JUMP) != 0)
	{
		if (!m_bJumpAnim)
		{
			SendWeaponAnim((m_iClip != 0) ? GLOCK_JUMP : GLOCK_JUMP_EMPTY);
			m_flAnimTime = gpGlobals->time + 0.2f;
			m_bJumpAnim = true;
			m_bSprintingAnim = m_bWalkingAnim = false;
			m_pPlayer->SetDOF(0.0f, 0.0f);
		}
	}
	else if ((m_pPlayer->pev->flags & FL_ONGROUND) == 0)
	{
		if (!m_bJumpAnim && m_flAnimTime < gpGlobals->time)
		{
			SendWeaponAnim((m_iClip != 0) ? GLOCK_IDLE1 : GLOCK_IDLE_EMPTY);
			m_flAnimTime = 0.0f;
			m_bJumpAnim = true;
			m_bSprintingAnim = m_bWalkingAnim = false;
			m_pPlayer->SetDOF(0.0f, 0.0f);
		}
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 150.0f)
	{
		if ((m_pPlayer->pev->button & IN_ALT1) != 0)
		{
			if (!m_bSprintingAnim && m_flAnimTime < gpGlobals->time)
			{
				SendWeaponAnim((m_iClip != 0) ? GLOCK_SPRINT : GLOCK_SPRINT_EMPTY);
				m_flAnimTime = gpGlobals->time + 0.2f;
				m_bWalkingAnim = true;
				m_bSprintingAnim = true;
				m_pPlayer->SetDOF(0.0f, 0.0f);
			}
		}
		else
		{
			if ((!m_bWalkingAnim || m_bSprintingAnim) && m_flAnimTime < gpGlobals->time)
			{
				SendWeaponAnim((m_iClip != 0) ? GLOCK_WALK : GLOCK_WALK_EMPTY);
				m_flAnimTime = gpGlobals->time + 0.4f;
				m_bWalkingAnim = true;
				m_bSprintingAnim = false;
				m_pPlayer->SetDOF(0.0f, 0.0f);
			}
		}
		m_bJumpAnim = false;
	}
	else
	{
		if (m_bWalkingAnim && m_flAnimTime < gpGlobals->time)
		{
			SendWeaponAnim((m_iClip != 0) ? GLOCK_IDLE1 : GLOCK_IDLE_EMPTY);
			m_flAnimTime = 0.0f;
			m_bWalkingAnim = false;
			m_bSprintingAnim = false;
			m_pPlayer->SetDOF(0.0f, 0.0f);
		}
		m_bJumpAnim = false;
	}
}



void CMP5::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	DynamicAnims();
	m_bFirstDraw = false;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;
}



class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_MP5CLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5clip, CMP5AmmoClip);
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CMP5AmmoClip);



class CMP5Chainammo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_CHAINBOX_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_9mmbox, CMP5Chainammo);


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5grenades, CMP5AmmoGrenade);
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CMP5AmmoGrenade);
