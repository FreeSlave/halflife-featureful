/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "combat.h"
#include "global_models.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "game.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"

#define SF_OSPREY_DONT_DEPLOY SF_MONSTER_SPECIAL_FLAG

#define SF_WAITFORTRIGGER	0x40

#define MAX_CARRY	24

#define OSPREY_GRUNT_TYPE_HL 0
#define OSPREY_GRUNT_TYPE_OPFOR 1

class COsprey : public CBaseMonster
{
public:
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int ObjectCaps() override { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* pkvd) override;
	const char* DefaultDisplayName() override { return "Osprey"; }
	int DefaultClassify() override { return CLASS_MACHINE; }
	int BloodColor() override { return DONT_BLEED; }
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;

	void UpdateGoal();
	bool HasDead();
	void EXPORT FlyThink();
	void EXPORT DeployThink();
	void Flight();
	void EXPORT HitTouch( CBaseEntity *pOther );
	void EXPORT FindAllThink();
	void EXPORT HoverThink();
	CBaseMonster *MakeGrunt( const Vector& vecSrc );
	virtual void PrepareGruntBeforeSpawn(CBaseEntity* pGrunt);
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink();
	void EXPORT CommandUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	void ShowDamage();
	void Update();

	CBaseEntity *m_pGoalEnt;
	Vector m_vel1;
	Vector m_vel2;
	Vector m_pos1;
	Vector m_pos2;
	Vector m_ang1;
	Vector m_ang2;
	float m_startTime;
	float m_dTime;

	Vector m_velocity;

	float m_flIdealtilt;
	float m_flRotortilt;

	float m_flRightHealth;
	float m_flLeftHealth;

	int m_iUnits;
	EHANDLE m_hGrunt[MAX_CARRY];
	Vector m_vecOrigin[MAX_CARRY];
	EHANDLE m_hRepel[4];

	int m_iSoundState;

	int m_iPitch;

	int m_iTailGibs;
	int m_iBodyGibs;
	int m_iEngineGibs;

	int m_iDoLeftSmokePuff;
	int m_iDoRightSmokePuff;

	short m_gruntType;
	short m_gruntNumber;

	float m_soundAttenuation;

	static const NamedSoundScript rotorSoundScript;
	static constexpr const char* crashSoundScript = "Osprey.Crash";

	static const NamedVisual sharedSmokeVisual;
	static const NamedVisual fallingSmokeVisual;
	static const NamedVisual damageSmokeVisual;
	static const NamedVisual fireBallVisual;
	static const NamedVisual blastCircleVisual;

protected:
	void SpawnImpl(const char* modelName, const float defaultHealth);
	void PrecacheImpl(const char* modelName, const char* tailGibs, const char* bodyGibs, const char* engineGibs);
	virtual const char* TrooperName();
	bool HasCustomRotorVolume() const {
		return pev->armorvalue > 0.0f && pev->armorvalue <= 1.0f;
	}
	float RotorVolume() const {
		if (HasCustomRotorVolume())
		{
			return pev->armorvalue;
		}
		return VOL_NORM;
	}
	bool HasCustomAttenuation() const {
		return m_soundAttenuation > 0.0f;
	}
	float RotorAttenuation() const {
		return HasCustomAttenuation() ? m_soundAttenuation : 0.15f;
	}
	void SetRotorSoundParams(SoundScriptParamOverride& param)
	{
		if (pev->armorvalue > 0.0f && pev->armorvalue <= 1.0f)
		{
			param.OverrideVolumeAbsolute(pev->armorvalue);
		}
		if (m_soundAttenuation > 0.0f)
		{
			param.OverrideAttenuationAbsolute(m_soundAttenuation);
		}
	}
};

LINK_ENTITY_TO_CLASS( monster_osprey, COsprey )

TYPEDESCRIPTION	COsprey::m_SaveData[] =
{
	DEFINE_FIELD( COsprey, m_pGoalEnt, FIELD_CLASSPTR ),
	DEFINE_FIELD( COsprey, m_vel1, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_vel2, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_pos1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COsprey, m_pos2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COsprey, m_ang1, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_ang2, FIELD_VECTOR ),

	DEFINE_FIELD( COsprey, m_startTime, FIELD_TIME ),
	DEFINE_FIELD( COsprey, m_dTime, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_velocity, FIELD_VECTOR ),

	DEFINE_FIELD( COsprey, m_flIdealtilt, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_flRotortilt, FIELD_FLOAT ),

	DEFINE_FIELD( COsprey, m_flRightHealth, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_flLeftHealth, FIELD_FLOAT ),

	DEFINE_FIELD( COsprey, m_iUnits, FIELD_INTEGER ),
	DEFINE_ARRAY( COsprey, m_hGrunt, FIELD_EHANDLE, MAX_CARRY ),
	DEFINE_ARRAY( COsprey, m_vecOrigin, FIELD_POSITION_VECTOR, MAX_CARRY ),
	DEFINE_ARRAY( COsprey, m_hRepel, FIELD_EHANDLE, 4 ),

	// DEFINE_FIELD( COsprey, m_iSoundState, FIELD_INTEGER ),
	// DEFINE_FIELD( COsprey, m_iSpriteTexture, FIELD_INTEGER ),
	// DEFINE_FIELD( COsprey, m_iPitch, FIELD_INTEGER ),

	DEFINE_FIELD( COsprey, m_iDoLeftSmokePuff, FIELD_INTEGER ),
	DEFINE_FIELD( COsprey, m_iDoRightSmokePuff, FIELD_INTEGER ),

	DEFINE_FIELD( COsprey, m_gruntType, FIELD_SHORT ),
	DEFINE_FIELD( COsprey, m_gruntNumber, FIELD_SHORT ),
	DEFINE_FIELD( COsprey, m_soundAttenuation, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COsprey, CBaseMonster )

const NamedSoundScript COsprey::rotorSoundScript = {
	CHAN_STATIC,
	{"apache/ap_rotor4.wav"},
	VOL_NORM,
	0.15f,
	"Osprey.Rotor"
};

const NamedVisual COsprey::sharedSmokeVisual = BuildVisual("Osprey.SmokeBase")
		.Model(g_pModelNameSmoke)
		.Alpha(255)
		.RenderMode(kRenderTransAlpha);

const NamedVisual COsprey::fallingSmokeVisual = BuildVisual("Osprey.FallingSmoke")
		.Scale(10.0f)
		.Framerate(10.0f)
		.Mixin(&COsprey::sharedSmokeVisual);

const NamedVisual COsprey::damageSmokeVisual = BuildVisual("Osprey.DamageSmoke")
		.Scale(FloatRange(0.9f, 2.9f))
		.Framerate(12)
		.Mixin(&COsprey::sharedSmokeVisual);

const NamedVisual COsprey::fireBallVisual = BuildVisual::Animated("Osprey.Fireball")
		.Model("sprites/fexplo.spr")
		.RenderMode(kRenderTransAdd)
		.Scale(25.0f)
		.Alpha(255);

const NamedVisual COsprey::blastCircleVisual = BuildVisual("Osprey.BlastCircle")
		.Model("sprites/rope.spr")
		.Life(0.4f)
		.BeamParams(32, 0)
		.RenderColor(255, 255, 192)
		.Alpha(128);

void COsprey::Spawn()
{
	SpawnImpl("models/osprey.mdl", gSkillData.ospreyHealth);
}

void COsprey::SpawnImpl(const char* modelName, const float defaultHealth)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetMyModel( modelName );
	UTIL_SetSize( pev, Vector( -400, -400, -100 ), Vector( 400, 400, 32 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER | FL_FLY;
	pev->takedamage = DAMAGE_YES;
	m_flRightHealth = 200;
	m_flLeftHealth = 200;
	SetMyHealth( defaultHealth );
	pev->max_health = pev->health;

	SetMyFieldOfView(0); // 180 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG( 0, 0xFF );

	InitBoneControllers();

	SetThink( &COsprey::FindAllThink );
	SetUse( &COsprey::CommandUse );

	if( !( pev->spawnflags & SF_WAITFORTRIGGER ) )
	{
		pev->nextthink = gpGlobals->time + 1.0f;
	}

	m_pos2 = pev->origin;
	m_ang2 = pev->angles;
	m_vel2 = pev->velocity;
}

void COsprey::Precache()
{
	PrecacheImpl("models/osprey.mdl", "models/osprey_tailgibs.mdl", "models/osprey_bodygibs.mdl", "models/osprey_enginegibs.mdl");
}

void COsprey::PrecacheImpl(const char* modelName, const char* tailGibs, const char* bodyGibs, const char* engineGibs)
{
	UTIL_PrecacheMonster( TrooperName(), m_reverseRelationship );

	PrecacheMyModel( modelName );

	RegisterAndPrecacheSoundScript(rotorSoundScript);
	RegisterAndPrecacheSoundScript(crashSoundScript, NPC::crashSoundScript);

	RegisterVisual(fallingSmokeVisual);
	RegisterVisual(damageSmokeVisual);
	RegisterVisual(fireBallVisual);
	RegisterVisual(blastCircleVisual);
	RegisterVisual(NPC::ropeVisual);

	m_iTailGibs = PRECACHE_MODEL( tailGibs );
	m_iBodyGibs = PRECACHE_MODEL( bodyGibs );
	m_iEngineGibs = PRECACHE_MODEL( engineGibs );
}

void COsprey::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "rotorvolume" ) )
	{
		pev->armorvalue = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "grunttype" ) )
	{
		m_gruntType = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "num" ) )
	{
		m_gruntNumber = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "attenuation" ) )
	{
		m_soundAttenuation = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void COsprey::CommandUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->nextthink = gpGlobals->time + 0.1f;
}

void COsprey::FindAllThink()
{
	CBaseEntity *pEntity = NULL;

	if (!FBitSet(pev->spawnflags, SF_OSPREY_DONT_DEPLOY))
	{
		m_iUnits = 0;
		while( m_iUnits < MAX_CARRY && ( pEntity = UTIL_FindEntityByClassname( pEntity, TrooperName() ) ) != NULL )
		{
			if( pEntity->IsAlive() && IRelationship(pEntity) < R_DL )
			{
				m_hGrunt[m_iUnits] = pEntity;
				m_vecOrigin[m_iUnits] = pEntity->pev->origin;
				m_iUnits++;
			}
		}

		if( m_iUnits == 0 )
		{
			ALERT( at_console, "osprey error: no grunts to resupply\n" );
			UTIL_Remove( this );
			return;
		}
	}

	SetThink( &COsprey::FlyThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_startTime = gpGlobals->time;
}

void COsprey::DeployThink()
{
	UTIL_MakeAimVectors( pev->angles );

	Vector vecForward = gpGlobals->v_forward;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	Vector vecSrc;

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0.0f, 0.0f, -4096.0f ), ignore_monsters, ENT( pev ), &tr );
	CSoundEnt::InsertSound( bits_SOUND_DANGER, tr.vecEndPos, 400, 0.3f );

	vecSrc = pev->origin + vecForward *  32 + vecRight *  100 + vecUp * -96;
	m_hRepel[0] = MakeGrunt( vecSrc );

	if (m_gruntNumber <= 0 || m_gruntNumber >= 2)
	{
		vecSrc = pev->origin + vecForward * -64 + vecRight *  100 + vecUp * -96;
		m_hRepel[1] = MakeGrunt( vecSrc );
	}

	if (m_gruntNumber <= 0 || m_gruntNumber >= 3)
	{
		vecSrc = pev->origin + vecForward *  32 + vecRight * -100 + vecUp * -96;
		m_hRepel[2] = MakeGrunt( vecSrc );
	}

	if (m_gruntNumber <= 0 || m_gruntNumber >= 4)
	{
		vecSrc = pev->origin + vecForward * -64 + vecRight * -100 + vecUp * -96;
		m_hRepel[3] = MakeGrunt( vecSrc );
	}

	SetThink( &COsprey::HoverThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

bool COsprey::HasDead()
{
	for( int i = 0; i < m_iUnits; i++ )
	{
		if( m_hGrunt[i] == 0 || !m_hGrunt[i]->IsAlive() )
		{
			return true;
		}
		else
		{
			m_vecOrigin[i] = m_hGrunt[i]->pev->origin;  // send them to where they died
		}
	}
	return false;
}

const char* COsprey::TrooperName()
{
	if (m_gruntType == OSPREY_GRUNT_TYPE_OPFOR)
		return "monster_human_grunt_ally";
	else
		return "monster_human_grunt";
}

CBaseMonster *COsprey::MakeGrunt( const Vector& vecSrc )
{
	CBaseEntity *pEntity;
	CBaseMonster *pGrunt;

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + Vector( 0.0f, 0.0f, -4096.0f ), dont_ignore_monsters, ENT( pev ), &tr );
	if( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP )
		return NULL;

	Vector spawnAngles = pev->angles;
	spawnAngles.x = spawnAngles.z = 0;
	for( int i = 0; i < m_iUnits; i++ )
	{
		if( m_hGrunt[i] == 0 || !m_hGrunt[i]->IsAlive() )
		{
			if( m_hGrunt[i] != 0 && m_hGrunt[i]->pev->rendermode == kRenderNormal )
			{
				m_hGrunt[i]->SUB_StartFadeOut();
			}
			pEntity = CreateNoSpawn( TrooperName(), vecSrc, spawnAngles );
			pGrunt = pEntity->MyMonsterPointer();
			// If player is my enemy and default relationship of my grunts with player is ally, reverse their relationship
			if (IDefaultRelationship(CLASS_PLAYER) >= R_DL && IDefaultRelationship(pGrunt->DefaultClassify(), CLASS_PLAYER) < R_DL)
			{
				pGrunt->m_reverseRelationship = true;
			}
			else if (IDefaultRelationship(CLASS_PLAYER) < R_DL && IDefaultRelationship(pGrunt->DefaultClassify(), CLASS_PLAYER) >= R_DL)
			{
				pGrunt->m_reverseRelationship = true;
			}
			pGrunt->m_iClass = m_iClass;
			PrepareGruntBeforeSpawn(pGrunt);
			DispatchSpawn(pEntity->edict());
			pGrunt->pev->movetype = MOVETYPE_FLY;
			pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
			pGrunt->SetActivity( ACT_GLIDE );

			CBeam *pBeam = CreateBeamFromVisual(GetVisual(NPC::ropeVisual));
			if (pBeam)
			{
				pBeam->PointEntInit( vecSrc + Vector(0, 0, 112), pGrunt->entindex() );
				pBeam->SetThink( &CBaseEntity::SUB_Remove );
				pBeam->pev->nextthink = gpGlobals->time + -4096.0f * tr.flFraction / pGrunt->pev->velocity.z + 0.5f;
			}

			// ALERT( at_console, "%d at %.0f %.0f %.0f\n", i, m_vecOrigin[i].x, m_vecOrigin[i].y, m_vecOrigin[i].z );  
			pGrunt->m_vecLastPosition = m_vecOrigin[i];
			m_hGrunt[i] = pGrunt;
			return pGrunt;
		}
	}
	// ALERT( at_console, "none dead\n");
	return NULL;
}

void COsprey::PrepareGruntBeforeSpawn(CBaseEntity *pGrunt)
{
	if (m_gruntType == OSPREY_GRUNT_TYPE_OPFOR)
	{
		CBaseMonster* pMonster = pGrunt->MyMonsterPointer();
		if (pMonster)
		{
			pMonster->SetHead(-1);
			// Set 9mmAR and hand grenades
			pMonster->pev->weapons = 3;
		}
	}
}

void COsprey::HoverThink()
{
	int i;
	for( i = 0; i < 4; i++ )
	{
		if( m_hRepel[i] != 0 && m_hRepel[i]->pev->health > 0 && !( m_hRepel[i]->pev->flags & FL_ONGROUND ) )
		{
			break;
		}
	}

	if( i == 4 )
	{
		m_startTime = gpGlobals->time;
		SetThink( &COsprey::FlyThink );
	}

	pev->nextthink = gpGlobals->time + 0.1f;
	UTIL_MakeAimVectors( pev->angles );
	Update();
}

void COsprey::UpdateGoal()
{
	if( m_pGoalEnt )
	{
		m_pos1 = m_pos2;
		m_ang1 = m_ang2;
		m_vel1 = m_vel2;
		m_pos2 = m_pGoalEnt->pev->origin;
		m_ang2 = m_pGoalEnt->pev->angles;
		UTIL_MakeAimVectors( Vector( 0, m_ang2.y, 0 ) );
		m_vel2 = gpGlobals->v_forward * m_pGoalEnt->pev->speed;

		m_startTime = m_startTime + m_dTime;
		m_dTime = 2.0f * ( m_pos1 - m_pos2 ).Length() / ( m_vel1.Length() + m_pGoalEnt->pev->speed );

		if( m_ang1.y - m_ang2.y < -180 )
		{
			m_ang1.y += 360;
		}
		else if( m_ang1.y - m_ang2.y > 180 )
		{
			m_ang1.y -= 360;
		}

		if( m_pGoalEnt->pev->speed < 400 )
			m_flIdealtilt = 0;
		else
			m_flIdealtilt = -90;
	}
	else
	{
		ALERT( at_console, "osprey missing target\n" );
	}
}

void COsprey::FlyThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( m_pGoalEnt == NULL && !FStringNull( pev->target) )// this monster has a target
	{
		m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );
		UpdateGoal();
	}

	if( gpGlobals->time > m_startTime + m_dTime )
	{
		if( m_pGoalEnt )
		{
			if( m_pGoalEnt->pev->speed == 0 )
			{
				SetThink( &COsprey::DeployThink );
			}

			if (FBitSet(pev->spawnflags, SF_OSPREY_DONT_DEPLOY))
			{
				m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_pGoalEnt->pev->target ) ) );
			}
			else
			{
				do {
					m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_pGoalEnt->pev->target ) ) );
				} while( m_pGoalEnt->pev->speed < 400 && !HasDead() );
			}
		}
		UpdateGoal();
	}

	Flight();
	Update();
}

void COsprey::Flight()
{
	float t = ( gpGlobals->time - m_startTime );
	float scale = 1.0f / m_dTime;

	float f = UTIL_SplineFraction( t * scale, 1.0f );

	Vector pos = ( m_pos1 + m_vel1 * t ) * ( 1.0f - f ) + ( m_pos2 - m_vel2 * ( m_dTime - t ) ) * f;
	Vector ang = ( m_ang1 ) * ( 1.0f - f ) + ( m_ang2 ) * f;
	m_velocity = m_vel1 * ( 1.0f - f ) + m_vel2 * f;

	UTIL_SetOrigin( pev, pos );
	pev->angles = ang;
	UTIL_MakeAimVectors( pev->angles );
	float flSpeed = DotProduct( gpGlobals->v_forward, m_velocity );

	// float flSpeed = DotProduct( gpGlobals->v_forward, pev->velocity );

	float m_flIdealtilt = ( 160.0f - flSpeed ) / 10.0f;

	// ALERT( at_console, "%f %f\n", flSpeed, flIdealtilt );
	if( m_flRotortilt < m_flIdealtilt )
	{
		m_flRotortilt += 0.5f;
		if ( m_flRotortilt > 0 )
			m_flRotortilt = 0;
	}
	if( m_flRotortilt > m_flIdealtilt )
	{
		m_flRotortilt -= 0.5f;
		if( m_flRotortilt < -90 )
			m_flRotortilt = -90;
	}
	SetBoneController( 0, m_flRotortilt );

	if( m_iSoundState == 0 )
	{
		SoundScriptParamOverride param;
		SetRotorSoundParams(param);
		param.OverridePitchRelative(110);
		EmitSoundScript(rotorSoundScript, param);
		// EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		if( pPlayer )
		{
			float pitch = DotProduct( m_velocity - pPlayer->pev->velocity, ( pPlayer->pev->origin - pev->origin ).Normalize() );

			pitch = (int)( 100 + pitch / 75.0f );

			if( pitch > 250 ) 
				pitch = 250;
			if( pitch < 50 )
				pitch = 50;

			if( pitch == 100 )
				pitch = 101;

			if( pitch != m_iPitch )
			{
				m_iPitch = pitch;
				SoundScriptParamOverride param;
				SetRotorSoundParams(param);
				param.OverridePitchRelative((int)pitch);
				EmitSoundScript(rotorSoundScript, param, SND_CHANGE_PITCH | SND_CHANGE_VOL);
				// ALERT( at_console, "%.0f\n", pitch );
			}
		}
		// EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch );
	}

}

void COsprey::HitTouch( CBaseEntity *pOther )
{
	pev->nextthink = gpGlobals->time + 2.0f;
}

/*
int COsprey::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	if( m_flRotortilt <= -90 )
	{
		m_flRotortilt = 0;
	}
	else
	{
		m_flRotortilt -= 45;
	}
	SetBoneController( 0, m_flRotortilt );
	return 0;
}
*/

KilledResult COsprey::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3f;
	pev->velocity = m_velocity;
	pev->avelocity = Vector( RANDOM_FLOAT( -20, 20 ), 0, RANDOM_FLOAT( -50, 50 ) );
	StopSoundScript(rotorSoundScript);

	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	SetThink( &COsprey::DyingThink );
	SetTouch( &COsprey::CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;

	m_startTime = gpGlobals->time + 4.0f;
	return KilledResult();
}

void COsprey::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if( pOther->pev->solid == SOLID_BSP )
	{
		SetTouch( NULL );
		m_startTime = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
		m_velocity = pev->velocity;
	}
}

void COsprey::DyingThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->avelocity = pev->avelocity * 1.02f;

	// still falling?
	if( m_startTime > gpGlobals->time )
	{
		UTIL_MakeAimVectors( pev->angles );
		Update();

		Vector vecSpot = pev->origin + pev->velocity * 0.2f;

		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_EXPLOSION );		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( vecSpot.y + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( vecSpot.z + RANDOM_FLOAT( -150.0f, -50.0f ) );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG( 0, 29 ) + 30 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// lots of smoke
		const Vector smokePosition(
			pev->origin.x + RANDOM_FLOAT(-150.0f, 150.0f),
			pev->origin.y + RANDOM_FLOAT(-150.0f, 150.0f),
			pev->origin.z + RANDOM_FLOAT(-150.0f, -50.0f)
		);
		SendSmoke(smokePosition, GetVisual(fallingSmokeVisual));

		vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_VECTOR( vecSpot );

			// size
			WRITE_COORD( 800 );
			WRITE_COORD( 800 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_VECTOR( pev->velocity );

			// randomization
			WRITE_BYTE( 50 ); 

			// Model
			WRITE_SHORT( m_iTailGibs );	//model id#

			// # of shards
			WRITE_BYTE( 8 );	// let client decide

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2f;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 10 ); // framerate
		MESSAGE_END();
		*/

		// gibs
		SendSprite(vecSpot + Vector(0, 0, 512.0f), GetVisual(fireBallVisual));

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 6 ); // framerate
		MESSAGE_END();
		*/

		// blast circle
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_CIRCLE( pev->origin, 2000 );
			WriteBeamVisual(GetVisual(blastCircleVisual));
		MESSAGE_END();

		EmitSoundScript(crashSoundScript);

		RadiusDamage( pev->origin, pev, pev, DamageInfo{300, DMG_BLAST}, CLASS_NONE );

		// gibs
		vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_VECTOR( vecSpot + Vector(0, 0, 64) );

			// size
			WRITE_COORD( 800 );
			WRITE_COORD( 800 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( m_velocity.x ); 
			WRITE_COORD( m_velocity.y );
			WRITE_COORD( fabs( m_velocity.z ) * 0.25f );

			// randomization
			WRITE_BYTE( 40 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 128 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		UTIL_Remove( this );
	}
}

void COsprey::ShowDamage()
{
	if( m_iDoLeftSmokePuff > 0 || RANDOM_LONG( 0, 99 ) > m_flLeftHealth )
	{
		const Vector vecSrc = pev->origin + gpGlobals->v_right * -340;
		SendSmoke(vecSrc, GetVisual(damageSmokeVisual));
		if( m_iDoLeftSmokePuff > 0 )
			m_iDoLeftSmokePuff--;
	}
	if( m_iDoRightSmokePuff > 0 || RANDOM_LONG( 0, 99 ) > m_flRightHealth )
	{
		const Vector vecSrc = pev->origin + gpGlobals->v_right * 340;
		SendSmoke(vecSrc, GetVisual(damageSmokeVisual));
		if( m_iDoRightSmokePuff > 0 )
			m_iDoRightSmokePuff--;
	}
}

void COsprey::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// only so much per engine
	if( ptr->iHitgroup == 3 )
	{
		if( m_flRightHealth < 0 )
			return;
		else
			m_flRightHealth -= damageInfo.damage;
		m_iDoRightSmokePuff = 3 + ( damageInfo.damage / 5.0f );
	}

	if( ptr->iHitgroup == 2 )
	{
		if( m_flLeftHealth < 0 )
			return;
		else
			m_flLeftHealth -= damageInfo.damage;
		m_iDoLeftSmokePuff = 3 + ( damageInfo.damage / 5.0f );
	}

	// hit hard, hits cockpit, hits engines
	if( damageInfo.damage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2 || ptr->iHitgroup == 3 )
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pevInflictor, pevAttacker, this, damageInfo );
	}
	else
	{
		UTIL_Sparks( ptr->vecEndPos );
	}
}

void COsprey::Update()
{
	//Look around so AI triggers work.
	Look(4092);

	//Listen for sounds so AI triggers work.
	Listen();

	ShowDamage();
	FCheckAITrigger();
	GlowShellUpdate();
}

TakeDamageResult COsprey::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo)
{
	//Set enemy to last attacker.
	//Ospreys are not capable of fighting so they'll get angry at whatever shoots at them, not whatever looks like an enemy.
	m_hEnemy = Instance(pevAttacker);

	//It's on now!
	m_MonsterState = MONSTERSTATE_COMBAT;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, damageInfo);
}

class CBlkopOsprey : public COsprey
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("blkop_osprey"); }
	void PrepareGruntBeforeSpawn(CBaseEntity* pGrunt) override;
	int	DefaultClassify() override
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return COsprey::DefaultClassify();
	}
protected:
	const char* TrooperName() override;
};

LINK_ENTITY_TO_CLASS( monster_blkop_osprey, CBlkopOsprey )

void CBlkopOsprey::Spawn()
{
	SpawnImpl("models/blkop_osprey.mdl", gSkillData.blackopsOspreyHealth);
}

void CBlkopOsprey::Precache()
{
	PrecacheImpl("models/blkop_osprey.mdl", "models/blkop_tailgibs.mdl", "models/blkop_bodygibs.mdl", "models/blkop_enginegibs.mdl");
}

void CBlkopOsprey::PrepareGruntBeforeSpawn(CBaseEntity *pGrunt)
{
	CBaseMonster* pMonster = pGrunt->MyMonsterPointer();
	if (pMonster)
	{
		pMonster->SetHead(-1);
	}
}

const char* CBlkopOsprey::TrooperName()
{
	return "monster_male_assassin";
}
