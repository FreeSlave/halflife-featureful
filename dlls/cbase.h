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
#pragma once
#if !defined(CBASE_H)
#define CBASE_H

#include "extdll.h"
#include "util.h"
#include "soundscripts.h"
#include "visuals.h"
#include "grapple_target.h"
#include "classify.h"
#include <type_traits>
/*

Class Hierachy

CBaseEntity
	CBaseDelay
		CBaseToggle
			CBaseItem
			CBaseMonster
				CBaseCycler
				CBasePlayer
				CBaseGroup
*/
#define		MAX_PATH_SIZE	10 // max number of nodes available for a path.

// These are caps bits to indicate what an object's capabilities (currently used for save/restore and level transitions)
#define		FCAP_CUSTOMSAVE				0x00000001
#define		FCAP_ACROSS_TRANSITION			0x00000002		// should transfer between transitions
#define		FCAP_MUST_SPAWN				0x00000004		// Spawn after restore
#define		FCAP_DONT_SAVE				0x80000000		// Don't save this
#define		FCAP_IMPULSE_USE			0x00000008		// can be used by the player
#define		FCAP_CONTINUOUS_USE			0x00000010		// can be used by the player
#define		FCAP_ONOFF_USE				0x00000020		// can be used by the player
#define		FCAP_DIRECTIONAL_USE			0x00000040		// Player sends +/- 1 when using (currently only tracktrains)
#define		FCAP_MASTER				0x00000080		// Can be used to "master" other entities (like multisource)

#define		FCAP_ONLYDIRECT_USE			0x00000100 // Only direct use, from sohl
#define		FCAP_ONLYVISIBLE_USE		0x00000200 // Don't use through walls

// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define		FCAP_FORCE_TRANSITION		0x00000080		// ALWAYS goes across transitions

enum
{
	PLAYER_USE_POLICY_DEFAULT = 0,
	PLAYER_USE_POLICY_DIRECT,
	PLAYER_USE_POLICY_VISIBLE,
};

#include "saverestore.h"
#include "schedule.h"

#if !defined(MONSTEREVENT_H)
#include "monsterevent.h"
#endif

// C functions for external declarations that call the appropriate C++ methods

#include "exportdef.h"

extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion );
extern "C" EXPORT int GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion );
extern "C" EXPORT int GetNewDLLFunctions( NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion );

// TODO: replace this by actual definitions from physint.h
typedef void *server_physics_api_t;
typedef void *physics_interface_t;
extern "C" EXPORT int Server_GetPhysicsInterface( int version, server_physics_api_t *api, physics_interface_t *interface );
extern bool g_fIsXash3D;

extern int DispatchSpawn( edict_t *pent );
extern void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd );
extern void DispatchTouch( edict_t *pentTouched, edict_t *pentOther );
extern void DispatchUse( edict_t *pentUsed, edict_t *pentOther );
extern void DispatchThink( edict_t *pent );
extern void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther );
extern void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData );
extern int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity );
extern void DispatchObjectCollsionBox( edict_t *pent );
extern void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
extern void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
extern void SaveGlobalState( SAVERESTOREDATA *pSaveData );
extern void RestoreGlobalState( SAVERESTOREDATA *pSaveData );
extern void ResetGlobalState( void );

typedef enum
{
	USE_OFF = 0,
	USE_ON = 1,
	USE_SET = 2,
	USE_TOGGLE = 3
} USE_TYPE;

const char* UseTypeToString(USE_TYPE useType);
extern void FireTargets( const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType = USE_TOGGLE, float value = 0.0f );
extern void KillTargets( const char *targetName );

typedef void(CBaseEntity::*BASEPTR)( void );
typedef void(CBaseEntity::*ENTITYFUNCPTR)( CBaseEntity *pOther );
typedef void(CBaseEntity::*USEPTR)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

class CBaseEntity;
class CBaseToggle;
class CBaseMonster;
class CBasePlayerWeapon;
class CBasePlayerAmmo;
class CSquadMonster;

struct EntTemplate;

#define SF_ITEM_TOUCH_ONLY 128
#define SF_ITEM_USE_ONLY 256 //  ITEM_USE_ONLY = BUTTON_USE_ONLY = DOOR_USE_ONLY!!!

#define SF_ITEM_NOFALL ( 1 << 28 )
#define SF_ITEM_FIX_PHYSICS ( 1 << 29 )
#define	SF_NORESPAWN	( 1 << 30 )// !!!set this bit on guns and stuff that should never respawn.

enum
{
	DID_NOT_GET_ITEM,
	GOT_NEW_ITEM,
	GOT_DUP_ITEM
};

//
// EHANDLE. Safe way to point to CBaseEntities who may die between frames
//
class EHANDLE
{
private:
	edict_t *m_pent;
	int m_serialnumber;
public:
	edict_t *Get( void );
	edict_t *Set( edict_t *pent );

	operator int ();

	operator CBaseEntity *();

	CBaseEntity *operator = ( CBaseEntity *pEntity );
	CBaseEntity *operator ->();

	template <typename T>
	T* Entity()
	{
		return static_cast<T*>(operator CBaseEntity*());
	}
};

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity 
{
public:
	// Constructor.  Set engine to use C/C++ callback functions
	// pointers to engine data
	entvars_t *pev;		// Don't need to save/restore this pointer, the engine resets it

	// path corners
	CBaseEntity *m_pGoalEnt;// path corner we are heading towards
	CBaseEntity *m_pLink;// used for temporary link-list operations. 

	byte m_EFlags;

	virtual bool	CalcPosition( CBaseEntity *pLocus, Vector* outResult )	{ *outResult = pev->origin; return true; }
	virtual bool	CalcVelocity( CBaseEntity *pLocus, Vector* outResult )	{ *outResult = pev->velocity; return true; }
	virtual bool	CalcRatio( CBaseEntity *pLocus, float* outResult )	{ *outResult = 0; return true; }

	// initialization functions
	virtual void Spawn( void ) { return; }
	virtual void Precache( void ) { return; }
	virtual bool IsEnabledInMod() { return true; }
	virtual void PreEntvarsKeyvalue( KeyValueData* pkvd ) { pkvd->fHandled = false; }
	virtual void KeyValue( KeyValueData* pkvd );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	virtual int ObjectCaps( void ) { return FCAP_ACROSS_TRANSITION; }
	virtual void Activate( void );

	// Setup the object->object collision box (pev->mins / pev->maxs is the object->world collision box)
	virtual void SetObjectCollisionBox( void );

	// Classify - returns the type of group (i.e, "houndeye", or "human military" so that monsters with different classnames
	// still realize that they are teammates. (overridden for monsters that form groups)
	virtual int Classify( void ) { return DefaultClassify(); }
	virtual int DefaultClassify() { return CLASS_NONE; }
	virtual void DeathNotice( entvars_t *pevChild ) {}// monster maker children use this to tell the monster maker that they have died.

	static TYPEDESCRIPTION m_SaveData[];

	virtual void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void ApplyTraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType );
	virtual int TakeArmor( CBaseEntity* pCharger, float flArmor, int flags = 0 ) { return 0; }
	virtual void Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib );
	virtual int BloodColor( void ) { return DONT_BLEED; }
	virtual void TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	virtual bool IsTriggered( CBaseEntity *pActivator ) {return true; }
	virtual CBaseToggle *MyTogglePointer( void ) { return NULL; }
	virtual CBaseMonster *MyMonsterPointer( void ) { return NULL; }
	virtual CSquadMonster *MySquadMonsterPointer( void ) { return NULL; }
	virtual	int GetToggleState( void ) { return TS_AT_TOP; }
	virtual void AddPoints( int score, bool bAllowNegativeScore ) { AddFloatPoints((float)score, bAllowNegativeScore); }
	virtual void AddPointsToTeam( int score, bool bAllowNegativeScore ) {}
	virtual int AddPlayerItem( CBasePlayerWeapon *pItem ) { return DID_NOT_GET_ITEM; }
	virtual bool RemovePlayerItem( CBasePlayerWeapon *pItem ) { return false; }
	virtual int GiveAmmo( int iAmount, const char *szName ) { return -1; }
	virtual float GetDelay( void ) { return 0; }
	virtual bool IsMoving( void ) { return pev->velocity != g_vecZero; }
	virtual void OverrideReset( void ) {}
	virtual int DamageDecal( int bitsDamageType );
	// This is ONLY used by the node graph to test movement through a door
	virtual void SetToggleState( int state ) {}
	virtual bool OnControls( entvars_t *pev ) { return false; }
	virtual bool IsAlive( void ) { return IsFullyAlive(); }
	virtual bool IsFullyAlive() { return (pev->deadflag == DEAD_NO) && pev->health > 0; } // IsAlive returns true for DEAD_DYING monsters. Use this when checking if monster is not dead and not dying
	virtual bool IsBSPModel( void ) { return pev->solid == SOLID_BSP || pev->movetype == MOVETYPE_PUSHSTEP; }
	virtual bool ReflectGauss( void ) { return ( IsBSPModel() && !pev->takedamage ); }
	virtual bool HasTarget( string_t targetname ) { return FStrEq(STRING(targetname), STRING(pev->target) ); }
	virtual bool IsInWorld( void );
	virtual	bool IsPlayer( void ) { return false; }
	virtual bool IsNetClient( void ) { return false; }
	virtual const char *TeamID( void ) { return ""; }

	//virtual void SetActivator( CBaseEntity *pActivator ) {}
	virtual CBaseEntity *GetNextTarget( void );
	
	// fundamental callbacks
	void ( CBaseEntity ::*m_pfnThink )( void);
	void ( CBaseEntity ::*m_pfnTouch )( CBaseEntity *pOther );
	void ( CBaseEntity ::*m_pfnUse )( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void ( CBaseEntity ::*m_pfnBlocked )( CBaseEntity *pOther );

	virtual void Think( void ) { if( m_pfnThink ) ( this->*m_pfnThink )(); }
	virtual void Touch( CBaseEntity *pOther ) { if( m_pfnTouch ) (this->*m_pfnTouch)( pOther ); }
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{ 
		if( m_pfnUse )
			( this->*m_pfnUse )( pActivator, pCaller, useType, value );
	}
	virtual void Blocked( CBaseEntity *pOther ) { if( m_pfnBlocked ) ( this->*m_pfnBlocked )( pOther ); }
	virtual bool ShouldCollide(CBaseEntity* pOther) { return true; }
	virtual bool ShouldCollideWithCorpses() { return true; }

	string_t m_entTemplate;
	string_t m_ownerEntTemplate;
	string_t m_soundList; // sound replacement list file name
	string_t m_objectHint; // the name of the spritehint template

	// Don't save those:
	const EntTemplate* m_cachedEntTemplate;
	const EntTemplate* m_cachedOwnerEntTemplate;
	bool m_entTemplateChecked;
	bool m_ownerEntTemplateChecked;

	int PRECACHE_SOUND(const char* soundName);
	static int PRECACHE_SOUND(const char* soundName, string_t soundList, const EntTemplate* entTemplate);

	bool EmitSoundDyn( int channel, const char *sample, float volume, float attenuation, int flags, int pitch );
	bool EmitSound( int channel, const char *sample, float volume, float attenuation );
	void EmitAmbientSound( const Vector &vecOrigin, const char *sample, float vol, float attenuation, int iFlags, int pitch );
	void StopSound( int channel, const char* sample );

	static const char* GetSoundScriptNameForTemplate(const char* name, const EntTemplate* entTemplate);
	const char* GetSoundScriptNameForMyTemplate(const char* name, string_t* usedTemplate = nullptr);
	const SoundScript* GetSoundScript(const char* name);
	bool EmitSoundScript(const SoundScript* soundScript, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	bool EmitSoundScript(const char* name, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	bool EmitSoundScriptSelectedSample(const SoundScript* soundScript, int sampleIndex, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	bool EmitSoundScriptSelectedSample(const SoundScript* soundScript, const char* sample, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	bool EmitSoundScriptSelectedSample(const char* name, int sampleIndex, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	bool EmitSoundScriptSelectedSample(const char* name, const char* sample, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	void StopSoundScript(const SoundScript* soundScript);
	void StopSoundScript(const char* name);
	void StopSoundScriptSelectedSample(const SoundScript* soundScript, int sampleIndex);
	void StopSoundScriptSelectedSample(const SoundScript* soundScript, const char* sample);
	void StopSoundScriptSelectedSample(const char* name, int sampleIndex);
	void StopSoundScriptSelectedSample(const char* name, const char* sample);
	void EmitSoundScriptAmbient(const Vector& vecOrigin, const SoundScript* soundScript, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	void EmitSoundScriptAmbient(const Vector& vecOrigin, const char* name, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride(), int flags = 0);
	void PrecacheSoundScript(const SoundScript& soundScript);
	void RegisterAndPrecacheSoundScriptByName(const char* name, const SoundScript& defaultSoundScript);
	void RegisterAndPrecacheSoundScript(const NamedSoundScript& defaultSoundScript);
	void RegisterAndPrecacheSoundScript(const char* derivative, const char* base, const SoundScript& defaultSoundScript, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride());
	void RegisterAndPrecacheSoundScript(const char* derivative, const NamedSoundScript& defaultSoundScript, const SoundScriptParamOverride paramsOverride = SoundScriptParamOverride());

	static const char* GetVisualNameForTemplate(const char* name, const EntTemplate* entTemplate);
	const char* GetVisualNameForMyTemplate(const char* name, string_t* usedTemplate = nullptr);
	const Visual* GetVisual(const char* name);
	const Visual* RegisterVisual(const NamedVisual& defaultVisual, bool precache = true, string_t* usedTemplate = nullptr);
	void AssignEntityOverrides(EntityOverrides entityOverrides);
	EntityOverrides GetProjectileOverrides();

	int OverridenRenderProps();
	virtual void ApplyDefaultRenderProps(int overridenRenderProps) {}
	void ApplyVisual(const Visual* visual, const char* modelOverride = nullptr);

	static const EntTemplate* GetCacheableEntTemplate(entvars_t* pev, string_t templateName, const EntTemplate*& entTemplate, bool& templateChecked, bool checkByClassname);
	const EntTemplate* GetMyEntTemplate();
	string_t GetMyTemplateName();
	const EntTemplate* GetOwnerEntTemplate();
	bool ShouldAutoPrecacheSounds();

	void SetMyHealth(const float defaultHealth);
	const Visual* MyOwnVisual();
	const char* MyOwnModel(const char* defaultModel);
	void SetMyModel(const char* defaultModel);
	void PrecacheMyModel(const char* defaultModel);

	// allow engine to allocate instance data
	void *operator new( size_t stAllocateBlock, entvars_t *pev )
	{
		return (void *)ALLOC_PRIVATE( ENT( pev ), stAllocateBlock );
	};

	// don't use this.
#if _MSC_VER >= 1200 // only build this code if MSVC++ 6.0 or higher
	void operator delete( void *pMem, entvars_t *pev )
	{
		pev->flags |= FL_KILLME;
	};
#endif

	virtual void UpdateOnRemove( void );

	// common member functions
	void EXPORT SUB_Remove( void );
	void EXPORT SUB_DoNothing( void );
	void EXPORT SUB_StartFadeOut ( void );
	void EXPORT SUB_FadeOut( void );
	void EXPORT SUB_CallUseToggle( void ) { this->Use( this, this, USE_TOGGLE, 0 ); }
	bool ShouldToggle( USE_TYPE useType, bool currentState );
	void FireBullets( ULONG cShots, Vector  vecSrc, Vector	vecDirShooting,	Vector	vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, entvars_t *pevAttacker = NULL  );
	Vector FireBulletsPlayer( ULONG cShots, Vector  vecSrc, Vector	vecDirShooting,	Vector	vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, entvars_t *pevAttacker = NULL, int shared_rand = 0 );

	virtual CBaseEntity *Respawn( void ) { return NULL; }

	void SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType = USE_TOGGLE, float value = 0.0f );
	// Do the bounding boxes of these two intersect?
	int Intersects( CBaseEntity *pOther );
	void MakeDormant( void );
	int IsDormant( void );

	static CBaseEntity *Instance( edict_t *pent )
	{
		if( !pent )
			pent = ENT( 0 );
		CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE( pent ); 
		return pEnt; 
	}

	static CBaseEntity *Instance( entvars_t *pev ) {
		if ( !pev )
			return Instance(ENT( 0 ));
		return Instance( ENT( pev ) );
	}
	static CBaseEntity *Instance( int eoffset) { return Instance( ENT( eoffset) ); }

	CBaseMonster *GetMonsterPointer( entvars_t *pevMonster ) 
	{ 
		CBaseEntity *pEntity = Instance( pevMonster );
		if( pEntity )
			return pEntity->MyMonsterPointer();
		return NULL;
	}
	CBaseMonster *GetMonsterPointer( edict_t *pentMonster ) 
	{ 
		CBaseEntity *pEntity = Instance( pentMonster );
		if( pEntity )
			return pEntity->MyMonsterPointer();
		return NULL;
	}

	// Ugly code to lookup all functions to make sure they are exported when set.
#if _DEBUG
	void FunctionCheck( void *pFunction, char *name ) 
	{ 
		if( pFunction && !NAME_FOR_FUNCTION( pFunction ) )
			ALERT( at_error, "No EXPORT: %s:%s (%08lx)\n", STRING( pev->classname ), name, (size_t)pFunction );
	}

	BASEPTR	ThinkSet( BASEPTR func, char *name ) 
	{ 
		m_pfnThink = func; 
		FunctionCheck( (void *)*( (int *)( (char *)this + ( offsetof( CBaseEntity, m_pfnThink ) ) ) ), name ); 
		return func;
	}
	ENTITYFUNCPTR TouchSet( ENTITYFUNCPTR func, char *name ) 
	{
		m_pfnTouch = func; 
		FunctionCheck( (void *)*( (int *)( (char *)this + ( offsetof( CBaseEntity, m_pfnTouch ) ) ) ), name );
		return func;
	}
	USEPTR UseSet( USEPTR func, char *name ) 
	{ 
		m_pfnUse = func; 
		FunctionCheck( (void *)*( (int *)( (char *)this + ( offsetof( CBaseEntity, m_pfnUse ) ) ) ), name );
		return func;
	}
	ENTITYFUNCPTR BlockedSet( ENTITYFUNCPTR func, char *name ) 
	{ 
		m_pfnBlocked = func; 
		FunctionCheck( (void *)*( (int *)( (char *)this + ( offsetof( CBaseEntity, m_pfnBlocked ) ) ) ), name ); 
		return func;
	}
#endif
	// virtual functions used by a few classes
	
	// used by monsters that are created by the MonsterMaker
	virtual	void UpdateOwner( void ) { return; };

	static CBaseEntity *Create( const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner = NULL, EntityOverrides entityOverrides = EntityOverrides() );
	static CBaseEntity *CreateNoSpawn( const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner = NULL, EntityOverrides entityOverrides = EntityOverrides() );

	virtual bool FBecomeProne( void ) {return false;}
	edict_t *edict() { return ENT( pev ); };
	EOFFSET eoffset() { return OFFSET( pev ); };
	int entindex() { return ENTINDEX( edict() ); };

	virtual Vector Center() { return ( pev->absmax + pev->absmin ) * 0.5; }; // center point of entity
	virtual Vector EyePosition() { return pev->origin + pev->view_ofs; };			// position of eyes
	virtual Vector EarPosition() { return pev->origin + pev->view_ofs; };			// position of ears
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center(); };		// position to shoot at

	virtual int Illumination() { return GETENTITYILLUM( ENT( pev ) ); };

	virtual	bool FVisible( CBaseEntity *pEntity );
	virtual	bool FVisible( const Vector &vecOrigin );

	virtual void AddFloatPoints( float score, bool bAllowNegativeScore ) {}
	virtual int DefaultSizeForGrapple() { return GRAPPLE_NOT_A_TARGET; }
	virtual int SizeForGrapple() { return DefaultSizeForGrapple(); }
	virtual bool IsDisplaceable() { return false; }
	virtual CBasePlayerWeapon* MyWeaponPointer() {return NULL;}
	virtual CBasePlayerAmmo* MyAmmoPointer() {return NULL;}

	virtual bool IsAlienMonster() { return false; }
	virtual bool HasFlesh() { return DefaultClassify() != CLASS_NONE && DefaultClassify() != CLASS_MACHINE; }
	virtual float InputByMonster(CBaseMonster* pMonster) { return 0.0f; }
	virtual NODE_LINKENT HandleLinkEnt(int afCapMask, bool nodeQueryStatic) { return NLE_PROHIBIT; }

	virtual void SendMessages(CBaseEntity* pClient) {}
	virtual bool HandleDoorBlockage(CBaseEntity* pDoor) { return false; }

	void ApplyDamageToHealth(float flDamage);
	void SetNonLethalHealthThreshold();
	float m_healthMinThreshold;

	virtual bool IsUsefulToDisplayHint(CBaseEntity* pPlayer) { return true; }
	virtual bool IsLockedByMaster() { return false; }
};

// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#if _DEBUG

#define SetThink( a ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), #a )
#define SetTouch( a ) TouchSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )
#define SetUse( a ) UseSet( static_cast <void (CBaseEntity::*)(	CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a), #a )
#define SetBlocked( a ) BlockedSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )
#define ResetThink() SetThink(NULL)

#else

#define SetThink( a ) m_pfnThink = static_cast <void (CBaseEntity::*)(void)> (a)
#define SetTouch( a ) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define SetUse( a ) m_pfnUse = static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a)
#define SetBlocked( a ) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define ResetThink( ) m_pfnThink = static_cast <void (CBaseEntity::*)(void)> (NULL)
#define ResetTouch( ) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (NULL)
#define ResetUse( ) m_pfnUse = static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (NULL)
#define ResetBlocked( ) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (NULL)

#endif

class CPointEntity : public CBaseEntity
{
public:
	void Spawn( void );
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
private:
};

typedef struct locksounds			// sounds that doors and buttons make when locked/unlocked
{
	string_t	sLockedSound;		// sound a door makes when it's locked
	string_t	sLockedSentence;	// sentence group played when door is locked
	string_t	sUnlockedSound;		// sound a door makes when it's unlocked
	string_t	sUnlockedSentence;	// sentence group played when door is unlocked

	int		iLockedSentence;		// which sentence in sentence group to play next
	int		iUnlockedSentence;		// which sentence in sentence group to play next

	float	flwaitSound;			// time delay between playing consecutive 'locked/unlocked' sounds
	float	flwaitSentence;			// time delay between playing consecutive sentences
	BYTE	bEOFLocked;				// true if hit end of list of locked sentences
	BYTE	bEOFUnlocked;			// true if hit end of list of unlocked sentences
} locksound_t;

void PlayLockSounds( entvars_t *pev, locksound_t *pls, bool flocked, bool fbutton );

//
// generic Delay entity.
//
class CBaseDelay : public CBaseEntity
{
public:
	float m_flDelay;
	string_t m_iszKillTarget;
	EHANDLE m_hActivator;

	virtual void KeyValue( KeyValueData *pkvd );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	// common member functions
	void SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType = USE_TOGGLE, float value = 0.0f );
	static void DelayedUse(float delay, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, string_t target, string_t killTarget = iStringNull, float value = 0.0f );
	void EXPORT DelayThink( void );
};

class CBaseAnimating : public CBaseDelay
{
public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	// Basic Monster Animation functions
	float StudioFrameAdvance( float flInterval = 0.0 ); // accumulate animation frame time from last time called until now
	int GetSequenceFlags( void );
	virtual int LookupActivity( int activity );
	int LookupActivityHeaviest( int activity );
	int LookupSequence( const char *label );
	void ResetSequenceInfo();
	void DispatchAnimEvents( float flFutureInterval = 0.1 ); // Handle events that have happend since last time called up until X seconds into the future
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent ) { return; };
	float SetBoneController( int iController, float flValue );
	void InitBoneControllers( void );
	float SetBlending( int iBlender, float flValue );
	void GetBonePosition( int iBone, Vector &origin, Vector &angles );
	void GetAutomovement( Vector &origin, Vector &angles, float flInterval = 0.1 );
	int FindTransition( int iEndingSequence, int iGoalSequence, int *piDir );
	void GetAttachment( int iAttachment, Vector &origin, Vector &angles );
	void SetBodygroup( int iGroup, int iValue );
	int GetBodygroup( int iGroup );
	int ExtractBbox( int sequence, float *mins, float *maxs );
	void SetSequenceBox( void );

	// animation needs
	float m_flFrameRate;		// computed FPS for current sequence
	float m_flGroundSpeed;	// computed linear movement rate for current sequence
	float m_flLastEventCheck;	// last time the event list was checked
	bool m_fSequenceFinished;// flag set when StudioAdvanceFrame moves across a frame boundry
	bool m_fSequenceLoops;	// true if the sequence loops
	int m_minAnimEventFrame;
};

//
// generic Toggle entity.
//

class CBaseToggle : public CBaseAnimating
{
public:
	void				KeyValue( KeyValueData *pkvd );

	TOGGLE_STATE		m_toggle_state;
	float				m_flActivateFinished;//like attack_finished, but for doors
	float				m_flMoveDistance;// how far a door should slide or rotate
	float				m_flWait;
	float				m_flLip;
	float				m_flTWidth;// for plats
	float				m_flTLength;// for plats

	Vector				m_vecPosition1;
	Vector				m_vecPosition2;
	Vector				m_vecAngle1;
	Vector				m_vecAngle2;

	float				m_flHeight;
	void (CBaseToggle::*m_pfnCallWhenMoveDone)(void);
	Vector				m_vecFinalDest;
	Vector				m_vecFinalAngle;

	int					m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	CBaseToggle *MyTogglePointer( void ) { return this; }
	virtual int		GetToggleState( void ) { return m_toggle_state; }
	virtual float	GetDelay( void ) { return m_flWait; }

	virtual bool PlaySentence( const char *pszSentence, float duration, float volume, float attenuation, bool subtitle );
	virtual void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener );
	virtual void SentenceStop( void );
	virtual bool IsAllowedToSpeak( void ) { return false; }

	// common member functions
	void LinearMove( Vector	vecDest, float flSpeed );
	void EXPORT LinearMoveDone( void );
	void AngularMove( Vector vecDestAngle, float flSpeed );
	void EXPORT AngularMoveDone( void );
	bool IsLockedByMaster();

	static float		AxisValue( int flags, const Vector &angles );
	static void			AxisDir( entvars_t *pev );
	static float		AxisDelta( int flags, const Vector &angle1, const Vector &angle2 );

	string_t m_sMaster;		// If this button has a master switch, this is the targetname.
							// A master switch must be of the multisource type. If all 
							// of the switches in the multisource have been triggered, then
							// the button will be allowed to operate. Otherwise, it will be
							// deactivated.
};
#define SetMoveDone( a ) m_pfnCallWhenMoveDone = static_cast <void (CBaseToggle::*)(void)> (a)



// people gib if their health is <= this at the time of death
#define	GIB_HEALTH_VALUE	-30

#define	ROUTE_SIZE			8 // how many waypoints a monster can store at one time
#define MAX_OLD_ENEMIES		4 // how many old enemies to remember

#define	bits_CAP_DUCK			( 1 << 0 )// crouch
#define	bits_CAP_JUMP			( 1 << 1 )// jump/leap
#define bits_CAP_STRAFE			( 1 << 2 )// strafe ( walk/run sideways)
#define bits_CAP_SQUAD			( 1 << 3 )// can form squads
#define	bits_CAP_SWIM			( 1 << 4 )// proficiently navigate in water
#define bits_CAP_CLIMB			( 1 << 5 )// climb ladders/ropes
#define bits_CAP_USE			( 1 << 6 )// open doors/push buttons/pull levers
#define bits_CAP_HEAR			( 1 << 7 )// can hear forced sounds
#define bits_CAP_AUTO_DOORS		( 1 << 8 )// can trigger auto doors
#define bits_CAP_OPEN_DOORS		( 1 << 9 )// can open manual doors
#define bits_CAP_TURN_HEAD		( 1 << 10)// can turn head, always bone controller 0

#define bits_CAP_RANGE_ATTACK1	( 1 << 11)// can do a range attack 1
#define bits_CAP_RANGE_ATTACK2	( 1 << 12)// can do a range attack 2
#define bits_CAP_MELEE_ATTACK1	( 1 << 13)// can do a melee attack 1
#define bits_CAP_MELEE_ATTACK2	( 1 << 14)// can do a melee attack 2

#define bits_CAP_FLY			( 1 << 15)// can fly, move all around

#define bits_CAP_USABLE			( 1 << 16 ) // can be used by player

#define bits_CAP_DOORS_GROUP    (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)

#define bits_CAP_MONSTERCLIPPED ( 1 << 31 )

// used by suit voice to indicate damage sustained and repaired type to player

#include "dmg_types.h"
#define DMG_TIMEBASED		(DMG_PARALYZE|DMG_NERVEGAS|DMG_POISON|DMG_RADIATION|DMG_DROWNRECOVER|DMG_ACID|DMG_SLOWBURN|DMG_SLOWFREEZE)	// mask for time-based damage

// these are the damage types that are allowed to gib corpses
#define DMG_GIB_CORPSE		( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB )

// these are the damage types that have client hud art
#define DMG_SHOWNHUD		(DMG_POISON | DMG_ACID | DMG_FREEZE | DMG_SLOWFREEZE | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK)

// NOTE: tweak these values based on gameplay feedback:

#define PARALYZE_DURATION	2		// number of 2 second intervals to take damage
#define PARALYZE_DAMAGE		1.0		// damage to take each 2 second interval

#define NERVEGAS_DURATION	2
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		5
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	2
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		2
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	2
#define SLOWFREEZE_DAMAGE	1.0


#define	itbd_Paralyze		0		
#define	itbd_NerveGas		1
#define	itbd_Poison			2
#define	itbd_Radiation		3
#define	itbd_DrownRecover	4
#define	itbd_Acid			5
#define	itbd_SlowBurn		6
#define	itbd_SlowFreeze		7
#define CDMG_TIMEBASED		8

// when calling KILLED(), a value that governs gib behavior is expected to be 
// one of these three values
#define GIB_NORMAL			0// gib if entity was overkilled
#define GIB_NEVER			1// never gib, no matter how much death damage is done ( freezing, etc )
#define GIB_ALWAYS			2// always gib ( Houndeye Shock, Barnacle Bite )

class CBaseMonster;
class CCineMonster;
class CSound;

#include "basemonster.h"

const char *ButtonSound( int sound );				// get string of button sound number

//
// Weapons 
//
#define	BAD_WEAPON 0x00007FFF

//
// Converts a entvars_t * to a class pointer
// It will allocate the class and entity if necessary
//
template <class T> T * GetClassPtr( T *a )
{
	entvars_t *pev = (entvars_t *)a;

	// allocate entity if necessary
	if( pev == NULL )
		pev = VARS( CREATE_ENTITY() );

	// get the private data
	a = (T *)GET_PRIVATE( ENT( pev ) );

	if( a == NULL ) 
	{
		// allocate private data 
		a = new( pev ) T;
		a->pev = pev;
	}
	return a;
}

/*
bit_PUSHBRUSH_DATA | bit_TOGGLE_DATA
bit_MONSTER_DATA
bit_DELAY_DATA
bit_TOGGLE_DATA | bit_DELAY_DATA | bit_MONSTER_DATA
bit_PLAYER_DATA | bit_MONSTER_DATA
bit_MONSTER_DATA | CYCLER_DATA
bit_LIGHT_DATA
path_corner_data
bit_MONSTER_DATA | wildcard_data
bit_MONSTER_DATA | bit_GROUP_DATA
boid_flock_data
boid_data
CYCLER_DATA
bit_ITEM_DATA
bit_ITEM_DATA | func_hud_data
bit_TOGGLE_DATA | bit_ITEM_DATA
EOFFSET
env_sound_data
env_sound_data
push_trigger_data
*/

#define TRACER_FREQ		4			// Tracers fire every 4 bullets

typedef struct _SelAmmo
{
	BYTE Ammo1Type;
	BYTE Ammo1;
	BYTE Ammo2Type;
	BYTE Ammo2;
} SelAmmo;

// this moved here from world.cpp, to allow classes to be derived from it
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================
#define SF_WORLD_DARK		0x0001		// Fade from black at startup
#define SF_WORLD_TITLE		0x0002		// Display game title at startup
#define SF_WORLD_FORCETEAM	0x0004		// Force teams
#define SF_WORLD_FREEROAM	0x0008		// Monsters freeroaming by default

class CWorld : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
};

namespace detail
{
template<typename T, FIELDTYPE ft>
struct assert_false : public std::false_type {};

template<typename T, size_t N>
struct array_error_tag : public std::false_type {};
}

template<typename T, FIELDTYPE ft, typename E = void>
struct FieldDefiner {
	static_assert(detail::assert_false<T, ft>::value, "Save-restore data type mismatch. Ensure the provided field type matches the actual C++ data type (including the array size in case it's an array)");
};

#define DEFINE_FIELD_DEFINER(type, ft) template<> struct FieldDefiner<type, ft> \
{\
	static TYPEDESCRIPTION D(const char* name, int offset, short count, short flags)\
	{\
		return {ft, name, offset, count, flags};\
	}\
};\
template<size_t N>\
struct FieldDefiner<type[N], ft>\
{\
	static TYPEDESCRIPTION D(const char* name, int offset, short count, short flags)\
	{\
		return {ft, name, offset, count, flags};\
	}\
}

DEFINE_FIELD_DEFINER(float, FIELD_FLOAT);
DEFINE_FIELD_DEFINER(string_t, FIELD_STRING);
DEFINE_FIELD_DEFINER(EOFFSET, FIELD_ENTITY);
DEFINE_FIELD_DEFINER(EHANDLE, FIELD_EHANDLE);
DEFINE_FIELD_DEFINER(entvars_t*, FIELD_EVARS);
DEFINE_FIELD_DEFINER(edict_t*, FIELD_EDICT);
DEFINE_FIELD_DEFINER(Vector, FIELD_VECTOR);
DEFINE_FIELD_DEFINER(Vector, FIELD_POSITION_VECTOR);
DEFINE_FIELD_DEFINER(int, FIELD_INTEGER);
DEFINE_FIELD_DEFINER(unsigned int, FIELD_INTEGER);
DEFINE_FIELD_DEFINER(decltype(CBaseEntity::m_pfnThink), FIELD_FUNCTION);
DEFINE_FIELD_DEFINER(decltype(CBaseEntity::m_pfnTouch), FIELD_FUNCTION);
DEFINE_FIELD_DEFINER(decltype(CBaseEntity::m_pfnUse), FIELD_FUNCTION);
DEFINE_FIELD_DEFINER(decltype(CBaseToggle::m_pfnCallWhenMoveDone), FIELD_FUNCTION);
DEFINE_FIELD_DEFINER(bool, FIELD_BOOLEAN);
DEFINE_FIELD_DEFINER(short, FIELD_SHORT);
DEFINE_FIELD_DEFINER(char, FIELD_CHARACTER);
DEFINE_FIELD_DEFINER(unsigned char, FIELD_CHARACTER);
DEFINE_FIELD_DEFINER(float, FIELD_TIME);
DEFINE_FIELD_DEFINER(string_t, FIELD_MODELNAME);
DEFINE_FIELD_DEFINER(string_t, FIELD_SOUNDNAME);
DEFINE_FIELD_DEFINER(std::uint64_t, FIELD_INT64);

template<typename T>
struct FieldDefiner<T*, FIELD_CLASSPTR, typename std::enable_if<
	std::is_base_of<CBaseEntity, T>::value>::type>
{
	static TYPEDESCRIPTION D(const char* name, int offset, short count, short flags)
	{
		return {FIELD_CLASSPTR, name, offset, count, flags};
	}
};

template<typename T, size_t N>
struct FieldDefiner<T*[N], FIELD_CLASSPTR, typename std::enable_if<
	std::is_base_of<CBaseEntity, T>::value>::type>
{
	static TYPEDESCRIPTION D(const char* name, int offset, short count, short flags)
	{
		return {FIELD_CLASSPTR, name, offset, count, flags};
	}
};

template<typename T>
struct FieldDefiner<T, FIELD_INTEGER, typename std::enable_if<std::is_enum<T>::value && sizeof(T) == sizeof(int)>::type>
{
	static TYPEDESCRIPTION D(const char* name, int offset, short count, short flags)
	{
		return {FIELD_INTEGER, name, offset, count, flags};
	}
};

template<typename T>
struct FieldDefiner<T, FIELD_CHARACTER, typename std::enable_if<
	std::is_pod<T>::value && !std::is_array<T>::value && sizeof(T) >= 2>::type>
{
	static TYPEDESCRIPTION D(const char* name, int offset, short count, short flags)
	{
		return {FIELD_CHARACTER, name, offset, count, flags};
	}
};

#define _FIELD_SAFE(T,name,fieldtype,count,flags) FieldDefiner<\
	std::conditional<\
		std::is_array<decltype(T::name)>::value && count != std::extent<decltype(T::name)>::value,\
		detail::array_error_tag<decltype(T::name), count>,\
		decltype(T::name)>::type, fieldtype>::D(#name, offsetof(T, name), count, flags)
#define DEFINE_FIELD(type,name,fieldtype)		_FIELD_SAFE(type, name, fieldtype, 1, 0)
#define DEFINE_ENTITY_FIELD(name,fieldtype)		_FIELD_SAFE(entvars_t, name, fieldtype, 1, 0 )
#define DEFINE_ENTITY_ARRAY(name, fieldtype, count) _FIELD_SAFE(entvars_t, name, fieldtype, count, 0)
#define DEFINE_ARRAY(type,name,fieldtype,count)		_FIELD_SAFE(type, name, fieldtype, count, 0)
#define DEFINE_ENTITY_GLOBAL_FIELD(name,fieldtype)	_FIELD_SAFE(entvars_t, name, fieldtype, 1, FTYPEDESC_GLOBAL )
#define DEFINE_GLOBAL_FIELD(type,name,fieldtype)		_FIELD_SAFE(type, name, fieldtype, 1, FTYPEDESC_GLOBAL )

#endif
