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

/*
==========================
This file contains "stubs" of class member implementations so that we can predict certain
 weapons client side.  From time to time you might find that you need to implement part of the
 these functions.  If so, cut it from here, paste it in hl_weapons.cpp or somewhere else and
 add in the functionality you need.
==========================
*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"nodes.h"
#include	"skill.h"
#include	"event_api.h"

// Globals used by game logic
const Vector g_vecZero = Vector( 0, 0, 0 );
int gmsgWeapPickup = 0;
enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;

ItemInfo CBasePlayerWeapon::ItemInfoArray[MAX_WEAPONS];

bool EMIT_SOUND_DYN( edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch ) { return true; }
int PRECACHE_MODEL(const char* name) {return 0;}
int PRECACHE_SOUND(const char* name) {return 0;}
void SET_MODEL(edict_t *e, const char *m) {}

// CBaseEntity Stubs
int CBaseEntity::TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType ) { return 1; }
TakeDamageResult CBaseEntity::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) { return TakeDamageResult(); }
CBaseEntity *CBaseEntity::GetNextTarget() { return NULL; }
void CBaseEntity::KeyValue( KeyValueData* pkvd ) { pkvd->fHandled = false; }
int CBaseEntity::Save( CSave &save ) { return 1; }
int CBaseEntity::Restore( CRestore &restore ) { return 1; }
void CBaseEntity::Activate() {}
void CBaseEntity::SetObjectCollisionBox() { }
void CBaseEntity::SetMyObjectCollisionBox(const Vector& defaultMins, const Vector& defaultMaxs) { }
bool CBaseEntity::IsInWorld() { return true; }
int CBaseEntity::DamageDecal( int bitsDamageType ) { return -1; }
void CBaseEntity::UpdateOnRemove() { }
int CBaseEntity::IRelationship( CBaseEntity *pTarget ) { return 0; }
void CBaseEntity::SetMyModel(const char* defaultModel) {}
int CBaseEntity::PRECACHE_SOUND(const char *soundName) { return 0; }
float CBaseEntity::GetSkillValue(const char* name) { return 0.0f; }

// CBaseDelay Stubs
void CBaseDelay::KeyValue( struct KeyValueData_s * ) { }
int CBaseDelay::Restore( class CRestore & ) { return 1; }
int CBaseDelay::Save( class CSave & ) { return 1; }

// CBaseAnimating Stubs
int CBaseAnimating::Restore( class CRestore & ) { return 1; }
int CBaseAnimating::Save( class CSave & ) { return 1; }

// DEBUG Stubs
edict_t *DBG_EntOfVars( const entvars_t *pev ) { return NULL; }
//void DBG_AssertFunction( bool fExpr, const char *szExpr, const char *szFile, int szLine, const char *szMessage) { }

// UTIL_* Stubs
bool UTIL_PrecacheOther( const char *szClassname, EntityOverrides entityOverrides ) { return false; }
void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount ) { }
void UTIL_DecalTrace( TraceResult *pTrace, int decalNumber ) { }
void UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber ) { }
void UTIL_MakeVectors( const Vector &vecAngles )
{
	gEngfuncs.pfnAngleVectors( vecAngles, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up );
}
void UTIL_SetOrigin( entvars_t *, const Vector &org ) { }
void UTIL_LogPrintf(char *,...) { }
void UTIL_ClientPrintAll( int,char const *,char const *,char const *,char const *,char const *) { }
void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 ) { }

// CBaseToggle Stubs
int CBaseToggle::Restore( class CRestore & ) { return 1; }
int CBaseToggle::Save( class CSave & ) { return 1; }
void CBaseToggle::KeyValue( struct KeyValueData_s * ) { }

void UTIL_Remove( CBaseEntity *pEntity ){ }
void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax ){ }
CBaseEntity *UTIL_FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius ){ return 0;}

void CBaseMonster::BarnacleVictimBitten( entvars_t *pevBarnacle ) { }
void CBaseMonster::BarnacleVictimReleased() { }
bool CBaseMonster::FValidateHintType( short sHint ) { return false; }
void CBaseMonster::Look( int iDistance ) { }
int CBaseMonster::DefaultISoundMask() { return 0; }
CSound *CBaseMonster::PBestSound() { return NULL; }
CSound *CBaseMonster::PBestScent() { return NULL; } 
float CBaseAnimating::StudioFrameAdvance( float flInterval ) { return 0.0; }
void CBaseMonster::MonsterThink() { }
int CBaseMonster::IgnoreConditions() { return 0; }
bool CBaseMonster::FBecomeProne() { return true; }
bool CBaseMonster::CheckRangeAttack1( float flDot, float flDist ) { return false; }
bool CBaseMonster::CheckRangeAttack2( float flDot, float flDist ) { return false; }
bool CBaseMonster::CheckMeleeAttack1( float flDot, float flDist ) { return false; }
bool CBaseMonster::CheckMeleeAttack2( float flDot, float flDist ) { return false; }
bool CBaseMonster::WantsToGetCloseToEnemy() { return false; }
bool CBaseMonster::FCanCheckAttacks() { return false; }
bool CBaseMonster::CheckEnemy( CBaseEntity *pEnemy ) { return false; }
void CBaseMonster::SetActivity( Activity NewActivity ) { }
int CBaseMonster::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist ) { return 0; }
void CBaseMonster::Move( float flInterval ) { }
bool CBaseMonster::ShouldAdvanceRoute( float flWaypointDist ) { return false; }
void CBaseMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval ) { }
void CBaseMonster::MonsterInit() { }
void CBaseMonster::MonsterInitThink() { }
void CBaseMonster::StartMonster() { }
int CBaseMonster::IRelationship( CBaseEntity *pTarget ) { return 0; }
bool CBaseMonster::BuildNearestRoute( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist ) { return false; }
CBaseEntity *CBaseMonster::BestVisibleEnemy() { return NULL; }
bool CBaseMonster::FInViewCone( CBaseEntity *pEntity ) { return false; }
bool CBaseMonster::FInViewCone( Vector *pOrigin ) { return false; }
bool CBaseEntity::FVisible( CBaseEntity *pEntity, CBaseEntity** ppSightBlocker ) { return false; }
bool CBaseEntity::FVisible( const Vector &vecOrigin, CBaseEntity** ppSightBlocker ) { return false; }
float CBaseMonster::ChangeYaw( int yawSpeed ) { return 0; }
int CBaseAnimating::LookupActivity( int activity ) { return 0; }
void CBaseMonster::HandleAnimEvent( MonsterEvent_t *pEvent ) { }
Vector CBaseMonster::GetGunPosition() { return g_vecZero; }
void CBaseEntity::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) { }
void CBaseEntity::FireBullets( unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, float flDamage, int iTracerFreq, entvars_t *pevAttacker ) { }
void CBaseEntity::TraceBleed( float flDamage, Vector vecDir, const TraceResult *ptr, int bitsDamageType ) { }
void CBaseMonster::ReportAIState( ALERT_TYPE ) { }
void CBaseMonster::KeyValue( KeyValueData *pkvd ) { }
void CBaseMonster::Activate() {}
void CBaseMonster::LaunchAsProjectile(const ProjectileParameters&) {}
void CBaseMonster::DropAsAmmoEnt(int amount) {}
bool CBaseMonster::CanPlaySequence( int interruptFlags ) { return false; }
bool CBaseMonster::FCanActiveIdle() { return false; }
bool CBaseToggle::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation, bool subtitle ) { return true; }
void CBaseToggle::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener ) { }
void CBaseToggle::SentenceStop() { }
bool CBaseToggle::IsLockedByMaster() { return false; }
void CBaseMonster::MonsterInitDead() { }
float CBaseMonster::HeadHitGroupDamageMultiplier() { return 3.0f; }
void CBaseMonster::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr) { }
bool CBaseMonster::IsFullyAlive() { return CBaseToggle::IsFullyAlive(); }
bool CBaseMonster::ShouldFadeOnDeath() { return false; }
bool CBaseMonster::ShouldCollide(CBaseEntity *pOther) { return true; }
bool CBaseMonster::ShouldCollideWithCorpses() { return true; }
void CBaseMonster::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, int iClassIgnore ) { }
void CBaseMonster::RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, int iClassIgnore ) { }
void CBaseMonster::FadeMonster() { }
void CBaseMonster::GibMonster() { }
bool CBaseMonster::HasHumanGibs() { return false; }
bool CBaseMonster::HasAlienGibs() { return false; }
Activity CBaseMonster::GetDeathActivity() { return ACT_DIE_HEADSHOT; }
MONSTERSTATE CBaseMonster::GetIdealState() { return MONSTERSTATE_ALERT; }
Schedule_t* CBaseMonster::GetScheduleOfType( int Type ) { return NULL; }
Schedule_t *CBaseMonster::GetSchedule() { return NULL; }
void CBaseMonster::RunTask( Task_t *pTask ) { }
void CBaseMonster::StartTask( Task_t *pTask ) { }
Schedule_t *CBaseMonster::ScheduleFromName( const char *pName ) { return NULL;}
void CBaseMonster::BecomeDead() {}
void CBaseMonster::RunAI() {}
KilledResult CBaseMonster::Killed( entvars_t * pevInflictor, entvars_t *pevAttacker, int iGib ) { return KilledResult(); }
void CBaseMonster::OnDying(bool gibbed) {}
void CBaseMonster::UpdateOnRemove() {}
int CBaseMonster::TakeHealth(CBaseEntity* pHealer, float flHealth, int bitsDamageType) { return 0; }
TakeDamageResult CBaseMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) { return TakeDamageResult(); }
int CBaseMonster::Restore( class CRestore & ) { return 1; }
int CBaseMonster::Save( class CSave & ) { return 1; }
int CBaseMonster::DefaultClassify() { return 0; }
int CBaseMonster::Classify() { return 0; }
const char* CBaseMonster::DefaultGibModel() {return 0;}
int CBaseMonster::DefaultGibCount() {return 0;}
bool CBaseMonster::IsAlienMonster() {return false;}
Vector CBaseMonster::DefaultMinHullSize() {return Vector(0,0,0); }
Vector CBaseMonster::DefaultMaxHullSize() {return Vector(0,0,0); }
int CBaseMonster::SizeForGrapple() { return GRAPPLE_NOT_A_TARGET; }
bool CBaseMonster::HandleBlocker(CBaseEntity* pBlocker, bool duringMovement) { return false; }
bool CBaseMonster::CanBeMadeMoveAway(CBaseEntity* pPusher) { return false; }
bool CBaseMonster::HandleDoorBlockage(CBaseEntity* pDoor) { return false; }
void CBaseMonster::AskMoveAwayFromSpot(CBaseEntity* pSpotEntity, float minDist, bool run) {}


void CBasePlayer::DeathSound() { }
int CBasePlayer::TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType ) { return 0; }
bool CBasePlayer::TakeArmor(CBaseEntity *pCharger, float flArmor, int flags) { return false; }
void CBasePlayer::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr) { }
TakeDamageResult CBasePlayer::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) { return TakeDamageResult(); }
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim ) { }
void CBasePlayer::WaterMove() { }
bool CBasePlayer::IsOnLadder() { return false; }
void CBasePlayer::PlayerDeathThink() { }
void CBasePlayer::StartDeathCam() { }
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle ) { }
void CBasePlayer::PlayerUse() { }
void CBasePlayer::Jump() { }
void CBasePlayer::Duck() { }
int  CBasePlayer::DefaultClassify() { return 0; }
int  CBasePlayer::Classify() { return 0; }
void CBasePlayer::PreThink() { }
void CBasePlayer::CheckTimeBasedDamage()  { }
void CBasePlayer::CheckSuitUpdate() { }
void CBasePlayer::SetSuitUpdate( const char *name, bool fgroup, int iNoRepeatTime ) { }
void CBasePlayer::PostThink() { }
void CBasePlayer::Precache() { }
int CBasePlayer::Save( CSave &save ) { return 0; }
int CBasePlayer::Restore( CRestore &restore ) { return 0; }
void CBasePlayer::ImpulseCommands() { }
int CBasePlayer::AddPlayerItem( CBasePlayerWeapon *pItem ) { return DID_NOT_GET_ITEM; }
int CBasePlayer::GetAmmoIndex( const char *psz ) { return 0; }
void CBasePlayer::UpdateClientData() { }
bool CBasePlayer::FBecomeProne() { return true; }
void CBasePlayer::BarnacleVictimBitten( entvars_t *pevBarnacle ) { }
void CBasePlayer::BarnacleVictimReleased() { }
int CBasePlayer::Illumination() { return 0; }
Vector CBasePlayer::GetAutoaimVector( float flDelta ) { return g_vecZero; }
Vector CBasePlayer::GetAutoaimVectorFromPoint( const Vector& vecSrc, float flDelta ) { return g_vecZero; }
Vector CBasePlayer::AutoaimDeflection( const Vector &vecSrc, float flDist, float flDelta  ) { return g_vecZero; }
void CBasePlayer::ResetAutoaim() { }
Vector CBasePlayer::GetGunPosition()
{
	Vector origin = pev->origin;
	Vector view_ofs;

	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);

	return origin + view_ofs;
}
const char *CBasePlayer::TeamID() { return ""; }
int CBasePlayer::GiveAmmo( int iCount, const char *szName ) { return 0; }
void CBasePlayer::AddPoints( int score, bool bAllowNegativeScore ) { }
void CBasePlayer::AddFloatPoints(float score, bool bAllowNegativeScore) {}
void CBasePlayer::AddPointsToTeam( int score, bool bAllowNegativeScore ) { }
bool CBasePlayer::HandleDoorBlockage(CBaseEntity *pDoor) { return false; }
bool CBasePlayer::ShouldCollideWithCorpses() { return false; }

float CBasePlayerWeapon::GetNextAttackDelay( float flTime ) { return flTime; }
void CBasePlayerWeapon::SetObjectCollisionBox() { }
void CBasePlayerWeapon::KeyValue( KeyValueData *pkvd ) {}
void CBasePlayerWeapon::FallInit() { }
CBaseEntity *CBasePlayerWeapon::Respawn() { return nullptr; }
bool CBasePlayerWeapon::IsLockedByMaster() { return false; }
bool CBasePlayerWeapon::IsUsefulToDisplayHint(CBaseEntity* pPlayer) { return false; }
void CBasePlayerWeapon::DropAsAmmoEnt(int amount) {}
void CBasePlayerWeapon::DefaultTouch( CBaseEntity *pOther ) { }
void CBasePlayerWeapon::DestroyItem() { }
bool CBasePlayerWeapon::IsEnabledInMod() { return true; }
bool CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer ) { return true; }
void CBasePlayerWeapon::Drop() { }
void CBasePlayerWeapon::Kill() { }
void CBasePlayerWeapon::AttachToPlayer ( CBasePlayer *pPlayer ) { }
void CBasePlayerWeapon::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {}
int CBasePlayerWeapon::ObjectCaps() {return 0;}
int CBasePlayerWeapon::AddDuplicate( CBasePlayerWeapon *pOriginal ) { return 0; }
bool CBasePlayerWeapon::AddToPlayerDefault( CBasePlayer *pPlayer ) { return false; }
int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer ) { return 0; }
bool CBasePlayerWeapon::IsUseable() { return true; }
bool CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon ) { return false; }
bool CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon ) { return false; }
void CBasePlayerWeapon::RetireWeapon() { }

bool CConfigurableWeapon::IsUseable() { return true; }
bool CConfigurableWeapon::AddToPlayer(CBasePlayer *pPlayer) { return true; }
