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

//	-------------------------------------------
//
//	maprules.cpp
//
//	This module contains entities for implementing/changing game
//	rules dynamically within each map (.BSP)
//
//	-------------------------------------------

#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "gamerules.h"
//#include "maprules.h" //empty file
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "game.h"

class CRuleEntity : public CBaseEntity
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	SetMaster( string_t iszMaster ) { m_iszMaster = iszMaster; }

protected:
	bool	CanFireForActivator( CBaseEntity *pActivator );

private:
	string_t	m_iszMaster;
};

TYPEDESCRIPTION	CRuleEntity::m_SaveData[] = 
{
	DEFINE_FIELD( CRuleEntity, m_iszMaster, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CRuleEntity, CBaseEntity )

void CRuleEntity::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= EF_NODRAW;
}

void CRuleEntity::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "master" ) )
	{
		SetMaster( ALLOC_STRING( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

bool CRuleEntity::CanFireForActivator( CBaseEntity *pActivator )
{
	if( m_iszMaster )
	{
		return UTIL_IsMasterTriggered( m_iszMaster, pActivator );
	}
	return true;
}

// 
// CRulePointEntity -- base class for all rule "point" entities (not brushes)
//
class CRulePointEntity : public CRuleEntity
{
public:
	void		Spawn( void );
};

void CRulePointEntity::Spawn( void )
{
	CRuleEntity::Spawn();
	pev->frame			= 0;
	pev->model			= 0;
}

// 
// CRuleBrushEntity -- base class for all rule "brush" entities (not brushes)
// Default behavior is to set up like a trigger, invisible, but keep the model for volume testing
//
class CRuleBrushEntity : public CRuleEntity
{
public:
	void		Spawn( void );
	int ObjectCaps() { return CRuleEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
};

void CRuleBrushEntity::Spawn( void )
{
	SET_MODEL( edict(), STRING(pev->model) );
	CRuleEntity::Spawn();
}

// CGameScore / game_score	-- award points to player / team 
//	Points +/- total
//	Flag: Allow negative scores					SF_SCORE_NEGATIVE
//	Flag: Award points to team in teamplay		SF_SCORE_TEAM

#define SF_SCORE_NEGATIVE			0x0001
#define SF_SCORE_TEAM				0x0002

class CGameScore : public CRulePointEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	inline	int		Points( void ) { return (int)pev->frags; }
	inline	bool	AllowNegativeScore( void ) { return pev->spawnflags & SF_SCORE_NEGATIVE; }
	inline	bool	AwardToTeam( void ) { return pev->spawnflags & SF_SCORE_TEAM; }

	inline	void	SetPoints( int points ) { pev->frags = points; }

private:
};

LINK_ENTITY_TO_CLASS( game_score, CGameScore )

void CGameScore::Spawn( void )
{
	CRulePointEntity::Spawn();
}

void CGameScore::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "points" ) )
	{
		SetPoints( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameScore::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	// Only players can use this
	if( pActivator && pActivator->IsPlayer() )
	{
		if( AwardToTeam() )
		{
			pActivator->AddPointsToTeam( Points(), AllowNegativeScore() );
		}
		else
		{
			pActivator->AddPoints( Points(), AllowNegativeScore() );
		}
	}
}

// CGameEnd / game_end	-- Ends the game in MP

class CGameEnd : public CRulePointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
private:
};

LINK_ENTITY_TO_CLASS( game_end, CGameEnd )

void CGameEnd::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	g_pGameRules->EndMultiplayerGame();
}

//
// CGameText / game_text	-- NON-Localized HUD Message (use env_message to display a titles.txt message)
//	Flag: All players					SF_ENVTEXT_ALLPLAYERS
//

#define SF_ENVTEXT_ALLPLAYERS			0x0001
#define SF_ENVTEXT_ONLY_ONCE			0x0002

class CGameText : public CRulePointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	inline	bool	MessageToAll( void ) { return (pev->spawnflags & SF_ENVTEXT_ALLPLAYERS); }
	inline	void	MessageSet( const char *pMessage ) { pev->message = ALLOC_STRING(pMessage); }
	inline	const char *MessageGet( void )	{ return STRING(pev->message); }

	void EXPORT TriggerThink();
private:
	hudtextparms_t	m_textParms;
	EHANDLE m_hActivator;
};

LINK_ENTITY_TO_CLASS( game_text, CGameText )

// Save parms as a block.  Will break save/restore if the structure changes, but this entity didn't ship with Half-Life, so
// it can't impact saved Half-Life games.
TYPEDESCRIPTION	CGameText::m_SaveData[] = 
{
	DEFINE_ARRAY( CGameText, m_textParms, FIELD_CHARACTER, sizeof(hudtextparms_t) ),
	DEFINE_FIELD( CGameText, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CGameText, CRulePointEntity )

void CGameText::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "channel" ) )
	{
		m_textParms.channel = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "x" ) )
	{
		m_textParms.x = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "y" ) )
	{
		m_textParms.y = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "effect" ) )
	{
		m_textParms.effect = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "color" ) )
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, pkvd->szValue );
		m_textParms.r1 = color[0];
		m_textParms.g1 = color[1];
		m_textParms.b1 = color[2];
		m_textParms.a1 = color[3];
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "color2" ) )
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, pkvd->szValue );
		m_textParms.r2 = color[0];
		m_textParms.g2 = color[1];
		m_textParms.b2 = color[2];
		m_textParms.a2 = color[3];
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "fadein" ) )
	{
		m_textParms.fadeinTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "fadeout" ) )
	{
		m_textParms.fadeoutTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		m_textParms.holdTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "fxtime" ) )
	{
		m_textParms.fxTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameText::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( MessageToAll() )
	{
		UTIL_HudMessageAll( m_textParms, MessageGet() );
	}
	else
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if( pPlayer && pPlayer->IsNetClient() )
		{
			UTIL_HudMessage( pPlayer, m_textParms, MessageGet() );
		}
	}

	if (pev->target)
	{
		m_hActivator = pActivator;
		SetThink(&CGameText::TriggerThink);
		pev->nextthink = gpGlobals->time + m_textParms.fadeinTime + m_textParms.holdTime + m_textParms.fadeoutTime;
	}
	else if (pev->spawnflags & SF_ENVTEXT_ONLY_ONCE)
	{
		SetThink(&CGameText::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

//LRC
void CGameText::TriggerThink()
{
	SUB_UseTargets(m_hActivator, USE_TOGGLE);

	if (pev->spawnflags & SF_ENVTEXT_ONLY_ONCE)
	{
		SetThink(&CGameText::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

//
// CGameTeamMaster / game_team_master	-- "Masters" like multisource, but based on the team of the activator
// Only allows mastered entity to fire if the team matches my team
//
// team index (pulled from server team list "mp_teamlist"
// Flag: Remove on Fire
// Flag: Any team until set?		-- Any team can use this until the team is set (otherwise no teams can use it)
//

#define SF_TEAMMASTER_FIREONCE			0x0001
#define SF_TEAMMASTER_ANYTEAM			0x0002

class CGameTeamMaster : public CRulePointEntity
{
public:
	void		KeyValue( KeyValueData *pkvd );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int			ObjectCaps( void ) { return CRulePointEntity:: ObjectCaps() | FCAP_MASTER; }

	bool		IsTriggered( CBaseEntity *pActivator );
	const char	*TeamID( void );
	inline bool RemoveOnFire( void ) { return (pev->spawnflags & SF_TEAMMASTER_FIREONCE); }
	inline bool AnyTeam( void ) { return (pev->spawnflags & SF_TEAMMASTER_ANYTEAM); }

private:
	bool		TeamMatch( CBaseEntity *pActivator );

	int			m_teamIndex;
	USE_TYPE	triggerType;
};

LINK_ENTITY_TO_CLASS( game_team_master, CGameTeamMaster )

void CGameTeamMaster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "teamindex" ) )
	{
		m_teamIndex = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = true;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameTeamMaster::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( useType == USE_SET )
	{
		if( value < 0 )
		{
			m_teamIndex = -1;
		}
		else
		{
			m_teamIndex = g_pGameRules->GetTeamIndex( pActivator->TeamID() );
		}
		return;
	}

	if( TeamMatch( pActivator ) )
	{
		SUB_UseTargets( pActivator, triggerType, value );
		if( RemoveOnFire() )
			UTIL_Remove( this );
	}
}

bool CGameTeamMaster::IsTriggered( CBaseEntity *pActivator )
{
	return TeamMatch( pActivator );
}

const char *CGameTeamMaster::TeamID( void )
{
	if( m_teamIndex < 0 )		// Currently set to "no team"
		return "";

	return g_pGameRules->GetIndexedTeamName( m_teamIndex );		// UNDONE: Fill this in with the team from the "teamlist"
}

bool CGameTeamMaster::TeamMatch( CBaseEntity *pActivator )
{
	if( m_teamIndex < 0 && AnyTeam() )
		return true;

	if( !pActivator )
		return false;

	return UTIL_TeamsMatch( pActivator->TeamID(), TeamID() );
}

//
// CGameTeamSet / game_team_set	-- Changes the team of the entity it targets to the activator's team
// Flag: Fire once
// Flag: Clear team				-- Sets the team to "NONE" instead of activator

#define SF_TEAMSET_FIREONCE			0x0001
#define SF_TEAMSET_CLEARTEAM		0x0002

class CGameTeamSet : public CRulePointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline bool RemoveOnFire( void ) { return (pev->spawnflags & SF_TEAMSET_FIREONCE); }
	inline bool ShouldClearTeam( void ) { return (pev->spawnflags & SF_TEAMSET_CLEARTEAM); }

private:
};

LINK_ENTITY_TO_CLASS( game_team_set, CGameTeamSet )

void CGameTeamSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( ShouldClearTeam() )
	{
		SUB_UseTargets( pActivator, USE_SET, -1 );
	}
	else
	{
		SUB_UseTargets( pActivator, USE_SET, 0 );
	}

	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGamePlayerZone / game_player_zone -- players in the zone fire my target when I'm fired
//
// Needs master?
class CGamePlayerZone : public CRuleBrushEntity
{
public:
	void		KeyValue( KeyValueData *pkvd );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	string_t	m_iszInTarget;
	string_t	m_iszOutTarget;
	string_t	m_iszInCount;
	string_t	m_iszOutCount;
};

LINK_ENTITY_TO_CLASS( game_zone_player, CGamePlayerZone )
TYPEDESCRIPTION	CGamePlayerZone::m_SaveData[] =
{
	DEFINE_FIELD( CGamePlayerZone, m_iszInTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszInCount, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutCount, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CGamePlayerZone, CRuleBrushEntity )

void CGamePlayerZone::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "intarget" ) )
	{
		m_iszInTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "outtarget" ) )
	{
		m_iszOutTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "incount" ) )
	{
		m_iszInCount = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "outcount" ) )
	{
		m_iszOutCount = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CRuleBrushEntity::KeyValue( pkvd );
}

void CGamePlayerZone::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int playersInCount = 0;
	int playersOutCount = 0;

	if( !CanFireForActivator( pActivator ) )
		return;

	CBaseEntity *pPlayer = NULL;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			TraceResult trace;
			int			hullNumber;

			hullNumber = human_hull;
			if( pPlayer->pev->flags & FL_DUCKING )
			{
				hullNumber = head_hull;
			}

			UTIL_TraceModel( pPlayer->pev->origin, pPlayer->pev->origin, hullNumber, edict(), &trace );

			if( trace.fStartSolid )
			{
				playersInCount++;
				if( m_iszInTarget )
				{
					FireTargets( STRING( m_iszInTarget ), pPlayer, pActivator, useType, value );
				}
			}
			else
			{
				playersOutCount++;
				if( m_iszOutTarget )
				{
					FireTargets( STRING( m_iszOutTarget ), pPlayer, pActivator, useType, value );
				}
			}
		}
	}

	if( m_iszInCount )
	{
		FireTargets( STRING( m_iszInCount ), pActivator, this, USE_SET, playersInCount );
	}

	if( m_iszOutCount )
	{
		FireTargets( STRING( m_iszOutCount ), pActivator, this, USE_SET, playersOutCount );
	}
}

//
// CGamePlayerHurt / game_player_hurt	-- Damages the player who fires it
// Flag: Fire once

#define SF_PKILL_FIREONCE			0x0001

class CGamePlayerHurt : public CRulePointEntity
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline bool RemoveOnFire( void ) { return (pev->spawnflags & SF_PKILL_FIREONCE); }

private:
};

LINK_ENTITY_TO_CLASS( game_player_hurt, CGamePlayerHurt )

void CGamePlayerHurt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( pActivator->IsPlayer() )
	{
		if( pev->dmg < 0 )
			pActivator->TakeHealth( this, -pev->dmg, DMG_GENERIC );
		else
			pActivator->TakeDamage( pev, pev, DamageInfo(pev->dmg, DMG_GENERIC) );
	}

	SUB_UseTargets( pActivator, useType, value );

	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGameCounter / game_counter	-- Counts events and fires target
// Flag: Fire once
// Flag: Reset on Fire

#define SF_GAMECOUNT_FIREONCE			0x0001
#define SF_GAMECOUNT_RESET				0x0002
#define SF_GAMECOUNT_FIREOVERLIMIT		0x0004

class CGameCounter : public CRulePointEntity
{
public:
	void		Spawn( void );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline bool RemoveOnFire( void ) { return (pev->spawnflags & SF_GAMECOUNT_FIREONCE); }
	inline bool ResetOnFire( void ) { return (pev->spawnflags & SF_GAMECOUNT_RESET); }

	inline void CountUp( void ) { pev->frags++; }
	inline void CountDown( void ) { pev->frags--; }
	inline void ResetCount( void ) { pev->frags = pev->dmg; }
	inline int CountValue( void ) { return (int)pev->frags; }
	inline int LimitValue( void ) { return (int)pev->health; }

	inline bool HitLimit( void ) {
		const int countValue = CountValue();
		const int limitValue = LimitValue();
		if (FBitSet(pev->spawnflags, SF_GAMECOUNT_FIREOVERLIMIT))
			return countValue >= limitValue;
		else
			return countValue == limitValue;
	}

	bool CalcRatio( CBaseEntity *pLocus, float* outResult );

private:

	inline void SetCountValue( int value ) { pev->frags = value; }
	inline void SetInitialValue( int value ) { pev->dmg = value; }
};

LINK_ENTITY_TO_CLASS( game_counter, CGameCounter )

void CGameCounter::Spawn( void )
{
	// Save off the initial count
	SetInitialValue( CountValue() );
	CRulePointEntity::Spawn();
}

void CGameCounter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	switch( useType )
	{
	case USE_ON:
	case USE_TOGGLE:
		CountUp();
		break;
	case USE_OFF:
		CountDown();
		break;
	case USE_SET:
		SetCountValue( (int)value );
		break;
	}

	if( HitLimit() )
	{
		SUB_UseTargets( pActivator );
		if( RemoveOnFire() )
		{
			UTIL_Remove( this );
		}

		if( ResetOnFire() )
		{
			ResetCount();
		}
	}
}

bool CGameCounter::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	*outResult = pev->frags;
	return true;
}

//
// CGameCounterSet / game_counter_set	-- Sets the counter's value
// Flag: Fire once

#define SF_GAMECOUNTSET_FIREONCE			0x0001

class CGameCounterSet : public CRulePointEntity
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline bool RemoveOnFire( void ) { return (pev->spawnflags & SF_GAMECOUNTSET_FIREONCE); }

private:
};

LINK_ENTITY_TO_CLASS( game_counter_set, CGameCounterSet )

void CGameCounterSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	SUB_UseTargets( pActivator, USE_SET, pev->frags );

	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGamePlayerEquip / game_playerequip	-- Sets the default player equipment
// Flag: USE Only

#define SF_PLAYEREQUIP_USEONLY			0x0001
#define MAX_EQUIP		32

class CGamePlayerEquip : public CRulePointEntity
{
public:
	void		KeyValue( KeyValueData *pkvd );
	void		Touch( CBaseEntity *pOther );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	inline bool	UseOnly( void ) { return (pev->spawnflags & SF_PLAYEREQUIP_USEONLY); }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	void		EquipPlayer( CBaseEntity *pPlayer );

	string_t	m_weaponNames[MAX_EQUIP];
	int			m_weaponCount[MAX_EQUIP];
};

LINK_ENTITY_TO_CLASS( game_player_equip, CGamePlayerEquip )

TYPEDESCRIPTION CGamePlayerEquip::m_SaveData[] =
{
	DEFINE_ARRAY( CGamePlayerEquip, m_weaponNames, FIELD_STRING, MAX_EQUIP ),
	DEFINE_ARRAY( CGamePlayerEquip, m_weaponCount, FIELD_INTEGER, MAX_EQUIP ),
};

IMPLEMENT_SAVERESTORE( CGamePlayerEquip, CRulePointEntity )

void CGamePlayerEquip::KeyValue( KeyValueData *pkvd )
{
	CRulePointEntity::KeyValue( pkvd );

	if( !pkvd->fHandled )
	{
		for( int i = 0; i < MAX_EQUIP; i++ )
		{
			if( !m_weaponNames[i] )
			{
				char tmp[128];

				UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ));

				m_weaponNames[i] = ALLOC_STRING( tmp );
				m_weaponCount[i] = atoi( pkvd->szValue );
				m_weaponCount[i] = Q_max( 1, m_weaponCount[i] );
				pkvd->fHandled = true;
				break;
			}
		}
	}
}

void CGamePlayerEquip::Touch( CBaseEntity *pOther )
{
	if( !CanFireForActivator( pOther ) )
		return;

	if( UseOnly() )
		return;

	EquipPlayer( pOther );
}

void CGamePlayerEquip::EquipPlayer( CBaseEntity *pEntity )
{
	CBasePlayer *pPlayer = NULL;

	if( pEntity->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pEntity;
	}

	if( !pPlayer )
		return;

	for( int i = 0; i < MAX_EQUIP; i++ )
	{
		if( !m_weaponNames[i] )
			break;

		if( g_pGameRules->IsBustingGame() && !strcmp( STRING( m_weaponNames[i] ), "weapon_egon" ))
			continue;

		for( int j = 0; j < m_weaponCount[i]; j++ )
		{
 			pPlayer->GiveNamedItem( STRING( m_weaponNames[i] ) );
		}
	}
}

void CGamePlayerEquip::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	EquipPlayer( pActivator );
}

//
// CGamePlayerTeam / game_player_team	-- Changes the team of the player who fired it
// Flag: Fire once
// Flag: Kill Player
// Flag: Gib Player

#define SF_PTEAM_FIREONCE			0x0001
#define SF_PTEAM_KILL    			0x0002
#define SF_PTEAM_GIB     			0x0004

class CGamePlayerTeam : public CRulePointEntity
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
	inline bool RemoveOnFire( void ) { return (pev->spawnflags & SF_PTEAM_FIREONCE); }
	inline bool ShouldKillPlayer( void ) { return (pev->spawnflags & SF_PTEAM_KILL); }
	inline bool ShouldGibPlayer( void ) { return (pev->spawnflags & SF_PTEAM_GIB); }
	
	const char *TargetTeamName( const char *pszTargetName );
};

LINK_ENTITY_TO_CLASS( game_player_team, CGamePlayerTeam )

const char *CGamePlayerTeam::TargetTeamName( const char *pszTargetName )
{
	CBaseEntity *pTeamEntity = NULL;

	while( ( pTeamEntity = UTIL_FindEntityByTargetname( pTeamEntity, pszTargetName ) ) != NULL )
	{
		if( FClassnameIs( pTeamEntity->pev, "game_team_master" ) )
			return pTeamEntity->TeamID();
	}

	return NULL;
}

void CGamePlayerTeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( pActivator->IsPlayer() )
	{
		const char *pszTargetTeam = TargetTeamName( STRING(pev->target) );
		if ( pszTargetTeam )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			g_pGameRules->ChangePlayerTeam( pPlayer, pszTargetTeam, ShouldKillPlayer(), ShouldGibPlayer() );
		}
	}
	
	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

/*
 * Starting player settings, like initial health, armor, weapons and items
 */
#define SF_PLAYER_SETTINGS_MEDKIT (1 << 0)
#define SF_PLAYER_SETTINGS_SUIT (1 << 1)
#define SF_PLAYER_SETTINGS_CROWBAR (1 << 2)
#define SF_PLAYER_SETTINGS_GLOCK (1 << 3)
#define SF_PLAYER_SETTINGS_PYTHON (1 << 4)
#define SF_PLAYER_SETTINGS_MP5 (1 << 5)
#define SF_PLAYER_SETTINGS_SHOTGUN (1 << 6)
#define SF_PLAYER_SETTINGS_CROSSBOW (1 << 7)
#define SF_PLAYER_SETTINGS_RPG (1 << 8)
#define SF_PLAYER_SETTINGS_GAUSS (1 << 9)
#define SF_PLAYER_SETTINGS_EGON (1 << 10)
#define SF_PLAYER_SETTINGS_HORNETGUN (1 << 11)
#define SF_PLAYER_SETTINGS_PIPEWRENCH (1 << 12)
#define SF_PLAYER_SETTINGS_KNIFE (1 << 13)
#define SF_PLAYER_SETTINGS_GRAPPLE (1 << 14)
#define SF_PLAYER_SETTINGS_DESERT_EAGLE (1 << 15)
#define SF_PLAYER_SETTINGS_SNIPERRIFLE (1 << 16)
#define SF_PLAYER_SETTINGS_M249 (1 << 17)
#define SF_PLAYER_SETTINGS_DISPACER (1 << 18)
#define SF_PLAYER_SETTINGS_SHOCKRIFLE (1 << 19)
#define SF_PLAYER_SETTINGS_SPORELAUNCHER (1 << 20)
#define SF_PLAYER_SETTINGS_LONGJUMP (1 << 23)

enum
{
	VALUE_SETTING_DEFAULT = 0,
	VALUE_SETTING_SET,
	VALUE_SETTING_ADD,
	VALUE_SETTING_SUBSTRUCT,
	VALUE_SETTING_ATMAX,
	VALUE_SETTING_ATLEAST
};

class CGamePlayerSettings : public CRulePointEntity
{
public:
	enum {
		SUIT_LIGHT_NOTHING = -1,
		SUIT_LIGHT_DEFAULT = 0,
		SUIT_LIGHT_FLASHLIGHT = 1,
		SUIT_LIGHT_NVG = 2,
	};

	enum
	{
		WEAPON_SETTING_REMOVE = -1,
		WEAPON_SETTING_NOCHANGE = 0,
		WEAPON_SETTING_GIVE = 1,
		WEAPON_SETTING_GIVE_ALLOW_DUP = 2,
	};

	void PreEntvarsKeyvalue( KeyValueData* pkvd );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		EquipPlayer(g_pGameRules->EffectiveAlivePlayer(pActivator));
	}
	void Touch( CBaseEntity *pOther )
	{
		EquipPlayer( pOther );
	}

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	void EquipPlayer( CBaseEntity *pPlayer );
	enum
	{
		SuitLongLogon,
		SuitShortLogon,
		SuitNoLogon
	};
	int m_ammoCounts[MAX_AMMO_TYPES];
	string_t m_weapons[MAX_WEAPONS];
	short m_weaponSettings[MAX_WEAPONS];
	short m_suit;
	short m_suitLogon;
	short m_suitLight;
	short m_longjump;

	bool m_allowOverheal;
	bool m_allowOvercharge;
	float m_armorStrength;

	short m_healthSetting;
	short m_armorSetting;
};

LINK_ENTITY_TO_CLASS( game_player_settings, CGamePlayerSettings )

TYPEDESCRIPTION	CGamePlayerSettings::m_SaveData[] =
{
	DEFINE_ARRAY( CGamePlayerSettings, m_ammoCounts, FIELD_INTEGER, MAX_AMMO_TYPES ),
	DEFINE_ARRAY( CGamePlayerSettings, m_weapons, FIELD_STRING, MAX_WEAPONS ),
	DEFINE_ARRAY( CGamePlayerSettings, m_weaponSettings, FIELD_SHORT, MAX_WEAPONS ),
	DEFINE_FIELD( CGamePlayerSettings, m_suit, FIELD_SHORT ),
	DEFINE_FIELD( CGamePlayerSettings, m_suitLogon, FIELD_SHORT ),
	DEFINE_FIELD( CGamePlayerSettings, m_suitLight, FIELD_SHORT ),
	DEFINE_FIELD( CGamePlayerSettings, m_longjump, FIELD_SHORT ),
	DEFINE_FIELD( CGamePlayerSettings, m_allowOverheal, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGamePlayerSettings, m_allowOvercharge, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGamePlayerSettings, m_armorStrength, FIELD_FLOAT ),
	DEFINE_FIELD( CGamePlayerSettings, m_healthSetting, FIELD_SHORT ),
	DEFINE_FIELD( CGamePlayerSettings, m_armorSetting, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CGamePlayerSettings, CRulePointEntity )

static float ParseValueAndSetting(const char* value, short& setting)
{
	if (*value == '=')
	{
		setting = VALUE_SETTING_SET;
		++value;
	}
	else if (*value == '+')
	{
		setting = VALUE_SETTING_ADD;
		++value;
	}
	else if (*value == '-')
	{
		setting = VALUE_SETTING_SUBSTRUCT;
		++value;
	}
	else if (*value == '<')
	{
		setting = VALUE_SETTING_ATMAX;
		++value;
	}
	else if (*value == '>')
	{
		setting = VALUE_SETTING_ATLEAST;
		++value;
	}
	else
	{
		setting = VALUE_SETTING_DEFAULT;
	}
	return atof(value);
}

void CGamePlayerSettings::PreEntvarsKeyvalue( KeyValueData* pkvd )
{
	if (FStrEq(pkvd->szKeyName, "health"))
	{
		pev->health = ParseValueAndSetting(pkvd->szValue, m_healthSetting);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "armorvalue"))
	{
		pev->armorvalue = ParseValueAndSetting(pkvd->szValue, m_armorSetting);
		pkvd->fHandled = true;
	}
	else
		CRulePointEntity::PreEntvarsKeyvalue(pkvd);
}

void CGamePlayerSettings::KeyValue(KeyValueData *pkvd)
{
	const char* ammoName = pkvd->szKeyName;
	if (*ammoName == '_')
		ammoName++;
	else
		ammoName = FixedAmmoName(ammoName);

	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(ammoName);
	if (ammoType)
	{
		m_ammoCounts[ammoType->id] = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "suit"))
	{
		m_suit = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "suitlogon"))
	{
		m_suitLogon = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "suitlight"))
	{
		m_suitLight = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "longjump"))
	{
		m_longjump = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "max_armor"))
	{
		pev->armortype = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "allow_overheal"))
	{
		m_allowOverheal = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "allow_overcharge"))
	{
		m_allowOvercharge = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "armor_strength"))
	{
		m_armorStrength = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (strncmp(pkvd->szKeyName, "weapon_", 7) == 0)
	{
		for (size_t i=0; i<ARRAYSIZE(m_weapons); ++i)
		{
			if (FStringNull(m_weapons[i]))
			{
				m_weapons[i] = ALLOC_STRING(pkvd->szKeyName);
				m_weaponSettings[i] = atoi(pkvd->szValue);
				break;
			}
			if (FStrEq(pkvd->szKeyName, STRING(m_weapons[i])))
			{
				m_weaponSettings[i] = atoi(pkvd->szValue);
				break;
			}
		}
		pkvd->fHandled = true;
	}
	else
		CRulePointEntity::KeyValue(pkvd);
}

void CGamePlayerSettings::EquipPlayer(CBaseEntity *pPlayer)
{
	if (!pPlayer || !pPlayer->IsPlayer())
		return;

	CBasePlayer* player = (CBasePlayer*)pPlayer;

	if (pev->max_health > 0)
	{
		player->SetMaxHealth(pev->max_health, !m_allowOverheal);
	}
	if (pev->armortype > 0)
	{
		player->SetMaxArmor(pev->armortype, !m_allowOvercharge);
	}

	switch (m_healthSetting) {
	case VALUE_SETTING_SET:
		player->SetHealth(pev->health, m_allowOverheal);
		break;
	case VALUE_SETTING_ADD:
		player->TakeHealth(this, (int)pev->health, m_allowOverheal ? HEAL_ALLOW_OVERFLOW : HEAL_GENERIC);
		break;
	case VALUE_SETTING_SUBSTRUCT:
		player->TakeDamage(pev, pev, DamageInfo(pev->health, DMG_GENERIC).SetIgnoreArmor().SetNoPlayerPush().SetNoPunch());
		break;
	case VALUE_SETTING_ATMAX:
		if (player->pev->health > pev->health)
		{
			player->SetHealth((int)pev->health, m_allowOverheal);
		}
		break;
	case VALUE_SETTING_ATLEAST:
		if (player->pev->health < pev->health)
		{
			player->SetHealth((int)pev->health, m_allowOverheal);
		}
		break;
	default:
		if (pev->health > 0)
		{
			player->SetHealth((int)pev->health, m_allowOverheal);
		}
		break;
	}

	switch (m_armorSetting) {
	case VALUE_SETTING_SET:
		player->SetArmor(pev->armorvalue, m_allowOvercharge);
		break;
	case VALUE_SETTING_ADD:
		player->TakeArmor(this, pev->armorvalue, m_allowOvercharge ? GIVEARMOR_ALLOW_OVERFLOW : GIVEARMOR_GENERIC);
		break;
	case VALUE_SETTING_SUBSTRUCT:
		player->TakeArmor(this, -pev->armorvalue, m_allowOvercharge ? GIVEARMOR_ALLOW_OVERFLOW : GIVEARMOR_GENERIC);
		break;
	case VALUE_SETTING_ATMAX:
		if (player->pev->armorvalue > pev->armorvalue)
		{
			player->SetArmor(pev->armorvalue, m_allowOvercharge);
		}
		break;
	case VALUE_SETTING_ATLEAST:
		if (player->pev->armorvalue < pev->armorvalue)
		{
			player->SetArmor(pev->armorvalue, m_allowOvercharge);
		}
		break;
	default:
		if (pev->armorvalue > 0)
		{
			player->SetArmor(pev->armorvalue, m_allowOvercharge);
		}
		break;
	}

	if (m_armorStrength > 0)
	{
		player->m_armorStrength = m_armorStrength;
	}

	short giveSuit = m_suit;
	if (!giveSuit && FBitSet(pev->spawnflags, SF_PLAYER_SETTINGS_SUIT))
	{
		giveSuit = 1;
	}
	if (giveSuit > 0)
	{
		if (!player->HasSuit())
		{
			int suitSpawnFlags = SF_ITEM_NOFALL;
			switch (m_suitLogon) {
			case 1:
				suitSpawnFlags |= SF_SUIT_SHORTLOGON;
				break;
			case 2:
				suitSpawnFlags |= SF_SUIT_NOLOGON;
				break;
			default:
				break;
			}
			player->GiveNamedItem("item_suit", suitSpawnFlags);
		}
	}
	else if (giveSuit < 0)
	{
		player->m_iItemsBits &= ~(PLAYER_ITEM_SUIT);
	}

	short giveLongjump = m_longjump;
	if (!giveLongjump && FBitSet(pev->spawnflags, SF_PLAYER_SETTINGS_LONGJUMP))
	{
		giveLongjump = 1;
	}
	if (giveLongjump > 0)
	{
		if (player->HasSuit() && !player->m_fLongJump)
		{
			player->GiveNamedItem("item_longjump", SF_ITEM_NOFALL);
		}
	}
	else if (giveLongjump < 0)
	{
		player->SetLongjump(false);
	}

	switch (m_suitLight) {
	case SUIT_LIGHT_NOTHING:
		player->RemoveSuitLight();
		break;
	case SUIT_LIGHT_FLASHLIGHT:
		player->SetFlashlightOnly();
		break;
	case SUIT_LIGHT_NVG:
		player->SetNVGOnly();
		break;
	}

	// Check this before giving ammo as player can get exhaustible weapon along with ammo.
	const bool hadWeapons = player->m_pActiveItem != NULL;

	size_t i;
	for (i=1; i<MAX_AMMO_TYPES; ++i)
	{
		const AmmoType* ammoInfo = g_AmmoRegistry.GetByIndex(i);
		if (ammoInfo)
		{
			if (m_ammoCounts[i] > 0)
				player->GiveAmmo(m_ammoCounts[i], ammoInfo->name);
			else if (m_ammoCounts[i] < 0)
				player->RemoveAmmo(-m_ammoCounts[i], ammoInfo->name);
		}
	}

	for (i=0; i<ARRAYSIZE(m_weapons); ++i)
	{
		if (FStringNull(m_weapons[i]))
			break;
		string_t weaponName = m_weapons[i];
		const int weaponSetting = m_weaponSettings[i];
		if (weaponSetting == WEAPON_SETTING_REMOVE)
		{
			CBasePlayerWeapon* pWeapon = player->GetWeaponByName(STRING(weaponName));
			if (pWeapon)
			{
				pWeapon->DestroyItem();
			}
		}
		else if (weaponSetting == WEAPON_SETTING_GIVE)
		{
			if (!player->HasNamedPlayerItem(STRING(weaponName)))
			{
				player->GiveNamedItem(STRING(weaponName));
			}
		}
		else if (weaponSetting == WEAPON_SETTING_GIVE_ALLOW_DUP)
		{
			player->GiveNamedItem(STRING(weaponName));
		}
	}

	if (!hadWeapons || player->m_pActiveItem == NULL)
		player->SwitchToBestWeapon();

	if (player->m_pActiveItem == NULL)
	{
		player->SendCurWeaponClear();
	}

	SUB_UseTargets(pPlayer);
}


class CGameAutosave : public CPointEntity
{
public:
	void Spawn();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

void CGameAutosave::Spawn()
{
	if( g_pGameRules->IsMultiplayer() )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}
	CPointEntity::Spawn();
}

void CGameAutosave::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->health > 0) {
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (pPlayer != 0 && pev->health > pPlayer->pev->health)
			return;
	}

	SERVER_COMMAND( "autosave\n" );
}

LINK_ENTITY_TO_CLASS( game_autosave, CGameAutosave )

class CGameNumber : public CPointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	bool CalcRatio( CBaseEntity *pLocus, float* outResult )
	{
		*outResult = pev->frags;
		return true;
	}
};

void CGameNumber::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	switch( useType )
	{
	case USE_ON:
	case USE_TOGGLE:
		pev->frags++;
		break;
	case USE_OFF:
		pev->frags--;
		break;
	case USE_SET:
		pev->frags = value;
		break;
	}
}

LINK_ENTITY_TO_CLASS( game_number, CGameNumber )

extern int gmsgJournal;

#define SF_GAMEJOURNAL_REPEATABLE 1
#define SF_GAMEJOURNAL_DONT_NOTIFY 2

class CGameJournal : public CPointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);

		if (pPlayer)
		{
			string_t section = MySection();
			const bool shouldNotify = !FBitSet(pev->spawnflags, SF_GAMEJOURNAL_DONT_NOTIFY);
			if (pPlayer->AddJournalRecord(section, pev->message))
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgJournal, NULL, pPlayer->pev );
				WRITE_BYTE(shouldNotify);
				WRITE_STRING(STRING(section));
				WRITE_STRING(STRING(pev->message));
				MESSAGE_END();
			}
		}
		if (!FBitSet(pev->spawnflags, SF_GAMEJOURNAL_REPEATABLE))
			UTIL_Remove(this);
	}
	virtual string_t MySection() {
		return pev->netname;
	}
};

LINK_ENTITY_TO_CLASS( game_journal, CGameJournal )
