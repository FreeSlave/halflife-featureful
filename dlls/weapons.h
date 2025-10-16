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

#include "cbase.h"
#include "weapon_ids.h"
#include "mod_features.h"
#include "weapon_animations.h"
#include "weaponinfo.h"
#include "player_items.h"
#include "ammoregistry.h"
#include "cone_degrees.h"
#include "weapon_parameters.h"

#if !CLIENT_DLL
#include "combat.h"
#include "effects.h"
#include "ggrenade.h"
#include "global_models.h"
#endif

class CBasePlayer;
extern int gmsgWeapPickup;

void DeactivateSatchels( CBasePlayer *pOwner );

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

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_GLOCKCLIP_GIVE		17
#define AMMO_357BOX_GIVE		6
#define AMMO_MP5CLIP_GIVE		50
#define AMMO_CHAINBOX_GIVE		200
#define AMMO_M203BOX_GIVE		2
#define AMMO_BUCKSHOTBOX_GIVE	12
#define AMMO_CROSSBOWCLIP_GIVE	5
#define AMMO_RPGCLIP_GIVE		1
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

enum
{
	WEAPONDATA_ALTMODE = (1<<0),
	WEAPONDATA_LASERSPOT = (1<<1),
	WEAPONDATA_BURST_IS_ALT = (1<<2),
	WEAPONDATA_BURSTING = (1<<3),
	WEAPONDATA_SWITCHING_BODY = (1<<4),
	WEAPONDATA_WAS_IN_ALT_MODE_BEFORE_SWITCHING_BODY = (1<<5),
	WEAPONDATA_SWITCHING_MODE = (1<<6),
	WEAPONDATA_SWING_MODE = (1<<7),
	WEAPONDATA_SWING_MODE2 = (1<<8),
};

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
	WeaponParameters params;
};

extern WeaponInfo& AccessWeaponInfo(int id);
extern void SetWeaponParameters();
extern int GetWeaponIdByName(const char* classname);
extern WeaponParameters* AccessWeaponParameters(const char* name);
extern const WeaponParameters& GetWeaponParameters(int id);

class CLaserSpot : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;

	int	ObjectCaps() override { return FCAP_DONT_SAVE; }

	void Suspend( float flSuspendTime );
	void EXPORT Revive();
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;

	static CLaserSpot *CreateSpot( edict_t* pOwner = 0 );
};

class CBasePlayerWeapon : public CBaseAnimating
{
public:
	void SetObjectCollisionBox() override;
	void KeyValue( KeyValueData *pkvd ) override;

#ifndef CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	virtual int WeaponId() const = 0;
	bool IsEnabledInMod() override;
	virtual void PrecacheDefaultModelSounds() {}
	void PrecacheModelSounds();
	virtual bool AddToPlayer( CBasePlayer *pPlayer );	// return true if the item you want the item added to the player inventory
	void EXPORT DestroyItem();
	void EXPORT DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	void EXPORT FallThink ();// when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize();// make a weapon visible and tangible
	void EXPORT AttemptToMaterialize();  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn() override;// copy a weapon
	bool IsLockedByMaster() override;
	bool IsUsefulToDisplayHint(CBaseEntity* pPlayer) override;
	void FallInit();
	void CheckRespawn();
	virtual bool GetItemInfo(ItemInfo *p) = 0;	// returns false if struct not filled out

	virtual WeaponParameters GetDefaultParameters() const = 0;
	const WeaponParameters& MyParameters() const;
	virtual bool CanDeploy();
	virtual bool Deploy()								// returns is deploy was successful
		 { return true; }

	virtual bool CanHolster() { return true; }// can this weapon be put away right now?

	virtual void ItemPreFrame()	{ return; }		// called each frame by the player PreThink

	virtual void Drop();
	virtual void Kill();
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	int ObjectCaps() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void TouchOrUse( CBaseEntity* other );

	static const AmmoType* GetAmmoType( const char* name );

	static ItemInfo ItemInfoArray[ MAX_WEAPONS ];

	CBasePlayer	*m_pPlayer;

	int			iItemPosition() { return ItemInfoArray[ WeaponId() ].iPosition; }
	const char	*pszAmmo1() const { return ItemInfoArray[ WeaponId() ].pszAmmo1; }
	int			iMaxAmmo1()	{
		if (m_iPrimaryAmmoType > 0)
			return g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType);
		return g_AmmoRegistry.GetMaxAmmo(pszAmmo1());
	}
	bool UsesAmmo() const {
		return m_iPrimaryAmmoType > 0 || pszAmmo1() != NULL;
	}
	const char	*pszAmmo2() const { return ItemInfoArray[ WeaponId() ].pszAmmo2; }
	int			iMaxAmmo2()	{
		if (m_iSecondaryAmmoType > 0)
			return g_AmmoRegistry.GetMaxAmmo(m_iSecondaryAmmoType);
		return g_AmmoRegistry.GetMaxAmmo(pszAmmo2());
	}
	bool UsesSecondaryAmmo() const {
		return m_iSecondaryAmmoType > 0 || pszAmmo2() != NULL;
	}

	const char	*pszName()	{ return ItemInfoArray[ WeaponId() ].pszName; }
	int			iMaxClip();
	int			iWeight();
	int			iFlags()		{ return ItemInfoArray[ WeaponId() ].iFlags; }
	const char* pszAmmoEntity() { return ItemInfoArray[ WeaponId() ].pszAmmoEntity; }
	int			iDropAmmo()	{ return ItemInfoArray[ WeaponId() ].iDropAmmo; }

	const char* MyWorldModel();
	void PrecacheWeaponModels();

	bool AddToPlayerDefault( CBasePlayer *pPlayer );
	virtual int AddDuplicate( CBasePlayerWeapon *pItem );

	virtual bool ExtractAmmo( CBasePlayerWeapon *pWeapon );	// TODO: check the return type usage. Return true if you can add ammo to yourself when picked up
	virtual bool ExtractClipAmmo( CBasePlayerWeapon *pWeapon );	// TODO: check the return type usage. Return true if you can add ammo to yourself when picked up

	virtual bool AddWeapon() { ExtractAmmo( this ); return true; }	// Return true if you want to add yourself to the player

	// generic "shared" ammo handlers
	bool AddPrimaryAmmo(int iCount);
	bool AddSecondaryAmmo(int iCount);

	virtual void UpdateItemInfo() {}	// updates HUD state

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
	virtual bool PlayEmptySound(bool altMode);
	virtual void ResetEmptySound();

	void SendWeaponAnim( int iAnim);
	void SendWeaponAnim( int iAnim, int body );

	virtual bool IsUseable();
	bool DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int body = 0, float attackDelay = 0.5f, float idleDelay = 1.0f );
	const char* ViewModelToDeploy(const char* viewModel);
	const char* DetonatorViewModelToDeploy(const char* viewModel);
	bool DefaultReload( int iClipSize, int iAnim, float fDelay, int body = 0 );
	bool DefaultClipReload(int iAnim, float fDelay, int body = 0);
	void PrecachePModel(const char* name);

	virtual void ItemPostFrame();	// called each frame by the player PostThink
	virtual void UpdateInaccuracy() {}
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack() { return; }				// do "+ATTACK"
	virtual void SecondaryAttack() { return; }			// do "+ATTACK2"
	bool CanReload();
	virtual void Reload() { return; }						// do "+RELOAD"
	virtual void WeaponIdle() { return; }					// called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );		// sends hud info to client dll, if things have changed
	virtual void GetWeaponData(weapon_data_t& data) {}
	virtual void SetWeaponData(const weapon_data_t& data) {}
	virtual void ResetWeaponData() {}

	virtual void RetireWeapon();
	virtual bool ShouldWeaponIdle() { return false; }
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

	int	PrimaryAmmoIndex() const;
	int	SecondaryAmmoIndex() const;
	const char* AmmoName(const char* defaultAmmoName);
	const char* SecondaryAmmoName(const char* defaultAmmoName);

	void PrintState();

	CBasePlayerWeapon *MyWeaponPointer() override { return this; }
	virtual bool CanBeDropped() { return true; }
	virtual int ViewModelBody() { return 0; }
	virtual float GetMaxSpeed() { return 0.0f; }
	float GetNextAttackDelay( float delay );

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

	void	SetInitialAmmoAmount();
	int		m_iDefaultAmmo;// how much ammo you get when you pick up this weapon as placed by a level designer.

	string_t m_sMaster;

	// hle time creep vars
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;

	//Hack so deploy animations work when weapon prediction is enabled.
	bool m_ForceSendAnimations;

	void InitMaxClip();
	int m_iMaxClip;
	int m_iClientMaxClip;

	bool UsesClip();
	bool HasAmmoToFire(int ammo = 1);
	bool IsOutOfAmmo();
	void CheckOutOfAmmo();
	void CheckOutOfSecondaryAmmo();
	void SpendAmmo(int ammo = 1);
	bool Emptied();

	void PlayWeaponSoundScript(const WeaponSoundScript& soundScript, float volumeFactor = 1.0f);
};

enum class SwitchModeReason
{
	Regular = 0,
	Reload,
	Holster,
	FirstDeploy
};

class CConfigurableWeapon : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool AddToPlayer(CBasePlayer *pPlayer) override;
	bool Deploy() override;

	bool IsUseable() override;
	void EjectBrassLate();
	void ItemPostFrame() override;
	void UpdateInaccuracy() override;
	void SendScreenShake(const PlayerShake& shake);
	bool SelectAndSendFireAnimation(const WeaponParameters::FireAnimArray& arr);
	bool PerformCooldown(bool altMode);
	void PerformWeaponFire(bool altMode);
	void FireRemaining();
	void ResetBurst();
	void ResetInaccuracy();
	void PrimaryAttack() override;
	void SwitchMode(SwitchModeReason reason = SwitchModeReason::Regular);
	void SecondaryAttack() override;
	bool PerformReload();
	void Reload() override;
	virtual void OnIdleAnimation(int anim) {}
	void SendIdleAnimation();
	void WeaponIdle() override;
	void Holster() override;
	int ViewModelBody() override { return pev->body; }
	void SetBody(int body);

	virtual void NativeAttack(bool altMode) { return; }
	virtual void OnSpendAmmo() { return; }
	virtual void OnEndReload() { return; }
	virtual int GetPlaybackEvent(bool altModeFire) const { return m_usFire; }

	bool PerformDeploy();

	bool InAltMode() const {
		return m_inAltMode;
	}
	void UpdateAutoAim();
	void UpdateSpot();
	void SetZoom(int fov);
	void ResetZoom(SwitchModeReason reason = SwitchModeReason::Regular);
	void KickBack(const WeaponKickBack& kickBack);
	void ApplyMyKickBack(bool altMode);

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;
	void ResetWeaponData() override;

	void EXPORT SwingAgain();
	void EXPORT Smack();
	bool Swing(bool fFirst);
	void BigSwing();

	virtual DamageInfo MeleeDamageInfo() { return DamageInfo{10.0f, DMG_CLUB}; }
	virtual DamageInfo MeleeWindDamageInfo() {
		float damage = Q_max(100.0f, (gpGlobals->time - m_flBigSwingStart) * 10.0f);
		return DamageInfo{damage, DMG_CLUB};
	}

	bool CanRechargeAmmo();
	void UpdateRechargeTime(bool altMode);

	float GetMaxSpeed() override;

#ifndef CLIENT_DLL
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	int PackIParam2();
	void PrecacheCommonEvent();

	bool m_inAltMode;
	bool m_wasEmptyReload;
	bool m_switchingBody;
	bool m_wasInAltModeBeforeSwitchingBody;
	bool m_wasInAltModeBeforeEjectLate;
	bool m_switchingMode;
	bool m_bAlternatingEject;
	bool m_playedFirstDeploy;
	bool m_shouldRestartReloading;

	// Kickback and inaccuracy
	bool m_kickBackDirectionVertical;
	bool m_kickBackDirectionLateral;
	bool m_lastShotWasInAltMode;
	bool m_bDelayFire;
	int m_iShotsFired; // this is for inaccuracy, this doesn't take burst shots into account
	float m_flInaccuracy;
	float m_flLastFire;
	float m_flDecreaseShotsFired;

	// Laser
	CLaserSpot *m_pLaser;
	bool m_bLaserActive;

	// Burst related
	bool m_burstFireIsAlt;
	int m_burstShotsFired;
	float m_burstTime;
	float m_burstSpreadX;
	float m_burstSpreadY;

	// Shotguns
	float m_flPumpTime;

	// models
	int shellModel;
	int shellModel2;
	int shellModelAlternate;
	int shellModelAlternate2;

	// melee
	int m_iSwing;
	TraceResult m_trHit;
	int m_iSwingMode;
	float m_flBigSwingStart;
	bool m_swingIsAltAttack;

	// recharge
	float m_flRechargeTime;

	// charge
	float m_flChargeStart;
	bool m_shouldPlayCooldown;

	// tool
	float m_toolTriggerTime;

	// for max speed
	float m_primaryFireEndTime;
	float m_secondaryFireEndTime;

	// Common event
	int m_usFire;
};

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseDelay
{
public:
	void Precache() override;
	void Spawn() override;
	void Touch( CBaseEntity *pOther ) override;
	void KeyValue( KeyValueData *pkvd ) override;
	bool IsEmpty();
	void SetObjectCollisionBox() override;

	int ObjectCaps() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void TouchOrUse( CBaseEntity* other );

	void EXPORT Kill ();
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
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

bool bIsMultiplayer ();

#if CLIENT_DLL
void LoadVModel ( const char *szViewModel, CBasePlayer *m_pPlayer );
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
