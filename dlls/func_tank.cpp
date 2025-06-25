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
#include "effects.h"
#include "weapons.h"
#include "explode.h"

#include "monsters.h"
#include "player.h"
#include "common_soundscripts.h"

#define SF_TANK_ACTIVE			0x0001
#define SF_TANK_PLAYER			0x0002
#define SF_TANK_HUMANS			0x0004
#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_LASERSPOT		0x0040 //LRC
#define SF_TANK_MATCHTARGET		0x0080 //LRC
#define SF_TANK_SMOKE_ON_EMPTY	0x0100
#define SF_TANK_SMOKE_IN_ORIGIN	0x0200

#define SF_TANK_SOUNDON			0x8000


enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3
};

class CFuncTankControls : public CBaseEntity
{
public:
	int ObjectCaps() override;
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	bool OnControls(entvars_t* pevTest) override;

	bool m_active;
	Vector m_vecControllerUsePos;
	CBasePlayer* m_pController;
};

//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void TrackTarget( void );

	CBaseEntity* BestVisibleEnemy( void );
	int IRelationship( CBaseEntity* pTarget ) override;
	int DefaultClassify() override { return m_iTankClass; }

	virtual void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
	virtual Vector UpdateTargetPosition( CBaseEntity *pTarget )
	{
		return pTarget->BodyTarget( pev->origin );
	}

	void StartRotSound( void );
	void StopRotSound( void );

	// Bmodels don't go across transitions
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline bool IsActive( void ) { return (pev->spawnflags & SF_TANK_ACTIVE); }
	inline void TankActivate( void ) { pev->spawnflags |= SF_TANK_ACTIVE; pev->nextthink = pev->ltime + 0.1f; m_fireLast = 0; }
	inline void TankDeactivate( void ) { pev->spawnflags &= ~SF_TANK_ACTIVE; m_fireLast = 0; StopRotSound(); }
	inline bool CanFire( void ) { return (gpGlobals->time - m_lastSightTime) < m_persist; }
	bool InRange( float range );

	void		TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr );

	Vector		BarrelPosition( void )
	{
		Vector forward, right, up;
		UTIL_MakeVectorsPrivate( pev->angles, forward, right, up );
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}

	void		AdjustAnglesForBarrel( Vector &angles, float distance );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	bool StartControl(CBasePlayer* pController, CFuncTankControls* pControls);
	void StopControl(CFuncTankControls* pControls);

	CFuncTankControls* m_pControls; //LRC - tankcontrols is used as a go-between.

	virtual void StopFire() {}

	inline bool HaveBullets() { return m_bulletCount != 0; }
	void RemoveBullet();
	void OnEmptyGun();

	void UpdateOnRemove() override;

	CLaserSpot*  m_pSpot;		// Laser spot entity

protected:
	float		m_flNextAttack;
	
	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
								// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle

	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle

	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// How many rounds/second
	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track
	float		m_maxRange;		// Max range to aim/track

	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_spriteScale;	// Scale of any sprites we shoot
	string_t	m_iszSpriteSmoke;
	string_t	m_iszSpriteFlash;
	TANKBULLET	m_bulletType;	// Bullet type
	int			m_iBulletDamage; // 0 means use Bullet type's default damage
	
	Vector		m_sightOrigin;	// Last sight of target
	int			m_spread;		// firing spread
	string_t	m_iszMaster;	// Master entity (game_team_master or multisource)
	int			m_bulletCount;	// Bullet count left. Negative means infinite.
	float		m_flEmptySoundTime;
	short		m_smokeRenderMode;

	int			m_iTankClass;	// Behave As

	void UpdateSpot( void );

	CPointEntity* m_pFireProxy; //LRC - locus position for custom shots
	string_t m_iszLocusFire;

	CBaseEntity* BarrelFireProxy();
	void UpdateBarrelFireProxyPosition();

	float m_checkOriginTime;
	bool m_originIsInWorldBrush;

	void UpdateOriginCheck();
};

TYPEDESCRIPTION	CFuncTank::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTank, m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_persist, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteSmoke, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteFlash, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_sightOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spread, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pControls, FIELD_CLASSPTR ), //LRC
	DEFINE_FIELD( CFuncTank, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_iBulletDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_iszMaster, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_bulletCount, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pSpot, FIELD_CLASSPTR ), //LRC
	DEFINE_FIELD( CFuncTank, m_smokeRenderMode, FIELD_SHORT ),
	DEFINE_FIELD( CFuncTank, m_iTankClass, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pFireProxy, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTank, m_iszLocusFire, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncTank, CBaseEntity )

static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};

#define MAX_FIRING_SPREADS ARRAYSIZE( gTankSpread )

void CFuncTank::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid = SOLID_BSP;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if( IsActive() )
		pev->nextthink = pev->ltime + 1.0f;

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if( m_fireRate <= 0 )
		m_fireRate = 1;
	if( m_spread > (int)MAX_FIRING_SPREADS )
		m_spread = 0;

	pev->oldorigin = pev->origin;
	
	if (!m_bulletCount) {
		m_bulletCount = -1;
	}
}

void CFuncTank::Precache( void )
{
	if( m_iszSpriteSmoke )
		PRECACHE_MODEL( STRING( m_iszSpriteSmoke ) );

	if( m_iszSpriteFlash )
		PRECACHE_MODEL( STRING( m_iszSpriteFlash ) );

	if( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );

	RegisterAndPrecacheSoundScript(Items::weaponEmptySoundScript);
}

void CFuncTank::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "yawrate" ) )
	{
		m_yawRate = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "yawrange" ) )
	{
		m_yawRange = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "yawtolerance" ) )
	{
		m_yawTolerance = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchrange" ) )
	{
		m_pitchRange = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchrate" ) )
	{
		m_pitchRate = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchtolerance" ) )
	{
		m_pitchTolerance = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "firerate" ) )
	{
		m_fireRate = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "barrel" ) )
	{
		m_barrelPos.x = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "barrely" ) )
	{
		m_barrelPos.y = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "barrelz" ) )
	{
		m_barrelPos.z = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spritescale" ) )
	{
		m_spriteScale = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spritesmoke" ) )
	{
		m_iszSpriteSmoke = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spriteflash" ) )
	{
		m_iszSpriteFlash = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "rotatesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "persistence" ) )
	{
		m_persist = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "bullet" ) )
	{
		m_bulletType = (TANKBULLET)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "bullet_damage" ) )
	{
		m_iBulletDamage = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "firespread" ) )
	{
		m_spread = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "minRange" ) )
	{
		m_minRange = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "maxRange" ) )
	{
		m_maxRange = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "bulletCount" ) )
	{
		m_bulletCount = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_iszMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "smokerendermode" ) )
	{
		m_smokeRenderMode = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iClass"))
	{
		m_iTankClass = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszLocusFire"))
	{
		m_iszLocusFire = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

bool CFuncTank::StartControl(CBasePlayer* pController, CFuncTankControls* pControls)
{
	if( m_pControls != nullptr )
		return false;

	// Team only or disabled?
	if( m_iszMaster )
	{
		if( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
			return false;
	}

	if (m_pSpot)
		m_pSpot->Revive();

	m_pControls = pControls;

	pev->nextthink = pev->ltime + 0.1f;
	return true;
}

void CFuncTank::StopControl(CFuncTankControls* pControls)
{
	StopFire();

	//LRC- various commands moved from here to FuncTankControls
	if (!m_pControls || m_pControls != pControls)
	{
		//ALERT(at_debug,"StopControl failed, not in use\n");
		return;
	}

	if (m_pSpot)
		m_pSpot->pev->effects |= EF_NODRAW;

	StopRotSound();

	pev->nextthink = 0;
	m_pControls = nullptr;

	if( IsActive() )
		pev->nextthink = pev->ltime + 1.0f;
}

void CFuncTank::UpdateSpot( void )
{
	if ( pev->spawnflags & SF_TANK_LASERSPOT )
	{
		if (!m_pSpot && !FBitSet(pev->flags, FL_KILLME))
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		if (!m_pSpot)
			return;

		Vector vecAiming;
		UTIL_MakeVectorsPrivate( pev->angles, vecAiming, NULL, NULL );
		Vector vecSrc = BarrelPosition( );

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(pev), &tr );

		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
}

CBaseEntity* CFuncTank::BarrelFireProxy()
{
	if (!m_pFireProxy)
	{
		m_pFireProxy = GetClassPtr((CPointEntity*)NULL);
		m_pFireProxy->pev->classname = MAKE_STRING("info_target");
		UpdateBarrelFireProxyPosition();
	}
	return m_pFireProxy;
}

void CFuncTank::UpdateBarrelFireProxyPosition()
{
	if (m_pFireProxy)
	{
		m_pFireProxy->pev->origin = BarrelPosition();
		UTIL_MakeVectorsPrivate(pev->angles, m_pFireProxy->pev->velocity, NULL, NULL);
	}
}

void CFuncTank::UpdateOnRemove()
{
	StopControl(m_pControls);
	CBaseEntity::UpdateOnRemove();
	if (m_pSpot) {
		UTIL_Remove(m_pSpot);
		m_pSpot = NULL;
	}
	if (m_pFireProxy) {
		m_pFireProxy->SetThink(&CBaseEntity::SUB_Remove);
		m_pFireProxy->pev->nextthink = gpGlobals->time + 0.1f;
		m_pFireProxy = NULL;
	}
}

void CFuncTank::RemoveBullet()
{
	if (m_bulletCount > 0) {
		m_bulletCount--;
		if (!m_bulletCount) {
			if (pev->spawnflags & SF_TANK_SMOKE_ON_EMPTY) {
				Vector pos = (pev->spawnflags & SF_TANK_SMOKE_IN_ORIGIN) ? pev->origin : BarrelPosition();
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_SMOKE );
					WRITE_VECTOR( pos );
					WRITE_SHORT( g_sModelIndexSmoke );
					WRITE_BYTE( 20 ); // scale * 10
					WRITE_BYTE( 5 ); // framerate
				MESSAGE_END();
			}
		}
	}
}

void CFuncTank::OnEmptyGun()
{
	if (m_flEmptySoundTime < gpGlobals->time) {
		EmitSoundScript(Items::weaponEmptySoundScript);
		m_flEmptySoundTime = gpGlobals->time + 0.5;
	}
}

////////////// END NEW STUFF //////////////

void CFuncTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->spawnflags & SF_TANK_CANCONTROL )
	{
		// player controlled turret
		if( !pActivator || pActivator->Classify() != CLASS_PLAYER )
		{
			ALERT(at_warning, "Triggering a controllable %s with non-player activator. Probably a mapping error?\n", STRING(pev->classname));
			return;
		}
	}
	else
	{
		if( !ShouldToggle( useType, IsActive() ) )
			return;

		if( IsActive() )
		{
			TankDeactivate();
			if (m_pSpot)
				m_pSpot->pev->effects |= EF_NODRAW;
		}
		else
		{
			TankActivate();
			if (m_pSpot) m_pSpot->Revive();
		}
	}
}

void CFuncTank::UpdateOriginCheck()
{
	if (m_checkOriginTime < gpGlobals->time)
	{
		m_checkOriginTime = gpGlobals->time + 2.0f;
		m_originIsInWorldBrush = UTIL_PointContents(pev->origin) == CONTENTS_SOLID;
	}
}

CBaseEntity *CFuncTank:: BestVisibleEnemy ( void )
{
	// PVS check won't work if origin is in world
	if (m_iTankClass == 0)
	{
		UpdateOriginCheck();
		if (!m_originIsInWorldBrush)
		{
			edict_t *pPlayer = FIND_CLIENT_IN_PVS( edict() );
			return FNullEnt(pPlayer) ? NULL : CBaseEntity::Instance(pPlayer);
		}
	}

	CBaseEntity	*pReturn = NULL;
	int			iNearest;
	int			iDist;
	int			iBestRelationship;
	int			iLookDist = m_maxRange?m_maxRange:512; //thanks to Waldo for this.

	iNearest = 8192;// so first visible entity will become the closest.
	iBestRelationship = R_DL;

	CBaseEntity *pList[100];

	Vector delta = Vector( iLookDist, iLookDist, iLookDist );

	// Find only monsters/clients in box, NOT limited to PVS
	int count = UTIL_EntitiesInBox( pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT|FL_MONSTER );
	int i;

	for (i = 0; i < count; i++ )
	{
		const int iRelationship = IRelationship( pList[i] );
		if ( pList[i]->IsFullyAlive() )
		{
			if ( iRelationship > iBestRelationship )
			{
				// this entity is disliked MORE than the entity that we
				// currently think is the best visible enemy. No need to do
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship ( pList[i] );
				iNearest = ( pList[i]->pev->origin - pev->origin ).Length();
				pReturn = pList[i];
			}
			else if ( iRelationship == iBestRelationship )
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = ( pList[i]->pev->origin - pev->origin ).Length();

				if ( iDist <= iNearest )
				{
					iNearest = iDist;
					//these are guaranteed to be the same! iBestRelationship = IRelationship ( pList[i] );
					pReturn = pList[i];
				}
			}
		}
	}

//	if (pReturn)
//		ALERT(at_debug, "Tank's best enemy is %s\n", STRING(pReturn->pev->classname));
//	else
//		ALERT(at_debug, "Tank has no best enemy\n");
	return pReturn;
}

int	CFuncTank::IRelationship( CBaseEntity* pTarget )
{
	if (m_iTankClass == 0)
	{
		if (pTarget->IsPlayer())
			return R_HT;
		else
			return R_NO;
	}
	return CBaseMonster::IDefaultRelationship(Classify(), pTarget->Classify());
}

bool CFuncTank::InRange( float range )
{
	if( range < m_minRange )
		return false;
	if( m_maxRange > 0 && range > m_maxRange )
		return false;

	return true;
}

void CFuncTank::Think( void )
{
	pev->avelocity = g_vecZero;
	TrackTarget();

	if( fabs( pev->avelocity.x ) > 1 || fabs( pev->avelocity.y ) > 1 )
		StartRotSound();
	else
		StopRotSound();
}

void CFuncTank::TrackTarget( void )
{
	TraceResult tr;
	bool updateTime = false;
	Vector angles, direction, targetPosition, barrelEnd;
	CBaseEntity* pTarget = nullptr;
	CBasePlayer* pController = nullptr;

	UpdateBarrelFireProxyPosition();

	// Get a position to aim for
	if (m_pControls && m_pControls->m_pController)
	{
		UpdateSpot();
		pController = m_pControls->m_pController;

		// Tanks attempt to mirror the player's angles
		angles = pController->pev->v_angle;
		angles[0] = 0 - angles[0];
		pev->nextthink = pev->ltime + 0.05f;
	}
	else
	{
		if( IsActive() )
			pev->nextthink = pev->ltime + 0.1f;
		else
			return;

		UpdateSpot();

		pTarget = BestVisibleEnemy();
		if (!pTarget)
		{
			if( IsActive() )
				pev->nextthink = pev->ltime + 2.0f;	// Wait 2 secs
			return;
		}

		// Calculate angle needed to aim at target
		barrelEnd = BarrelPosition();
		targetPosition = pTarget->pev->origin + pTarget->pev->view_ofs;
		float range = ( targetPosition - barrelEnd ).Length();

		if( !InRange( range ) )
			return;

		UTIL_TraceLine( barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr );

		if( tr.flFraction == 1.0f || tr.pHit == pTarget->edict() )
		{
			if( InRange( range ) && pTarget && pTarget->IsAlive() )
			{
				updateTime = true;
				m_sightOrigin = UpdateTargetPosition( pTarget );
			}
		}

		// Track sight origin
		// !!! I'm not sure what i changed
		direction = m_sightOrigin - pev->origin;
		//direction = m_sightOrigin - barrelEnd;
		angles = UTIL_VecToAngles( direction );

		// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center) 
		AdjustAnglesForBarrel( angles, direction.Length() );
	}

	angles.x = -angles.x;

	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance( angles.y, m_yawCenter );
	angles.x = m_pitchCenter + UTIL_AngleDistance( angles.x, m_pitchCenter );

	// Limit against range in y
	if( angles.y > m_yawCenter + m_yawRange )
	{
		angles.y = m_yawCenter + m_yawRange;
		updateTime = false;	// Don't update if you saw the player, but out of range
	}
	else if( angles.y < ( m_yawCenter - m_yawRange ) )
	{
		angles.y = ( m_yawCenter - m_yawRange );
		updateTime = false; // Don't update if you saw the player, but out of range
	}

	if( updateTime )
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
	pev->avelocity.y = distY * 10;
	if( pev->avelocity.y > m_yawRate )
		pev->avelocity.y = m_yawRate;
	else if( pev->avelocity.y < -m_yawRate )
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if( angles.x > m_pitchCenter + m_pitchRange )
		angles.x = m_pitchCenter + m_pitchRange;
	else if( angles.x < m_pitchCenter - m_pitchRange )
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( angles.x, pev->angles.x );
	pev->avelocity.x = distX  * 10;

	if( pev->avelocity.x > m_pitchRate )
		pev->avelocity.x = m_pitchRate;
	else if( pev->avelocity.x < -m_pitchRate )
		pev->avelocity.x = -m_pitchRate;

	if( pController )
	{
		if ( gpGlobals->time < m_flNextAttack )
			return;

		if ( pController->pev->button & IN_ATTACK )
		{
			Vector forward;
			UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

			// to make sure the gun doesn't fire too many bullets
			m_fireLast = gpGlobals->time - (1.0f / m_fireRate) - 0.01f;

			Fire( BarrelPosition(), forward, pController->pev );

			// HACKHACK -- make some noise (that the AI can hear)
			if ( pController && pController->IsPlayer() )
				((CBasePlayer *)pController)->m_iWeaponVolume = LOUD_GUN_VOLUME;

			m_flNextAttack = gpGlobals->time + (1.0f / m_fireRate);
		}
	}
	else if( CanFire() && ( ( fabs( distX ) < m_pitchTolerance && fabs( distY ) < m_yawTolerance ) || ( pev->spawnflags & SF_TANK_LINEOFSIGHT ) ) )
	{
		bool fire = false;
		Vector forward;
		UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

		if( pev->spawnflags & SF_TANK_LINEOFSIGHT )
		{
			float length = direction.Length();
			UTIL_TraceLine( barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr );
			if( tr.pHit == pTarget->edict() )
				fire = true;
		}
		else
			fire = true;

		if( fire )
		{
			Fire( BarrelPosition(), forward, pev );
		}
		else
			m_fireLast = 0;
	}
	else
		m_fireLast = 0;
}

// If barrel is offset, add in additional rotation
void CFuncTank::AdjustAnglesForBarrel( Vector &angles, float distance )
{
	float r2, d2;

	if( m_barrelPos.y != 0 || m_barrelPos.z != 0 )
	{
		distance -= m_barrelPos.z;
		d2 = distance * distance;
		if( m_barrelPos.y )
		{
			r2 = m_barrelPos.y * m_barrelPos.y;
			if( d2 > r2 )
				angles.y += ( 180.0f / M_PI_F ) * atan2( m_barrelPos.y, sqrt( d2 - r2 ) );
		}
		if( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			if( d2 > r2 )
				angles.x += ( 180.0f / M_PI_F ) * atan2( -m_barrelPos.z, sqrt( d2 - r2 ) );
		}
	}
}

// Fire targets and spawn sprites
void CFuncTank::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if (!HaveBullets()) {
		OnEmptyGun();
		return;
	}
	
	if( m_fireLast != 0 )
	{
		if( m_iszSpriteSmoke )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteSmoke ), barrelEnd, true );
			pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0f, 20.0f ) );
			pSprite->SetTransparency( m_smokeRenderMode ? m_smokeRenderMode : kRenderTransAlpha, (int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z, 255, kRenderFxNone );
			pSprite->pev->velocity.z = RANDOM_FLOAT( 40.0f, 80.0f );
			pSprite->SetScale( m_spriteScale );
		}
		if( m_iszSpriteFlash )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteFlash ), barrelEnd, true );
			pSprite->AnimateAndDie( 60 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );

			// Hack Hack, make it stick around for at least 100 ms.
			pSprite->pev->nextthink += 0.1f;
		}

		//LRC
		if (!FStringNull(m_iszLocusFire))
		{
			CBaseEntity* pFireProxy = BarrelFireProxy();
			FireTargets(STRING(m_iszLocusFire), pFireProxy, this);
		}

		SUB_UseTargets( this );
	}
	m_fireLast = gpGlobals->time;
}

void CFuncTank::TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr )
{
	// get circular gaussian spread
	float x, y, z;
	do
	{
		x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
		y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
		z = x * x + y * y;
	} while( z > 1 );
	Vector vecDir = vecForward +
		x * vecSpread.x * gpGlobals->v_right +
		y * vecSpread.y * gpGlobals->v_up;
	Vector vecEnd;

	vecEnd = vecStart + vecDir * 4096;
	UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &tr );
}
	
void CFuncTank::StartRotSound( void )
{
	if( !pev->noise || ( pev->spawnflags & SF_TANK_SOUNDON ) )
		return;
	pev->spawnflags |= SF_TANK_SOUNDON;
	EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise ), 0.85, ATTN_NORM );
}

void CFuncTank::StopRotSound( void )
{
	if( pev->spawnflags & SF_TANK_SOUNDON )
		STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noise ) );
	pev->spawnflags &= ~SF_TANK_SOUNDON;
}

class CFuncTankGun : public CFuncTank
{
public:
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};

LINK_ENTITY_TO_CLASS( func_tank, CFuncTankGun )
LINK_ENTITY_TO_CLASS( func_tank_of, CFuncTankGun )

static int TankBulletToBulletType(int tankBullet)
{
	switch (tankBullet) {
	case TANK_BULLET_9MM:
		return BULLET_MONSTER_9MM;
	case TANK_BULLET_MP5:
		return BULLET_MONSTER_MP5;
	case TANK_BULLET_12MM:
		return BULLET_MONSTER_12MM;
	default:
		return BULLET_NONE;
	}
}

void CFuncTankGun::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;
	
	if (!HaveBullets()) {
		OnEmptyGun();
		return;
	}

	if( m_fireLast != 0 )
	{
		// FireBullets needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors( pev->angles );

		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		if( bulletCount > 0 )
		{
			for( i = 0; i < bulletCount && HaveBullets(); i++ )
			{
				if (m_bulletType != TANK_BULLET_NONE)
				{
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, TankBulletToBulletType(m_bulletType), 1, m_iBulletDamage, pevAttacker );
					RemoveBullet();
				}
				else
					break;
			}
			CFuncTank::Fire( barrelEnd, forward, pevAttacker );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pevAttacker );
}

class CFuncTankLaser : public CFuncTank
{
public:
	void Activate( void );
	void KeyValue( KeyValueData *pkvd );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
	void Think( void );
	CLaser *GetLaser( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	void StopFire() override;

	void UpdateOnRemove();

private:
	CLaser *m_pLaser;
	float m_laserTime;
};

LINK_ENTITY_TO_CLASS( func_tanklaser, CFuncTankLaser )
LINK_ENTITY_TO_CLASS( func_tanklaser_of, CFuncTankLaser )

TYPEDESCRIPTION	CFuncTankLaser::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTankLaser, m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankLaser, m_laserTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFuncTankLaser, CFuncTank )

void CFuncTankLaser::Activate( void )
{
	CFuncTank::Activate();

	if( !GetLaser() )
	{
		UTIL_Remove( this );
		ALERT( at_error, "Laser tank with no env_laser!\n" );
	}
	else
	{
		ClearBits(pev->flags, FL_ALWAYSTHINK);
		m_pLaser->TurnOff();
	}
}

void CFuncTankLaser::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "laserentity" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

CLaser *CFuncTankLaser::GetLaser( void )
{
	if (FBitSet(pev->flags, FL_KILLME))
		return NULL;

	if( m_pLaser )
		return m_pLaser;

	edict_t	*pentLaser;

	pentLaser = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->message ) );
	while( !FNullEnt( pentLaser ) )
	{
		// Found the landmark
		if( FClassnameIs( pentLaser, "env_laser" ) )
		{
			m_pLaser = (CLaser *)CBaseEntity::Instance( pentLaser );
			break;
		}
		else
			pentLaser = FIND_ENTITY_BY_TARGETNAME( pentLaser, STRING( pev->message ) );
	}

	return m_pLaser;
}

void CFuncTankLaser::Think( void )
{
	if( m_pLaser && (gpGlobals->time > m_laserTime) && m_pLaser->IsOn() ) {
		ClearBits(pev->flags, FL_ALWAYSTHINK);
		m_pLaser->TurnOff();
	}

	CFuncTank::Think();
}

void CFuncTankLaser::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if (!HaveBullets()) {
		OnEmptyGun();
		return;
	}
	
	int i;
	TraceResult tr;

	if( m_fireLast != 0 && GetLaser() )
	{
		// TankTrace needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors( pev->angles );

		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		if( bulletCount )
		{
			for( i = 0; i < bulletCount && HaveBullets(); i++ )
			{
				m_pLaser->pev->origin = barrelEnd;
				TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

				m_laserTime = gpGlobals->time;
				m_pLaser->TurnOn();
				if (pev->solid != SOLID_NOT)
					SetBits(pev->flags, FL_ALWAYSTHINK);
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0f;
				m_pLaser->FireAtPoint( tr, pevAttacker );
				m_pLaser->pev->nextthink = 0;
				RemoveBullet();
			}
			CFuncTank::Fire( barrelEnd, forward, pevAttacker );
		}
	}
	else
	{
		CFuncTank::Fire( barrelEnd, forward, pevAttacker );
	}
}

void CFuncTankLaser::StopFire( void )
{
	if( m_pLaser )
		m_pLaser->TurnOff();
}

void CFuncTankLaser::UpdateOnRemove()
{
	CFuncTank::UpdateOnRemove();
	m_pLaser = NULL;
}

class CFuncTankRocket : public CFuncTank
{
public:
	void Precache( void );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};

LINK_ENTITY_TO_CLASS( func_tankrocket, CFuncTankRocket )
LINK_ENTITY_TO_CLASS( func_tankrocket_of, CFuncTankRocket )

void CFuncTankRocket::Precache( void )
{
	UTIL_PrecacheOther( "rpg_rocket" );
	CFuncTank::Precache();
}

void CFuncTankRocket::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if (!HaveBullets()) {
		OnEmptyGun();
		return;
	}
	
	int i;

	if( m_fireLast != 0 )
	{
		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		if( bulletCount > 0 )
		{
			for( i = 0; i < bulletCount && HaveBullets(); i++ )
			{
				CBaseEntity* owner = FNullEnt(pevAttacker) ? NULL : CBaseEntity::Instance(pevAttacker);
				CBaseEntity::Create( "rpg_rocket", barrelEnd, pev->angles, owner ? owner->edict() : edict() );
			}
			RemoveBullet();
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}

class CFuncTankMortar : public CFuncTank
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};

LINK_ENTITY_TO_CLASS( func_tankmortar, CFuncTankMortar )
LINK_ENTITY_TO_CLASS( func_tankmortar_of, CFuncTankMortar )

void CFuncTankMortar::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "iMagnitude" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

void CFuncTankMortar::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if (!HaveBullets()) {
		OnEmptyGun();
		return;
	}
	
	if( m_fireLast != 0 )
	{
		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		// Only create 1 explosion
		if( bulletCount > 0 )
		{
			TraceResult tr;

			// TankTrace needs gpGlobals->v_up, etc.
			UTIL_MakeAimVectors( pev->angles );

			TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

			ExplosionCreate( tr.vecEndPos, pev->angles, edict(), pev->impulse, true, pevAttacker );

			RemoveBullet();
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}

//============================================================================
// FUNC TANK CONTROLS
//============================================================================

LINK_ENTITY_TO_CLASS( func_tankcontrols, CFuncTankControls )
LINK_ENTITY_TO_CLASS( func_tankcontrols_of, CFuncTankControls )

TYPEDESCRIPTION	CFuncTankControls::m_SaveData[] =
{
	DEFINE_FIELD(CFuncTankControls, m_active, FIELD_BOOLEAN),
	DEFINE_FIELD(CFuncTankControls, m_pController, FIELD_CLASSPTR),
	DEFINE_FIELD(CFuncTankControls, m_vecControllerUsePos, FIELD_VECTOR)
};

IMPLEMENT_SAVERESTORE( CFuncTankControls, CBaseEntity )

int CFuncTankControls::ObjectCaps( void ) 
{ 
	return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_IMPULSE_USE; 
}

bool CFuncTankControls::OnControls(entvars_t* pevTest)
{
	if ((m_vecControllerUsePos - pevTest->origin).IsLengthLessThan(30))
		return true;

	return false;
}

void CFuncTankControls::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// LRC- rewritten to allow TankControls to be the thing that handles the relationship
	// between the player and one or more faithful tanks.
	CBaseEntity* tryTank = nullptr;

	if ( !m_pController && useType != USE_OFF )
	{
		// if not activated by a player, don't work.
		if (!pActivator || !pActivator->IsPlayer())
			return;
		CBasePlayer* pPlayer = (CBasePlayer*)pActivator;
		// if I've already got a controller, or the player's already using
		// another controls, then forget it.
		if (m_active || pPlayer->m_hTankControls != 0)
			return;

		//LRC- Now uses FindEntityByTargetname, so that aliases work.
		//TODO: we don't support aliases in Featureful yet
		while( ( tryTank = UTIL_FindEntityByTargetname( tryTank, STRING( pev->target ) ) ) )
		{
			if (!strncmp( STRING(tryTank->pev->classname), "func_tank", 9 ))
			{
				if (((CFuncTank*)tryTank)->StartControl(pPlayer, this))
				{
					//ALERT(at_console,"started controlling tank %s\n",STRING(tryTank->pev->targetname));
					// here's a tank we can control. Phew.
					m_active = true;
				}
			}
		}
		if (m_active)
		{
			// we found at least one tank to use, so holster player's weapon
			m_pController = pPlayer;
			m_pController->m_hTankControls = this;
			if ( m_pController->m_pActiveItem )
			{
				m_pController->m_pActiveItem->Holster();
				m_pController->pev->weaponmodel = 0;
				m_pController->pev->viewmodel = 0;
			}

			m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;

			// remember where the player's standing, so we can tell when he walks away
			m_vecControllerUsePos = m_pController->pev->origin;
		}
	}
	else if (m_pController && useType != USE_ON)
	{
		// player stepped away or died, most likely.
		//ALERT(at_console, "TANK controls deactivated\n");

		//LRC- Now uses FindEntityByTargetname, so that aliases work.
		while( ( tryTank = UTIL_FindEntityByTargetname( tryTank, STRING( pev->target ) ) ) )
		{
			if( FClassnameIs( tryTank->pev, "func_tank" ) ||
				FClassnameIs( tryTank->pev, "func_tanklaser" ) ||
				FClassnameIs( tryTank->pev, "func_tankmortar" ) ||
				FClassnameIs( tryTank->pev, "func_tankrocket" ) )
			{
				// this is a tank we're controlling.
				((CFuncTank*)tryTank)->StopControl(this);
			}
		}

		// bring back player's weapons
		if ( m_pController->m_pActiveItem )
			m_pController->m_pActiveItem->Deploy();

		m_pController->m_iHideHUD &= ~(HIDEHUD_WEAPONS);
		m_pController->m_hTankControls = 0;

		m_pController = nullptr;
		m_active = false;
	}
}

void CFuncTankControls::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	//pev->nextthink = gpGlobals->time + 0.3f;	// After all the func_tank's have spawned

	CBaseEntity::Spawn();
}
