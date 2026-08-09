/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "animation.h"
#include "effects.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"

#define SF_XEN_PLANT_DROP_TO_FLOOR 2
#define SF_XEN_PLANT_NONSOLID 8
#define SF_XEN_PLANT_TRANSIT 0x2000

#define SF_XEN_PLANT_LIGHT_IGNORE_PLAYER 64

#define XEN_PLANT_GLOW_SPRITE		"sprites/flare3.spr"
#define XEN_PLANT_HIDE_TIME			5

class CActAnimating : public CBaseAnimating
{
public:
	void SetActivity( Activity act );
	inline Activity	GetActivity() { return m_Activity; }

	int ObjectCaps() override {
		int caps = CBaseAnimating::ObjectCaps();
		if (FBitSet(pev->spawnflags, SF_XEN_PLANT_TRANSIT))
			return caps;
		return caps & ~FCAP_ACROSS_TRANSITION;
	}

	void HandleAnimEvent(MonsterEvent_t *pEvent) override
	{
		HandleBaseAnimEvent(pEvent);
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

protected:
	void DropToFloor();
private:
	Activity m_Activity;
};

TYPEDESCRIPTION	CActAnimating::m_SaveData[] =
{
	DEFINE_FIELD( CActAnimating, m_Activity, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CActAnimating, CBaseAnimating )

void CActAnimating::SetActivity( Activity act ) 
{
	int sequence = LookupActivity( act ); 
	if( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		pev->sequence = sequence;
		m_Activity = act; 
		pev->frame = 0;
		ResetSequenceInfo();
	}
}

void CActAnimating::DropToFloor()
{
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
	}
}

class CXenPLight : public CActAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch( CBaseEntity *pOther ) override;
	void Think() override;
	void UpdateOnRemove() override;

	void LightOn();
	void LightOff();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	static const NamedVisual glowVisual;
private:
	CSprite *m_pGlow;
};

LINK_ENTITY_TO_CLASS( xen_plantlight, CXenPLight )

TYPEDESCRIPTION	CXenPLight::m_SaveData[] =
{
	DEFINE_FIELD( CXenPLight, m_pGlow, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CXenPLight, CActAnimating )

const NamedVisual CXenPLight::glowVisual = BuildVisual("XenLight.Glow")
		.Model(XEN_PLANT_GLOW_SPRITE)
		.RenderMode(kRenderGlow);

void CXenPLight::Spawn()
{
	Precache();

	SetMyModel("models/light.mdl");
	pev->movetype = MOVETYPE_NONE;

	if (FBitSet(pev->spawnflags, SF_XEN_PLANT_DROP_TO_FLOOR))
	{
		DropToFloor();
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize( pev, Vector( -80, -80, 0 ), Vector( 80, 80, 32 ) );
	SetActivity( ACT_IDLE );
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->frame = RANDOM_FLOAT( 0, 255 );

	if (FBitSet(pev->flags, FL_KILLME))
		return;

	const Visual* pGlow = GetVisual(glowVisual);
	if (pGlow)
	{
		Visual glow = *pGlow;
		if (!glow.HasDefined(Visual::COLOR_DEFINED))
		{
			glow.SetColor(Color3((int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z));
		}
		if (!glow.HasDefined(Visual::ALPHA_DEFINED))
		{
			glow.SetAlpha((int)pev->renderamt);
		}
		if (!glow.HasDefined(Visual::RENDERFX_DEFINED))
		{
			glow.SetRenderFx(pev->renderfx);
		}
		m_pGlow = CreateSpriteFromVisual(&glow, pev->origin + Vector( 0, 0, ( pev->mins.z + pev->maxs.z ) * 0.5f ));
		if (m_pGlow)
		{
			m_pGlow->SetAttachment( edict(), 1 );
			if (FBitSet(pev->spawnflags, SF_XEN_PLANT_TRANSIT))
				m_pGlow->pev->spawnflags |= SF_SPRITE_TRANSIT;
		}
	}
}

void CXenPLight::Precache()
{
	PrecacheMyModel("models/light.mdl");
	RegisterVisual(glowVisual);
}

void CXenPLight::Think()
{
	float flInterval = StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents(flInterval);

	switch( GetActivity() )
	{
	case ACT_CROUCH:
		if( m_fSequenceFinished )
		{
			SetActivity( ACT_CROUCHIDLE );
			LightOff();
		}
		break;
	case ACT_CROUCHIDLE:
		if( gpGlobals->time > pev->dmgtime )
		{
			SetActivity( ACT_STAND );
			LightOn();
		}
		break;
	case ACT_STAND:
		if( m_fSequenceFinished )
			SetActivity( ACT_IDLE );
		break;
	case ACT_IDLE:
	default:
		break;
	}
}

void CXenPLight::Touch( CBaseEntity *pOther )
{
	if( !FBitSet(pev->spawnflags, SF_XEN_PLANT_LIGHT_IGNORE_PLAYER) && pOther->IsPlayer() )
	{
		pev->dmgtime = gpGlobals->time + XEN_PLANT_HIDE_TIME;
		if( GetActivity() == ACT_IDLE || GetActivity() == ACT_STAND )
		{
			SetActivity( ACT_CROUCH );
		}
	}
}

void CXenPLight::UpdateOnRemove()
{
	UTIL_RemoveAndClean(m_pGlow);
	CActAnimating::UpdateOnRemove();
}

void CXenPLight::LightOn()
{
	SUB_UseTargets( this, USE_ON );
	if( m_pGlow )
		m_pGlow->pev->effects &= ~EF_NODRAW;
}

void CXenPLight::LightOff()
{
	SUB_UseTargets( this, USE_OFF );
	if( m_pGlow )
		m_pGlow->pev->effects |= EF_NODRAW;
}

class CXenHair : public CActAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void Think() override;
};

LINK_ENTITY_TO_CLASS( xen_hair, CXenHair )

#define SF_HAIR_SYNC		0x0001

void CXenHair::Spawn()
{
	Precache();
	SetMyModel("models/hair.mdl");
	UTIL_SetSize( pev, Vector( -4, -4, 0 ), Vector( 4, 4, 32 ) );
	pev->sequence = 0;

	if( !( pev->spawnflags & SF_HAIR_SYNC ) )
	{
		pev->frame = RANDOM_FLOAT( 0, 255 );
		pev->framerate = RANDOM_FLOAT( 0.7f, 1.4f );
	}
	ResetSequenceInfo();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.4f );	// Load balance these a bit

	if (FBitSet(pev->spawnflags, SF_XEN_PLANT_DROP_TO_FLOOR))
	{
		DropToFloor();
	}
}

void CXenHair::Think()
{
	float flInterval = StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents(flInterval);
}

void CXenHair::Precache()
{
	PrecacheMyModel( "models/hair.mdl" );
}

class CXenTreeTrigger : public CBaseEntity
{
public:
	void Touch( CBaseEntity *pOther ) override;
	static CXenTreeTrigger *TriggerCreate( edict_t *pOwner, const Vector &position );
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( xen_ttrigger, CXenTreeTrigger )

CXenTreeTrigger *CXenTreeTrigger::TriggerCreate( edict_t *pOwner, const Vector &position )
{
	CXenTreeTrigger *pTrigger = GetClassPtr( (CXenTreeTrigger *)NULL );
	pTrigger->pev->origin = position;
	//pTrigger->pev->classname = MAKE_STRING( "xen_ttrigger" ); // re-create on reload
	pTrigger->pev->solid = SOLID_TRIGGER;
	pTrigger->pev->movetype = MOVETYPE_NONE;
	pTrigger->pev->owner = pOwner;

	return pTrigger;
}

void CXenTreeTrigger::Touch( CBaseEntity *pOther )
{
	if( pev->owner )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( pev->owner );
		if (pEntity)
			pEntity->Touch( pOther );
	}
}

#define TREE_AE_ATTACK		1

class CXenTree : public CActAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void Touch( CBaseEntity *pOther ) override;
	void Think() override;
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override { Attack(); return TakeDamageResult(); }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void Attack();
	int DefaultClassify() override { return CLASS_BARNACLE; }

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	static constexpr const char* attackHitSoundScript = "XenTree.AttackHit";
	static constexpr const char* attackMissSoundScript = "XenTree.AttackMiss";

private:
	CXenTreeTrigger	*m_pTrigger;
};

LINK_ENTITY_TO_CLASS( xen_tree, CXenTree )

TYPEDESCRIPTION	CXenTree::m_SaveData[] =
{
	DEFINE_FIELD( CXenTree, m_pTrigger, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CXenTree, CActAnimating )

void CXenTree::Spawn()
{
	Precache();

	SetMyModel("models/tree.mdl");
	pev->movetype = MOVETYPE_NONE;
	pev->solid = FBitSet(pev->spawnflags, SF_XEN_PLANT_NONSOLID) ? SOLID_NOT : SOLID_BBOX;

	pev->takedamage = DAMAGE_YES;

	UTIL_SetSize( pev, Vector( -30, -30, 0 ), Vector( 30, 30, 188 ) );
	SetActivity( ACT_IDLE );
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->frame = RANDOM_FLOAT( 0, 255 );
	pev->framerate = RANDOM_FLOAT( 0.7f, 1.4f );

	if (FBitSet(pev->spawnflags, SF_XEN_PLANT_DROP_TO_FLOOR))
	{
		DropToFloor();
	}

	if (FBitSet(pev->flags, FL_KILLME))
		return;
}

void CXenTree::Precache()
{
	PrecacheMyModel( "models/tree.mdl" );
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
}

void CXenTree::Activate()
{
	CActAnimating::Activate();

	Vector triggerPosition;
	UTIL_MakeVectorsPrivate( pev->angles, triggerPosition, NULL, NULL );
	triggerPosition = pev->origin + ( triggerPosition * 64 );
	// Create the trigger
	m_pTrigger = CXenTreeTrigger::TriggerCreate( edict(), triggerPosition );
	UTIL_SetSize( m_pTrigger->pev, Vector( -24, -24, 0 ), Vector( 24, 24, 128 ) );
}

void CXenTree::Touch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() && FClassnameIs( pOther->pev, "monster_bigmomma" ) )
		return;

	Attack();
}

void CXenTree::Attack()
{
	if( GetActivity() == ACT_IDLE )
	{
		SetActivity( ACT_MELEE_ATTACK1 );
		pev->framerate = RANDOM_FLOAT( 1.0f, 1.4f );
		EmitSoundScript(attackMissSoundScript);
	}
}

void CXenTree::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case TREE_AE_ATTACK:
		{
			CBaseEntity *pList[8];
			bool sound = false;
			int count = UTIL_EntitiesInBox( pList, 8, m_pTrigger->pev->absmin, m_pTrigger->pev->absmax, FL_MONSTER | FL_CLIENT );
			Vector forward;

			UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

			for( int i = 0; i < count; i++ )
			{
				if( pList[i] != this )
				{
					if( pList[i]->pev->owner != edict() )
					{
						sound = true;
						pList[i]->TakeDamage( pev, pev, DamageInfo(25, DMG_CRUSH | DMG_SLASH) );
						pList[i]->pev->punchangle.x = 15;
						pList[i]->pev->velocity = pList[i]->pev->velocity + forward * 100;
					}
				}
			}

			if( sound )
			{
				EmitSoundScript(attackHitSoundScript);
			}
		}
		return;
	}

	CActAnimating::HandleAnimEvent( pEvent );
}

void CXenTree::Think()
{
	float flInterval = StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents(flInterval);

	switch( GetActivity() )
	{
	case ACT_MELEE_ATTACK1:
		if( m_fSequenceFinished )
		{
			SetActivity( ACT_IDLE );
			pev->framerate = RANDOM_FLOAT( 0.6f, 1.4f );
		}
		break;
	default:
	case ACT_IDLE:
		break;
	}
}

// UNDONE:	These need to smoke somehow when they take damage
//			Touch behavior?
//			Cause damage in smoke area

//
// Spores
//
class CXenSpore : public CActAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch( CBaseEntity *pOther ) override;
	void Think() override;
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override { Attack(); return TakeDamageResult(); }
	void Attack() {}

	virtual const char* DefaultModel() const = 0;
	void SetMySize(const Vector& vecMin, const Vector& vecMax);
};

class CXenSporeSmall : public CXenSpore
{
	void Spawn() override;
	const char* DefaultModel() const override {
		return "models/fungus(small).mdl";
	}
};

class CXenSporeMed : public CXenSpore
{
	void Spawn() override;
	const char* DefaultModel() const override {
		return "models/fungus.mdl";
	}
};

class CXenSporeLarge : public CXenSpore
{
	void Spawn() override;
	const char* DefaultModel() const override {
		return "models/fungus(large).mdl";
	}

	static const Vector m_hullSizes[];
};

// Fake collision box for big spores
class CXenHull : public CBaseEntity
{
public:
	static CXenHull	*CreateHull( CBaseEntity *source, const Vector &mins, const Vector &maxs, const Vector &offset );
	int Classify() override { return CLASS_BARNACLE; }
	int ObjectCaps() override
	{
		int caps = CBaseEntity::ObjectCaps();
		if (FBitSet(pev->spawnflags, SF_XEN_PLANT_TRANSIT))
			return caps;
		return caps & ~FCAP_ACROSS_TRANSITION;
	}
};

CXenHull *CXenHull::CreateHull( CBaseEntity *source, const Vector &mins, const Vector &maxs, const Vector &offset )
{
	CXenHull *pHull = GetClassPtr( (CXenHull *)NULL );

	UTIL_SetOrigin( pHull->pev, source->pev->origin + offset );
	SET_MODEL( pHull->edict(), STRING( source->pev->model ) );
	pHull->pev->solid = SOLID_BBOX;
	pHull->pev->classname = MAKE_STRING( "xen_hull" );
	pHull->pev->movetype = MOVETYPE_NONE;
	pHull->pev->owner = source->edict();
	UTIL_SetSize( pHull->pev, mins, maxs );
	pHull->pev->renderamt = 0;
	pHull->pev->rendermode = kRenderTransTexture;
	//	pHull->pev->effects = EF_NODRAW;

	return pHull;
}

LINK_ENTITY_TO_CLASS( xen_spore_small, CXenSporeSmall )
LINK_ENTITY_TO_CLASS( xen_spore_medium, CXenSporeMed )
LINK_ENTITY_TO_CLASS( xen_spore_large, CXenSporeLarge )
LINK_ENTITY_TO_CLASS( xen_hull, CXenHull )

void CXenSporeSmall::Spawn()
{
	CXenSpore::Spawn();
	SetMySize( Vector( -16, -16, 0 ), Vector( 16, 16, 64) );
}

void CXenSporeMed::Spawn()
{
	CXenSpore::Spawn();
	SetMySize( Vector( -40, -40, 0 ), Vector( 40, 40, 120 ) );
}

// I just eyeballed these -- fill in hulls for the legs
const Vector CXenSporeLarge::m_hullSizes[] =
{
	Vector( 90, -25, 0 ),
	Vector( 25, 75, 0 ),
	Vector( -15, -100, 0 ),
	Vector( -90, -35, 0 ),
	Vector( -90, 60, 0 ),
};

void CXenSporeLarge::Spawn()
{
	CXenSpore::Spawn();
	SetMySize( Vector( -48, -48, 110 ), Vector( 48, 48, 240 ) );

	if (FBitSet(pev->flags, FL_KILLME))
		return;

	Vector forward, right;

	UTIL_MakeVectorsPrivate( pev->angles, forward, right, NULL );

	if (FBitSet(pev->spawnflags, SF_XEN_PLANT_NONSOLID))
		return;

	// Rotate the leg hulls into position
	for( int i = 0; i < (int)ARRAYSIZE( m_hullSizes ); i++ )
	{
		CXenHull* hull = CXenHull::CreateHull( this, Vector( -12, -12, 0 ), Vector( 12, 12, 120 ), ( m_hullSizes[i].x * forward ) + ( m_hullSizes[i].y * right ) );
		if (hull && FBitSet(pev->spawnflags, SF_XEN_PLANT_TRANSIT))
			hull->pev->spawnflags |= SF_XEN_PLANT_TRANSIT;
	}
}

void CXenSpore :: Spawn()
{
	Precache();

	SetMyModel(DefaultModel());
	pev->movetype = MOVETYPE_NONE;
	pev->solid = FBitSet(pev->spawnflags, SF_XEN_PLANT_NONSOLID) ? SOLID_NOT : SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;

	//SetActivity( ACT_IDLE );
	pev->sequence = 0;
	pev->frame = RANDOM_FLOAT( 0, 255 );
	pev->framerate = RANDOM_FLOAT( 0.7f, 1.4f );
	ResetSequenceInfo();
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.4f );	// Load balance these a bit

	if (FBitSet(pev->spawnflags, SF_XEN_PLANT_DROP_TO_FLOOR))
	{
		DropToFloor();
	}
}

void CXenSpore::Precache()
{
	PrecacheMyModel(DefaultModel());
}

void CXenSpore::Touch( CBaseEntity *pOther )
{
}

void CXenSpore::Think()
{
	float flInterval = StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents(flInterval);
}

void CXenSpore::SetMySize(const Vector &vecMin, const Vector &vecMax)
{
	Vector vecMins = vecMin;
	Vector vecMaxs = vecMax;
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate && entTemplate->IsSizeDefined())
	{
		vecMins = entTemplate->MinSize();
		vecMaxs = entTemplate->MaxSize();
	}
	UTIL_SetSize(pev, vecMins, vecMaxs);
}
