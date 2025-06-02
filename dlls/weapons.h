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
#if !defined(WEAPONS_H)
#define WEAPONS_H

#include "weapon_ids.h"
#include "mod_features.h"
#include "bullet_types.h"
#include "weapon_animations.h"
#include "weaponinfo.h"
#include "player_items.h"
#include "ammoregistry.h"
#include "cone_degrees.h"

#if !CLIENT_DLL
#include "combat.h"
#include "effects.h"
#include "ggrenade.h"
#include "global_models.h"
#endif

class CBasePlayer;
extern int gmsgWeapPickup;

void DeactivateSatchels( CBasePlayer *pOwner );

// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT		0
#define GLOCK_WEIGHT		10
#define PYTHON_WEIGHT		15
#define MP5_WEIGHT			15
#define SHOTGUN_WEIGHT		15
#define CROSSBOW_WEIGHT		10
#define RPG_WEIGHT			20
#define GAUSS_WEIGHT		20
#define EGON_WEIGHT			20
#define HORNETGUN_WEIGHT	15
#define HANDGRENADE_WEIGHT	5
#define SNARK_WEIGHT		5
#define SATCHEL_WEIGHT		-10
#define TRIPMINE_WEIGHT		-10
#define EAGLE_WEIGHT		15
#define PIPEWRENCH_WEIGHT		0
#define M249_WEIGHT			15
#define SNIPERRIFLE_WEIGHT		10
#define DISPLACER_WEIGHT		20
#define SHOCKRIFLE_WEIGHT		15
#define SPORELAUNCHER_WEIGHT		20
#define KNIFE_WEIGHT			0
#define GRAPPLE_WEIGHT			21
#define MEDKIT_WEIGHT		-1
#define UZI_WEIGHT			15

// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY		100
#define	_9MM_MAX_CARRY			250
#define _357_MAX_CARRY			36
#define BUCKSHOT_MAX_CARRY		125
#define BOLT_MAX_CARRY			50
#define ROCKET_MAX_CARRY		5
#define HANDGRENADE_MAX_CARRY	10
#define SATCHEL_MAX_CARRY		5
#define TRIPMINE_MAX_CARRY		5
#define SNARK_MAX_CARRY			15
#define HORNET_MAX_CARRY		8
#define M203_GRENADE_MAX_CARRY	10
#define PENGUIN_MAX_CARRY		9
#define	_556_MAX_CARRY			200
#define _762_MAX_CARRY			15
#define SHOCK_MAX_CARRY			10
#define SPORE_MAX_CARRY			20
#define MEDKIT_MAX_CARRY		100

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP			17
#define PYTHON_MAX_CLIP			6
#define MP5_MAX_CLIP			50
#define SHOTGUN_MAX_CLIP		8
#define CROSSBOW_MAX_CLIP		5
#define RPG_MAX_CLIP			1
#define GAUSS_MAX_CLIP			WEAPON_NOCLIP
#define EGON_MAX_CLIP			WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP		WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP	WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP		WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP		WEAPON_NOCLIP
#define SNARK_MAX_CLIP			WEAPON_NOCLIP
#define EAGLE_MAX_CLIP			7
#define M249_MAX_CLIP			50
#define SNIPERRIFLE_MAX_CLIP	5
#define SHOCKRIFLE_MAX_CLIP		WEAPON_NOCLIP
#define SPORELAUNCHER_MAX_CLIP		5
#define UZI_MAX_CLIP			32

// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE			17
#define PYTHON_DEFAULT_GIVE			6
#if FEATURE_OPFOR_SPECIFIC
#define MP5_DEFAULT_GIVE			50
#else
#define MP5_DEFAULT_GIVE			25
#endif
#define MP5_DEFAULT_GIVE_MP			MP5_MAX_CLIP
#define MP5_M203_DEFAULT_GIVE		0
#define SHOTGUN_DEFAULT_GIVE		12
#define CROSSBOW_DEFAULT_GIVE		5
#define RPG_DEFAULT_GIVE			1
#define GAUSS_DEFAULT_GIVE			20
#define EGON_DEFAULT_GIVE			20
#define HANDGRENADE_DEFAULT_GIVE	5
#define SATCHEL_DEFAULT_GIVE		1
#define TRIPMINE_DEFAULT_GIVE		1
#define SNARK_DEFAULT_GIVE			5
#define HIVEHAND_DEFAULT_GIVE		8
#define EAGLE_DEFAULT_GIVE			7
#define PENGUIN_DEFAULT_GIVE		3
#define M249_DEFAULT_GIVE			50
#define SNIPERRIFLE_DEFAULT_GIVE		5
#define DISPLACER_DEFAULT_GIVE		40
#define SHOCKRIFLE_DEFAULT_GIVE		10
#define SPORELAUNCHER_DEFAULT_GIVE	5
#define MEDKIT_DEFAULT_GIVE			50
#define UZI_DEFAULT_GIVE			UZI_MAX_CLIP

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_GLOCKCLIP_GIVE		GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE		PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE		MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE		200
#define AMMO_M203BOX_GIVE		2
#define AMMO_BUCKSHOTBOX_GIVE	12
#define AMMO_CROSSBOWCLIP_GIVE	CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE		RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_SNARKBOX_GIVE		5
#define AMMO_PENGUINBOX_GIVE		3
#define AMMO_556CLIP_GIVE			50
#define AMMO_762BOX_GIVE		5
#define AMMO_SPORE_GIVE			1

#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NOAUTOSWITCHTO	32

#define WEAPON_IS_ONTARGET 0x40

struct ItemInfo
{
	int		iSlot = 0;
	int		iPosition = 0;
	const char	*pszAmmo1 = nullptr;	// ammo 1 type
	const char	*pszAmmo2 = nullptr;	// ammo 2 type
	const char	*pszName = nullptr;
	int		iId = 0;
	int		iFlags = 0;
	const char* pszAmmoEntity = nullptr;
	int iDropAmmo = 0;
};

#if !CLIENT_DLL
void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, CBasePlayer *pPlayer );
#endif

struct WeaponInfo
{
	int id = 0;
	const char* classname = nullptr;
	CBasePlayerWeapon* pWeapon = nullptr;
};

extern WeaponInfo& AccessWeaponInfo(int id);

class CBasePlayerWeapon : public CBaseAnimating
{
public:
	virtual void SetObjectCollisionBox( void );
	virtual void KeyValue( KeyValueData *pkvd );

#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	virtual int WeaponId() const { return WEAPON_NONE; }
	bool IsEnabledInMod();
	virtual bool AddToPlayer( CBasePlayer *pPlayer );	// return true if the item you want the item added to the player inventory
	void EXPORT DestroyItem( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	void EXPORT FallThink ( void );// when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize( void );// make a weapon visible and tangible
	void EXPORT AttemptToMaterialize( void );  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn ( void );// copy a weapon
	void FallInit( void );
	void CheckRespawn( void );
	virtual bool GetItemInfo(ItemInfo *p) { return false; }	// returns 0 if struct not filled out
	virtual bool CanDeploy( void );
	virtual bool Deploy()								// returns is deploy was successful
		 { return true; }

	virtual bool CanHolster() { return true; }// can this weapon be put away right now?

	virtual void ItemPreFrame( void )	{ return; }		// called each frame by the player PreThink

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	int ObjectCaps();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	static const AmmoType* GetAmmoType( const char* name );

	static ItemInfo ItemInfoArray[ MAX_WEAPONS ];

	CBasePlayer	*m_pPlayer;

	int			iItemPosition( void ) { return ItemInfoArray[ WeaponId() ].iPosition; }
	const char	*pszAmmo1( void ) const { return ItemInfoArray[ WeaponId() ].pszAmmo1; }
	int			iMaxAmmo1( void )	{
		if (m_iPrimaryAmmoType > 0)
			return g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType);
		return g_AmmoRegistry.GetMaxAmmo(pszAmmo1());
	}
	bool UsesAmmo() const {
		return m_iPrimaryAmmoType > 0 || pszAmmo1() != NULL;
	}
	const char	*pszAmmo2( void ) const { return ItemInfoArray[ WeaponId() ].pszAmmo2; }
	int			iMaxAmmo2( void )	{
		if (m_iSecondaryAmmoType > 0)
			return g_AmmoRegistry.GetMaxAmmo(m_iSecondaryAmmoType);
		return g_AmmoRegistry.GetMaxAmmo(pszAmmo2());
	}
	bool UsesSecondaryAmmo() const {
		return m_iSecondaryAmmoType > 0 || pszAmmo2() != NULL;
	}

	const char	*pszName( void )	{ return ItemInfoArray[ WeaponId() ].pszName; }
	int			iMaxClip( void );
	int			iWeight( void )		{ return DefaultWeight(); }
	int			iFlags( void )		{ return ItemInfoArray[ WeaponId() ].iFlags; }
	const char* pszAmmoEntity( void ) { return ItemInfoArray[ WeaponId() ].pszAmmoEntity; }
	int			iDropAmmo( void )	{ return ItemInfoArray[ WeaponId() ].iDropAmmo; }

	virtual const char* MyWModel() { return 0; }

	bool AddToPlayerDefault( CBasePlayer *pPlayer );
	virtual int AddDuplicate( CBasePlayerWeapon *pItem );

	virtual bool ExtractAmmo( CBasePlayerWeapon *pWeapon );	// TODO: check the return type usage. Return true if you can add ammo to yourself when picked up
	virtual bool ExtractClipAmmo( CBasePlayerWeapon *pWeapon );	// TODO: check the return type usage. Return true if you can add ammo to yourself when picked up

	virtual bool AddWeapon( void ) { ExtractAmmo( this ); return true; }	// Return true if you want to add yourself to the player

	// generic "shared" ammo handlers
	bool AddPrimaryAmmo(int iCount);
	bool AddSecondaryAmmo(int iCount);

	virtual void UpdateItemInfo( void ) {}	// updates HUD state

	//Special stuff for grenades and satchels.
	float m_flStartThrow;
	float m_flReleaseThrow;
	int m_chargeReady;
	int m_fInAttack;

	enum EGON_FIRESTATE { FIRE_OFF, FIRE_CHARGE };
	int m_fireState;

	bool m_iPlayEmptySound;
	bool m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the
							// attack key(s)
	virtual bool PlayEmptySound( void );
	virtual void ResetEmptySound( void );

	virtual void SendWeaponAnim( int iAnim, int body = 0 );  // skiplocal is 1 if client is predicting weapon animations

	virtual bool IsUseable( void );
	bool DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int body = 0 );
	bool DefaultReload( int iClipSize, int iAnim, float fDelay, int body = 0 );
	bool DefaultClipReload(int iAnim, float fDelay, int body = 0);
	void PrecachePModel(const char* name);

	virtual void ItemPostFrame( void );	// called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack( void ) { return; }				// do "+ATTACK"
	virtual void SecondaryAttack( void ) { return; }			// do "+ATTACK2"
	bool CanReload();
	virtual void Reload( void ) { return; }						// do "+RELOAD"
	virtual void WeaponIdle( void ) { return; }					// called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );		// sends hud info to client dll, if things have changed
	virtual void GetWeaponData(weapon_data_t& data) {}
	virtual void SetWeaponData(const weapon_data_t& data) {}

	virtual void RetireWeapon( void );
	virtual bool ShouldWeaponIdle( void ) { return false; }
	virtual void Holster();
	virtual bool UseDecrement()
	{
#if CLIENT_WEAPONS
		return true;
#else
		return false;
#endif
	}
	inline int PlaybackFlags()
	{
#if CLIENT_WEAPONS
		return FEV_NOTHOST;
#else
		return 0;
#endif
	}
	virtual int DefaultWeight() { return 0; }

	int	PrimaryAmmoIndex();
	int	SecondaryAmmoIndex();
	const char* AmmoName(const char* defaultAmmoName);

	void PrintState( void );

	virtual CBasePlayerWeapon *MyWeaponPointer( void ) { return this; }
	virtual bool CanBeDropped() { return true; }
	virtual int ViewModelBody() { return 0; }
	float GetNextAttackDelay( float delay );
	bool InZoom();

	int		m_fInSpecialReload;									// Are we in the middle of a reload for the shotguns
	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;

	void	InitDefaultAmmo(int defaultGive);
	int		m_iDefaultAmmo;// how much ammo you get when you pick up this weapon as placed by a level designer.

	string_t m_sMaster;

	// hle time creep vars
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;

	//Hack so deploy animations work when weapon prediction is enabled.
	bool m_ForceSendAnimations;

	void InitMaxClip(int defaultMaxClip);
	int m_iMaxClip;
	int m_iClientMaxClip;

	bool UsesClip();
	bool HasAmmoToFire(int ammo = 1);
	bool IsOutOfAmmo();
	void CheckOutOfAmmo();
	void SpendAmmo(int ammo = 1);
	bool Emptied();
};

#define LOUD_GUN_VOLUME			1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME		200

#define	BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define	DIM_GUN_FLASH			128

#define	WEAPON_ACTIVITY_VOLUME	64

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseDelay
{
public:
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	bool IsEmpty( void );
	void SetObjectCollisionBox( void );

	int ObjectCaps();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	void EXPORT Kill ( void );
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	bool HasWeapon( CBasePlayerWeapon *pCheckItem );
	bool PackWeapon( CBasePlayerWeapon *pWeapon );
	bool PackAmmo( string_t iszName, int iCount );

	void SetWeaponModel( CBasePlayerWeapon* pItem );

	void InsertWeaponById( CBasePlayerWeapon* pItem );
	CBasePlayerWeapon* WeaponById( int id );

	CBasePlayerWeapon	*m_rgpPlayerWeapons[MAX_WEAPONS];// one slot for each

	string_t m_rgiszAmmo[MAX_AMMO_TYPES];// ammo names
	int	m_rgAmmo[MAX_AMMO_TYPES];// ammo quantities

	int m_cAmmoTypes;// how many ammo types packed into this box (if packed by a level designer)
};

bool bIsMultiplayer ( void );

#if CLIENT_DLL
void LoadVModel ( const char *szViewModel, CBasePlayer *m_pPlayer );
#endif

class CGlock : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_GLOCK; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void GlockFire( float flSpread, float flCycleTime, bool fUseAutoAim );
	bool Deploy() override;
	void Reload( void );
	void WeaponIdle( void );

	const char* MyWModel() override { return "models/w_9mmhandgun.mdl"; }
	int DefaultWeight() override { return GLOCK_WEIGHT; }
private:
	int m_iShell;

	unsigned short m_usFireGlock1;
	unsigned short m_usFireGlock2;
};

class CCrowbar : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_CROWBAR; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	bool Swing( bool fFirst );
	bool Deploy() override;
	void Holster();
	void WeaponIdle();
	int m_iSwing;
	TraceResult m_trHit;

	const char* MyWModel() override { return "models/w_crowbar.mdl"; }
	int DefaultWeight() override { return CROWBAR_WEIGHT; }
private:
	unsigned short m_usCrowbar;
};

class CPython : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_PYTHON; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Holster();
	void Reload( void );
	void WeaponIdle( void );
	float m_flSoundDelay;

	int ViewModelBody() override;

	const char* MyWModel() override { return "models/w_357.mdl"; }
	int DefaultWeight() override { return PYTHON_WEIGHT; }
private:
	unsigned short m_usFirePython;
};

class CMP5 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_MP5; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;

	const char* MyWModel() override { return "models/w_9mmAR.mdl"; }
	int DefaultWeight() override { return MP5_WEIGHT; }

private:
	unsigned short m_usMP5;
	unsigned short m_usMP52;
};

class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_CROSSBOW; }
	bool GetItemInfo(ItemInfo *p) override;

	void FireBolt( void );
	void FireSniperBolt( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool AddToPlayer( CBasePlayer *pPlayer ) override;
	bool Deploy() override;
	void Holster();
	void Reload( void );
	void WeaponIdle( void );

	const char* MyWModel() override { return "models/w_crossbow.mdl"; }
	int DefaultWeight() override { return CROSSBOW_WEIGHT; }

private:
	unsigned short m_usCrossbow;
	unsigned short m_usCrossbow2;
};

class CShotgun : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_SHOTGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Reload( void );
	void WeaponIdle( void );

	void ItemPostFrame( void );
	float m_flPumpTime;
	float m_flNextReload;
	int m_iShell;

	const char* MyWModel() { return "models/w_shotgun.mdl"; }

private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};

class CLaserSpot : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }

	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;

	static CLaserSpot *CreateSpot( edict_t* pOwner = 0 );
};

class CRpg : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_RPG; }
	void Reload( void );
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	bool Deploy() override;
	bool CanHolster() override;
	void Holster();

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void UpdateSpot( void );
	bool ShouldWeaponIdle() override { return true; }

	CLaserSpot *m_pSpot;
	int m_fSpotActive;
	int m_cActiveRockets;// how many missiles in flight from this launcher right now?

	const char* MyWModel() override { return "models/w_rpg.mdl"; }
	int DefaultWeight() override { return RPG_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

private:
	unsigned short m_usRpg;
};

class CGauss : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_GAUSS; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	bool Deploy() override;
	void Holster();

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void StartFire( void );
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	float GetFullChargeTime( void );
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iSoundState; // don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects.
	bool m_fPrimaryFire;

	const char* MyWModel() override { return "models/w_gauss.mdl"; }
	int DefaultWeight() override { return GAUSS_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

private:
	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};

class CEgon : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_EGON; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	bool Deploy() override;
	bool CanHolster() override;
	void Holster();

	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );

	void CreateEffect ( void );
	void DestroyEffect ( void );
	void EndAttack( void );
	void Attack( void );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	bool HasAmmo( void );

	void UseAmmo( int count );

	enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

#if !CLIENT_DLL
	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;
#endif

	const char* MyWModel() override { return "models/w_egon.mdl"; }
	int DefaultWeight() override { return EGON_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

	unsigned short m_usEgonStop;

private:
#if !CLIENT_DLL
	float				m_shootTime;
#endif
	EGON_FIREMODE		m_fireMode;
	float				m_shakeTime;

	unsigned short m_usEgonFire;
};

class CHgun : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_HORNETGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	bool IsUseable( void ) override;
	void Holster();
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;

	float m_flRechargeTime;

	int m_iFirePhase;

	const char* MyWModel() override { return "models/w_hgun.mdl"; }
	int DefaultWeight() override { return HORNETGUN_WEIGHT; }
private:
	unsigned short m_usHornetFire;
};

class CHandGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_HANDGRENADE; }
	bool GetItemInfo(ItemInfo *p) override;

	void PrimaryAttack( void );
	bool Deploy() override;
	bool CanHolster() override;
	void Holster();
	void WeaponIdle( void );
	bool PreferNewPhysics();

	const char* MyWModel() override { return "models/w_grenade.mdl"; }
	int DefaultWeight() override { return HANDGRENADE_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
};

class CSatchel : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_SATCHEL; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddDuplicate(CBasePlayerWeapon *pOriginal );
	bool CanDeploy( void ) override;
	bool Deploy() override;
	bool IsUseable( void ) override;
	bool CanBeDropped();

	void Holster();
	void ItemPreFrame();
	void WeaponIdle( void );
	void Throw( void );
	void Detonate(bool allowThrow);
	int ControlBehavior();
	void DrawSatchel( void );
	void DrawRadio();

	const char* MyWModel() override { return "models/w_satchel.mdl"; }
	int DefaultWeight() override { return SATCHEL_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
};

class CTripmine : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_TRIPMINE; }
	bool GetItemInfo(ItemInfo *p) override;
	void SetObjectCollisionBox( void )
	{
		//!!!BUGBUG - fix the model!
		SetMyObjectCollisionBox(Vector(-16, -16, -5), Vector(16, 16, 28));
	}

	void PrimaryAttack( void );
	bool Deploy() override;
	void Holster();
	void WeaponIdle( void );

	const char* MyWModel() override { return "models/v_tripmine.mdl"; }
	int DefaultWeight() override { return TRIPMINE_WEIGHT; }
private:
	unsigned short m_usTripFire;
};

class CSqueak : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_SNARK; }
	bool GetItemInfo(ItemInfo *p) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Holster();
	void WeaponIdle( void );
	int m_fJustThrown;

	virtual const char* GrenadeName() const;
	virtual const char* NestModel() const;
	virtual const char* PModel() const;
	virtual const char* VModel() const;
	virtual int PositionInSlot() const;
	virtual int DefaultGive() const;
	virtual const char* DefaultAmmoName() const;
	virtual const char* EventsFile() const;

	const char* MyWModel() override { return NestModel(); }
	int DefaultWeight() override { return SNARK_WEIGHT; }
private:
	unsigned short m_usSnarkFire;
};

#if FEATURE_DESERT_EAGLE
class CEagle : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_EAGLE; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Holster();
	void Reload( void );
	void ItemPostFrame();
	void WeaponIdle( void );

	void UpdateSpot( void );
	CLaserSpot *m_pEagleLaser;
	int m_fEagleLaserActive;

	const char* MyWModel() override { return "models/w_desert_eagle.mdl"; }
	int DefaultWeight() override { return EAGLE_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
private:
	int m_iShell;

	unsigned short m_usEagle;
};
#endif

#if FEATURE_PIPEWRENCH
class CPipeWrench : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int WeaponId() const override { return WEAPON_PIPEWRENCH; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer(CBasePlayer *pPlayer) override;

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);

	bool Swing(bool fFirst);
	bool Deploy() override;
	void WeaponIdle(void);
	void Holster();
	void BigSwing(void);

	int m_iSwing;
	TraceResult m_trHit;
	int m_iSwingMode;
	float m_flBigSwingStart;

	const char* MyWModel() override { return "models/w_pipe_wrench.mdl"; }
	int DefaultWeight() override { return PIPEWRENCH_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
private:

	unsigned short m_usPWrench;
};
#endif

#if FEATURE_MEDKIT
class CMedkit : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn(void);
	void Precache(void);
	int WeaponId() const override { return WEAPON_MEDKIT; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer(CBasePlayer *pPlayer) override;

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	bool Deploy() override;
	void Holster();
	void Reload( void );
	void WeaponIdle(void);
	bool PlayEmptySound() override;
	bool ShouldWeaponIdle() override { return true; }
	CBaseEntity* FindHealTarget(bool increasedRadius = false);

	const char* MyWModel() override { return "models/w_medkit.mdl"; }
	int DefaultWeight() override { return MEDKIT_WEIGHT; }

	float	m_flSoundDelay;
	float	m_flRechargeTime;
	bool	m_secondaryAttack;

protected:
	bool CanRecharge();
private:
	unsigned short m_usMedkitFire;
};
#endif

#if FEATURE_GRAPPLE
class CBarnacleGrappleTip;

class CBarnacleGrapple : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	enum FireState
	{
		OFF		= 0,
		CHARGE	= 1
	};

	void Precache( void );
	void Spawn( void );
	int WeaponId() const override { return WEAPON_GRAPPLE; }
	void EndAttack( void );

	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer* pPlayer ) override;
	bool Deploy() override;
	void Holster();
	void WeaponIdle( void );
	void PrimaryAttack( void );

	void Fire( Vector vecOrigin, Vector vecDir );

	void CreateEffect( void );
	void UpdateEffect( void );
	void DestroyEffect( void );

	const char* MyWModel() override { return "models/w_bgrap.mdl"; }
	int DefaultWeight() override { return GRAPPLE_WEIGHT; }
private:
	CBarnacleGrappleTip* m_pTip;
#if !CLIENT_DLL
	CBeam* m_pBeam;
#endif

	float m_flShootTime;
	float m_flDamageTime;

	bool m_bGrappling;
	bool m_bMissed;
	bool m_bMomentaryStuck;
};
#endif

#if FEATURE_M249
class CM249 : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int WeaponId() const override { return WEAPON_M249; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer(CBasePlayer *pPlayer) override;

	void PrimaryAttack(void);
	bool Deploy() override;
	void Holster();
	void Reload(void);
	void ItemPostFrame();
	void WeaponIdle(void);

	int m_iShell;
	int m_iLink;
	bool m_bAlternatingEject;

	const char* MyWModel() override { return "models/w_saw.mdl"; }
	int DefaultWeight() override { return M249_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

	int ViewModelBody() override;

	void UpdateTape();
	void UpdateTape(int clip);
	int BodyFromClip();
	int BodyFromClip(int clip);

	int m_iVisibleClip;

private:
	unsigned short m_usM249;
};
#endif

#if FEATURE_SNIPERRIFLE
class CSniperrifle : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int WeaponId() const override { return WEAPON_SNIPERRIFLE; }

	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer(CBasePlayer *pPlayer) override;
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	bool Deploy() override;
	void Holster();
	void Reload(void);
	void WeaponIdle(void);

	const char* MyWModel() override { return "models/w_m40a1.mdl"; }
	int DefaultWeight() override { return SNIPERRIFLE_WEIGHT; }
private:
	unsigned short m_usSniper;
};
#endif

#if FEATURE_DISPLACER
class CDisplacer : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_DISPLACER; }

	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Holster();
	void WeaponIdle( void );

	bool PlayEmptySound( void ) override;

	const char* MyWModel() override { return "models/w_displacer.mdl"; }
	int DefaultWeight() override { return DISPLACER_WEIGHT; }

	bool CanFireDisplacer( int count ) const;

	enum DISPLACER_FIREMODE { FIREMODE_FORWARD = 1, FIREMODE_BACKWARD };

	void ClearSpin( void );
	void EXPORT SpinUp( void );
	void EXPORT Teleport( void );
	void EXPORT Displace( void );
	void LightningEffect( void );
	void ClearBeams( void );
private:
#if !CLIENT_DLL
	CBeam *m_pBeam[3];
#endif
	int m_iFireMode;
	unsigned short m_usDisplacer;
};
#endif

#if FEATURE_SHOCKRIFLE
class CShockrifle : public CHgun
{
public:
	void Spawn(void);
	void Precache(void);
	int WeaponId() const override { return WEAPON_SHOCKRIFLE; }

	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer(CBasePlayer *pPlayer) override;

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	bool Deploy() override;
	void Holster();
	void Reload(void);
	void WeaponIdle(void);
	void CreateChargeEffect(void);
	void EXPORT ClearBeams(void);

	const char* MyWModel() override { return "models/w_shock_rifle.mdl"; }
	int DefaultWeight() override { return SHOCKRIFLE_WEIGHT; }
private:
	unsigned short m_usShockFire;
#if !CLIENT_DLL
	CBeam* m_pBeam[4];
#endif
};
#endif

#if FEATURE_KNIFE
class CKnife : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int WeaponId() const override { return WEAPON_KNIFE; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);

	bool Swing(bool fFirst);
	bool Deploy() override;
	void WeaponIdle();
	void Holster();
	void Stab();

	int m_iSwing;
	TraceResult m_trHit;
	int m_iSwingMode;
	float m_flStabStart;

	const char* MyWModel() override { return "models/w_knife.mdl"; }
	int DefaultWeight() override { return KNIFE_WEIGHT; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
private:
	unsigned short m_usKnife;
};
#endif

#if FEATURE_PENGUIN
class CPenguin : public CSqueak
{
public:
	int WeaponId() const override { return WEAPON_PENGUIN; }
	const char* GrenadeName() const override;
	const char* NestModel() const override;
	const char* PModel() const override;
	const char* VModel() const override;
	int PositionInSlot() const override;
	int DefaultGive() const override;
	const char* DefaultAmmoName() const override;
	const char* EventsFile() const override;
};
#endif

#if FEATURE_SPORELAUNCHER
class CSporelauncher : public CShotgun
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_SPORELAUNCHER; }

	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	bool Deploy() override;
	void Reload( void );
	void WeaponIdle( void );

	const char* MyWModel() override { return "models/w_spore_launcher.mdl"; }
	int DefaultWeight() override { return SPORELAUNCHER_WEIGHT; }

	int m_iSquidSpitSprite;
private:
	unsigned short m_usSporeFire;
};
#endif

#if FEATURE_UZI

class CUzi : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int WeaponId() const override { return WEAPON_UZI; }
	bool GetItemInfo(ItemInfo *p) override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void PrimaryAttack( void );
	bool Deploy() override;
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;

	const char* MyWModel() override { return "models/w_uzi.mdl"; }
	int DefaultWeight() override { return UZI_WEIGHT; }
private:
	unsigned short m_usUzi;
};

#endif

class WeaponRegistrator
{
public:
	WeaponRegistrator(const char* classname, CBasePlayerWeapon* pWeapon);
	WeaponRegistrator() = delete;
	WeaponRegistrator& operator=(const WeaponRegistrator&) = delete;
};

#define LINK_WEAPON_TO_CLASS( mapClassName, DLLClassName )\
namespace detail_##mapClassName {\
	static DLLClassName instance;\
	static WeaponRegistrator registry(#mapClassName, &instance);\
}\
LINK_ENTITY_TO_CLASS( mapClassName, DLLClassName )

#endif // WEAPONS_H
