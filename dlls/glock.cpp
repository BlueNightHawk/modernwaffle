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

LINK_ENTITY_TO_CLASS(weapon_glock, CGlock);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock);

void CGlock::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	m_bFirstDraw = true;

	FallInit(); // get ready to fall down.
}


void CGlock::Precache()
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pl_gun1.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav"); //handgun

	m_usFireGlock1 = PRECACHE_EVENT(1, "events/glock1.sc");
	m_usFireGlock2 = PRECACHE_EVENT(1, "events/glock2.sc");
}

bool CGlock::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return true;
}

void CGlock::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "9mm", _9MM_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

bool CGlock::Deploy()
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
	return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", iAnim, "onehanded");
}

void CGlock::SecondaryAttack()
{
	m_bAiming = !m_bAiming;

	if (m_bAiming)
	{
		m_pPlayer->SetDOF(0.0f, 0.0f);
		SendWeaponAnim((m_iClip != 0) ? GLOCK_AIM_IN : GLOCK_AIM_IN_EMPTY);
		m_flAnimTime = gpGlobals->time + 0.35f;
		m_pPlayer->m_iFOV = 80;
	}
	else
	{
		m_pPlayer->SetDOF(0.0f, 0.0f);
		SendWeaponAnim((m_iClip != 0) ? GLOCK_AIM_OUT : GLOCK_AIM_OUT_EMPTY);
		m_flAnimTime = gpGlobals->time + 0.6f;
		m_pPlayer->m_iFOV = 0;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.35f);

}

void CGlock::PrimaryAttack()
{
	GlockFire(0.01, 0.3, true);
}

void CGlock::GlockFire(float flSpread, float flCycleTime, bool fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		//if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined(CLIENT_WEAPONS)
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, m_bAiming ? 1 : 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	m_bSprintingAnim = m_bWalkingAnim = false;
	m_flAnimTime = gpGlobals->time + 0.4f;
	m_pPlayer->SetDOF(0.0f, 0.0f);
}


void CGlock::Reload()
{
	bool iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(17, GLOCK_RELOAD, 3.8);
	else
		iResult = DefaultReload(17, GLOCK_RELOAD_NOT_EMPTY, 3.0);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

		m_bSprintingAnim = m_bWalkingAnim = false;
		m_flAnimTime = gpGlobals->time + 1.5f;
		m_pPlayer->SetDOF(0.45f, 2.9f);

		m_pPlayer->m_iFOV = 0;
		m_bAiming = false;
	}
	else
	{
		if ((m_pPlayer->m_afButtonPressed & IN_RELOAD) == 0 || m_flInspectDelay > gpGlobals->time )
		{
			return;
		}

		m_pPlayer->SetDOF(1.0f, 3.9f);

		SendWeaponAnim((m_iClip != 0) ? GLOCK_AMMO_CHECK : GLOCK_AMMO_CHECK_EMPTY);

		m_bSprintingAnim = m_bWalkingAnim = false;
		m_flAnimTime = gpGlobals->time + 3.9f;
		m_flInspectDelay = gpGlobals->time + 3.9f;

		m_pPlayer->m_iFOV = 0;
		m_bAiming = false;
	}
}


void CGlock::DynamicAnims()
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


void CGlock::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	DynamicAnims();

	m_bFirstDraw = false;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;
}








class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CGlockAmmo);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);
