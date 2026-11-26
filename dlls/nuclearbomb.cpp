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

//=========================================================
// Nuclear bomb
//=========================================================

class CNuclearBombTimer : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;
	void EXPORT NuclearBombTimerThink();
	void SetNuclearBombTimer(bool on);
	int ObjectCaps() override {return FCAP_DONT_SAVE;}

	bool bPlayBombSound;
	bool bBombSoundPlaying;

	static const NamedSoundScript tickingSoundScript;

	static const NamedVisual modelVisual;
};

LINK_ENTITY_TO_CLASS(item_nuclearbombtimer, CNuclearBombTimer)

const NamedSoundScript CNuclearBombTimer::tickingSoundScript = {
	CHAN_BODY,
	{"common/nuke_ticking.wav"},
	0.75f,
	ATTN_IDLE,
	"NuclearBomb.Ticking"
};

const NamedVisual CNuclearBombTimer::modelVisual = BuildVisual("NuclearBomb.Timer")
	.Model("models/nuke_timer.mdl");

void CNuclearBombTimer::Precache()
{
	PrecacheEntityResourcesWithOwnVisual(modelVisual);
	RegisterAndPrecacheSoundScript(tickingSoundScript);
}

void CNuclearBombTimer::Spawn()
{
	Precache();
	ApplyVisualWithOwn(GetVisual(modelVisual));
	pev->solid = SOLID_NOT;
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,32));
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Nuclear Bomb timer fell out of level at %f,%f,%f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
	pev->skin = 0;
	bPlayBombSound = false;
	bBombSoundPlaying = false;
}

void CNuclearBombTimer::NuclearBombTimerThink()
{
	if (pev->skin <= 1)
		pev->skin++;
	else
		pev->skin = 0;
	if (bPlayBombSound)
	{
		EmitSoundScript(tickingSoundScript);
		bBombSoundPlaying = true;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CNuclearBombTimer::SetNuclearBombTimer(bool on)
{
	if (on)
	{
		SetThink(&CNuclearBombTimer::NuclearBombTimerThink);
		pev->nextthink = gpGlobals->time;
		bPlayBombSound = true;
	}
	else
	{
		SetThink(NULL);
		pev->nextthink = gpGlobals->time;
		pev->skin = 3;
		if (bBombSoundPlaying)
		{
			StopSoundScript(tickingSoundScript);
			bBombSoundPlaying = false;
		}
	}
}

class CNuclearBombButton : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;
	void SetNuclearBombButton(bool on);
	int ObjectCaps() override {return FCAP_DONT_SAVE;}

	static const NamedVisual modelVisual;
};

LINK_ENTITY_TO_CLASS(item_nuclearbombbutton, CNuclearBombButton)

const NamedVisual CNuclearBombButton::modelVisual = BuildVisual("NuclearBomb.Button")
	.Model("models/nuke_button.mdl");

void CNuclearBombButton::Precache()
{
	PrecacheEntityResourcesWithOwnVisual(modelVisual);
}

void CNuclearBombButton::Spawn()
{
	Precache();
	ApplyVisualWithOwn(GetVisual(modelVisual));
	pev->solid = SOLID_NOT;
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,32));
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Nuclear Bomb button fell out of level at %f,%f,%f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
	pev->skin = 0;
}

void CNuclearBombButton::SetNuclearBombButton(bool on)
{
	pev->skin = on ? 1 : 0;
}

class CNuclearBomb : public CBaseToggle
{
public:
	void Precache() override;
	void Spawn() override;
	const char* DefaultModel() override { return "models/nuke_case.mdl"; }
	void Activate() override;
	void KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override {return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;}
	void UpdateOnRemove() override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_fOn;
	float m_flLastPush;
	int m_iPushCount;
	CNuclearBombTimer* m_pTimer;
	CNuclearBombButton* m_pButton;

	static const NamedSoundScript offSoundScript;
	static const NamedSoundScript onSoundScript;
};

LINK_ENTITY_TO_CLASS(item_nuclearbomb, CNuclearBomb)

TYPEDESCRIPTION CNuclearBomb::m_SaveData[] =
{
	DEFINE_FIELD(CNuclearBomb, m_fOn, FIELD_BOOLEAN),
	DEFINE_FIELD(CNuclearBomb, m_flLastPush, FIELD_TIME),
	DEFINE_FIELD(CNuclearBomb, m_iPushCount, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CNuclearBomb, CBaseToggle)

const NamedSoundScript CNuclearBomb::offSoundScript = {
	CHAN_VOICE,
	{"buttons/button4.wav"},
	"NuclearBomb.Off"
};

const NamedSoundScript CNuclearBomb::onSoundScript = {
	CHAN_VOICE,
	{"buttons/button6.wav"},
	"NuclearBomb.On"
};

void CNuclearBomb::Precache()
{
	PrecacheEntityResources();

	EntityOverrides entityOverrides = GetProjectileOverrides();
	UTIL_PrecacheOther("item_nuclearbombtimer", entityOverrides);
	UTIL_PrecacheOther("item_nuclearbombbutton", entityOverrides);

	RegisterAndPrecacheSoundScript(offSoundScript);
	RegisterAndPrecacheSoundScript(onSoundScript);
}

void CNuclearBomb::Activate()
{
	CBaseToggle::Activate();

	m_pTimer = (CNuclearBombTimer*)Create("item_nuclearbombtimer", pev->origin, pev->angles, nullptr, GetProjectileOverrides());
	if (m_pTimer)
	{
		m_pTimer->pev->effects = pev->effects;
		m_pTimer->SetNuclearBombTimer(m_fOn);
	}
	m_pButton = (CNuclearBombButton*)Create("item_nuclearbombbutton", pev->origin, pev->angles, nullptr, GetProjectileOverrides());
	if (m_pButton)
	{
		m_pButton->pev->effects = pev->effects;
		m_pButton->SetNuclearBombButton(m_fOn);
	}
}

void CNuclearBomb::Spawn()
{
	Precache();
	SetMyModel();
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,32));
	pev->movetype = MOVETYPE_NONE;
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Nuclear Bomb fell out of level at %f,%f,%f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
	m_iPushCount = 0;
	m_flLastPush = gpGlobals->time;
}

void CNuclearBomb::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "initialstate" ) )
	{
		m_fOn = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

void CNuclearBomb::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if ((m_flWait >= 0 || m_iPushCount <= 0) && m_flWait <= gpGlobals->time - m_flLastPush && ShouldToggle(useType, m_fOn))
	{
		if (m_fOn)
		{
			m_fOn = false;
			EmitSoundScript(offSoundScript);
		}
		else
		{
			m_fOn = true;
			EmitSoundScript(onSoundScript);
		}
		SUB_UseTargets(pActivator);
		if (m_pButton)
		{
			m_pButton->SetNuclearBombButton(m_fOn);
		}
		if (m_pTimer)
		{
			m_pTimer->SetNuclearBombTimer(m_fOn);
		}
		m_iPushCount++;
		m_flLastPush = gpGlobals->time;
	}
}

void CNuclearBomb::UpdateOnRemove()
{
	UTIL_RemoveAndClean(m_pTimer);
	UTIL_RemoveAndClean(m_pButton);
	CBaseToggle::UpdateOnRemove();
}
