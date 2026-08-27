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
#if !defined(PLAYER_H)
#define PLAYER_H

#include "pm_materials.h"
#include "basemonster.h"
#include "objecthint_spec.h"
#include "player_templates.h"
#include "com_model.h"
#include <cstdint>
#include <vector>

class CRope;

#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

enum
{
	INVENTORY_ITEM_NO_CHANGE = -2,
	INVENTORY_ITEM_NONE_GIVEN_MAXITEMS = -1,
	INVENTORY_ITEM_NONE_GIVEN_MAXCOUNT = 0,
	INVENTORY_ITEM_GIVEN = 1,
	INVENTORY_ITEM_GIVEN_OVERFLOW = 2,
	INVENTORY_ITEM_COUNT_CHANGED = 3
};

#define STRIP_WEAPONS_ONLY 0
#define STRIP_SUIT 1
#define STRIP_SUITLIGHT 2
#define STRIP_LONGJUMP 4
#define STRIP_INVENTORY 8
#define STRIP_CANISTERS 16
#define STRIP_ALL_ITEMS (STRIP_SUIT | STRIP_SUITLIGHT | STRIP_LONGJUMP | STRIP_INVENTORY | STRIP_CANISTERS)

#define SF_DISPLACER_TARGET_DISABLED 1

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER		( 1<<0 )
#define PFLAG_ONSWING		( 1<<0 )
#define PFLAG_ONTRAIN		( 1<<1 )
#define PFLAG_ONBARNACLE	( 1<<2 )
#define PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define PFLAG_USING		( 1<<4 )		// Using a continuous entity
#define PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.
#define	PFLAG_LATCHING		( 1<<6 )	// Player is latching to a target
#define	PFLAG_ATTACHED		( 1<<7 )	// Player is attached by a barnacle tongue tip


#define PFLAG_ONROPE		( 1<<8 )
//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time
#define MAX_JOURNAL_RECORDS 4

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define TEAM_NAME_LENGTH	16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_GRAPPLE,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END
};

#define CHAT_INTERVAL 1.0f

#define ARMOR_RATIO	0.2	// Armor Takes 80% of the damage

// trigger_camera related player flags
#define PLAYER_CAMERA_INVULNERABLE (1 << 0)

// this is trigger_camera flag, need to have it here
#define SF_CAMERA_STOP_BY_PLAYER_INPUT_USE (1 << 25)

CBaseEntity *FindEntityForward(CBaseEntity *pMe);

class CBasePlayer : public CBaseMonster
{
public:
	// Spectator camera
	void	Observer_FindNextPlayer( bool bReverse );
	void	Observer_HandleButtons();
	void	Observer_SetMode( int iMode );
	void	Observer_CheckTarget();
	void	Observer_CheckProperties();
	EHANDLE	m_hObserverTarget;
	float	m_flNextObserverInput;
	int		m_iObserverWeapon;	// weapon of current tracked target
	int		m_iObserverLastMode;// last used observer mode
	int		IsObserver() { return pev->iuser1; };

	int					random_seed;    // See that is shared between client & server for shared weapons code

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;

	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;

	edict_t				*m_pentSndLast;			// last sound entity to modify player room type
	int					m_SndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity
	int					m_ClientSndRoomtype;

	float				m_flFallVelocity;

	int					m_antidotes;
	int					m_iClientAntidotes;
	float				m_antidoteProtectionTime;

	int					m_radcans;
	int					m_iClientRadcans;
	float				m_radiationProtectionTime;

	int					m_adrenalines;
	int					m_iClientAdrenalines;
	float				m_flNextRevive;
	bool				m_preventAdrenalineRevival;

	bool				m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command

	// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
										// the hude via the DAMAGE message
	bool				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	bool				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	bool				m_fWeapon;				// Set this to false to force a reset of the current weapon HUD info

	EHANDLE				m_hTankControls;				// the tank which the player is currently controlling,  NULL if no tank
	EHANDLE				m_hViewEntity;			// The view entity being used, or null if the player is using itself as the view entity
	bool				m_bResetViewEntity;		//True if the player's view needs to be set back to the view entity
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	bool			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true.
	bool			m_fLongJump; // does this player have the longjump module?

	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientMaxHealth;
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iClientMaxBattery;
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV

	// usable player items 
	CBasePlayerWeapon *m_rgpPlayerWeapons[MAX_WEAPONS];
	CBasePlayerWeapon *m_pActiveItem;
	CBasePlayerWeapon *m_pClientActiveItem;  // client version of the active item
	CBasePlayerWeapon *m_pLastItem;

	std::uint64_t m_WeaponBits;

	//Not saved, used to update client.
	std::uint64_t m_ClientWeaponBits;

	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_TYPES];
	int	m_rgAmmoLast[MAX_AMMO_TYPES];

	Vector				m_vecAutoAim;
	bool				m_fOnTarget;
	int					m_iDeaths;
	float				m_flRespawnTimer;	// used in PlayerDeathThink() to make sure players can always respawn

	float m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	void Spawn() override;

	//virtual void Think();
	virtual void Jump();
	virtual void Duck();
	virtual void PreThink();
	virtual void PostThink();
	Vector GetGunPosition() override;
	int TakeHealth(CBaseEntity *pHealer, float flHealth, int healType ) override;
	void SetHealth(int health, bool allowOverheal = false);
	void SetMaxHealth(int maxHealth, bool clampValue = true);
	int TakeArmor(CBaseEntity *pCharger, float flArmor, int flags = 0) override;
	int MaxArmor();
	void SetMaxArmor(int maxArmor, bool clampValue = true);
	void SetArmor(int armor, bool allowOvercharge = false);
	float ArmorStrength();
	bool IsInvulnerable();
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	TakeDamageResult TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo) override;
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	Vector BodyTarget( const Vector &posSrc ) override { return Center() + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); }		// position to shoot at
	bool IsAlive() override { return IsFullyAlive(); }
	bool IsFullyAlive() override { return CBaseMonster::IsFullyAlive() && !IsObserver(); }
	bool ShouldFadeOnDeath() override { return false; }
	bool IsPlayer() override { return true; }			// Spectators should return false for this, they aren't "players" as far as game logic is concerned

	bool IsNetClient() override { return true; }		// Bots should return false for this, they can't receive NET messages
															// Spectators should return true for this
	const char *TeamID() override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	void RenewItems();
	void PackDeadPlayerItems();
	void RemoveAllItems( int stripFlags );
	void RemoveAllWeapons();
	void RemoveAllAmmo();
	bool SwitchWeapon( CBasePlayerWeapon *pWeapon );
	bool SwitchToBestWeapon();

	void SetWeaponBit(int id) {
		m_WeaponBits |= 1ULL << id;
	}
	void ClearWeaponBit(int id) {
		m_WeaponBits &= ~(1ULL << id);
	}
	bool HasWeaponBit(int id) {
		return (m_WeaponBits & (1ULL << id)) != 0;
	}

	bool HasSuit() const
	{
		return (m_iItemsBits & PLAYER_ITEM_SUIT) != 0;
	}
	bool HasFlashlight() const
	{
		return (m_iItemsBits & PLAYER_ITEM_FLASHLIGHT) != 0;
	}
	bool HasNVG() const
	{
		return (m_iItemsBits & PLAYER_ITEM_NIGHTVISION) != 0;
	}
	bool HasSuitLight() const {
		return HasFlashlight() || HasNVG();
	}
	void RemoveSuitLight();

	void SetJustSuit() {
		m_iItemsBits |= PLAYER_ITEM_SUIT;
	}
	void SetFlashlight() {
		m_iItemsBits |= PLAYER_ITEM_FLASHLIGHT;
	}
	void SetFlashlightOnly();
	void RemoveFlashlight();
	void SetNVG() {
		m_iItemsBits |= PLAYER_ITEM_NIGHTVISION;
	}
	void SetNVGOnly();
	void RemoveNVG();

	void SetSuitAndDefaultLight();
	void SetDefaultLight();
	void SetLongjump(bool enabled);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData();
	void GatherAndSendObjectHints();
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	int ObjectCaps() override { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Precache() override;
	bool IsOnLadder();
	bool FlashlightIsOn() { return FBitSet(pev->effects, EF_DIMLIGHT) || m_fFlashlightON; }
	bool NVGIsOn() { return m_fNVGisON; }
	bool SuitLightIsOn() { return FlashlightIsOn() || NVGIsOn(); }
	void SuitLightTurnOn();
	void SuitLightTurnOff( bool playOffSound = true );
	void UpdateSuitLightBattery( bool on );
	void FlashlightToggle();
	void FlashlightTurnOn();
	void FlashlightTurnOff( bool playOffSound = true );
	void NVGToggle();
	void NVGTurnOn();
	void NVGTurnOff( bool playOffSound = true );

	void UpdatePlayerSound ();
	void DeathSound() override;

	int DefaultClassify() override;
	int Classify() override;
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands();
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam();
	void StartObserver( Vector vecPosition, Vector vecViewAngle );
	void StopObserver();

	void AddPoints( int score, bool bAllowNegativeScore ) override;
	void AddPointsToTeam( int score, bool bAllowNegativeScore ) override;
	void AddFloatPoints( float score, bool bAllowNegativeScore ) override;
	int AddPlayerItem( CBasePlayerWeapon *pItem ) override;
	bool RemovePlayerItem( CBasePlayerWeapon *pItem, bool bCallHoster );
	void DropPlayerItem ( const char *pszItemName );
	void DropPlayerItemById( int iId );
	void DropAmmo(bool secondary);
	bool HasPlayerItem( CBasePlayerWeapon *pCheckItem );
	bool HasNamedPlayerItem( const char *pszItemName );
	CBasePlayerWeapon* GetWeaponByName( const char *pszItemName );
	bool HasWeapons();// do I have ANY weapons?
	void SendCurWeaponClear();
	void SendCurWeaponDead();
	void SelectPrevItem( int iItem );
	void SelectLastItem();
	void SelectItem(const char *pstr);
	void ItemPreFrame();
	void ItemPostFrame();
	void GiveNamedItem( const char *szName, int spawnFlags = 0 );
	void EnableControl(bool fControl);

	int  GiveAmmo( int iAmount, const char *szName ) override;
	void RemoveAmmo( int iAmount, const char *szName );
	void SendAmmoUpdate();

	void WaterMove();
	void EXPORT PlayerDeathThink();
	std::pair<CBaseEntity*, const ObjectHintSpec*> GetInteractiveEntity(std::vector<std::pair<CBaseEntity*, const ObjectHintSpec*>>* hintedEntities = nullptr);
	void PlayerUse();
	void ReleaseTank();

	bool CanPlaySuitSentences();
	void CheckSuitUpdate();
	void SetSuitUpdate( const char *name, float iNoRepeat );
	void SetPickupSuitUpdate( CBaseEntity* pPickupEntity, const char *name, float iNoRepeat );
	void UpdateGeigerCounter();
	void CheckTimeBasedDamage();

	bool FBecomeProne() override;
	void BarnacleVictimBitten( entvars_t *pevBarnacle ) override;
	void BarnacleVictimReleased() override;
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	void ClearAmmoByIndex(int iAmmoIndex);
	int Illumination() override;

	void ResetAutoaim();
	Vector GetAutoaimVector( float flDelta  );
	Vector GetAutoaimVectorFromPoint( const Vector& vecSrc,float flDelta  );
	Vector AutoaimDeflection( const Vector &vecSrc, float flDist, float flDelta  );

	void MarkClientValuesForUpdate();
	void ForceClientDllUpdate();  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames();

	bool HandleDoorBlockage(CBaseEntity* pDoor) override;
	bool ShouldCollideWithCorpses() override;

	void SetMovementMode();
	float GetBaseMaxSpeed();
	bool HasCustomBaseMaxSpeed();
	void RecruitFollowers();
	void DisbandFollowers();
	void MakeStartFollowing(CFollowingMonster* pMonster);
	void MakeStopFollowing(CFollowingMonster* pMonster);

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?

	// Player ID
	void InitStatusBar();
	void UpdateStatusBar();

	void InsertWeaponById( CBasePlayerWeapon* pItem );
	CBasePlayerWeapon* WeaponById( int id );

	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	int m_lastSeenEntityIndex;
	int m_lastSeenHealth;
	int m_lastSeenArmor;
	float m_lastSeenTime;

	void SetPrefsFromUserinfo( char *infobuffer );

	float m_flNextChatTime;

	int m_iAutoWepSwitch;

	Vector m_vecLastViewAngles;
	float m_flNextRespawnMessageTime;
	Vector m_DisplacerReturn;
	int m_DisplacerSndRoomtype;
	bool	m_fInXen;
	bool	m_fNVGisON;
	bool	m_fFlashlightON;
	bool	m_fFlashlightFlicker;
	float	m_flNextFlashlightFlick;
	friend class CDisplacer;
	friend class CTriggerXenReturn;

private:
	enum {
		NoAmmoDrop,
		DropAllAmmo,
		DropAmmoFair
	};

	void DropPlayerItemImpl(CBasePlayerWeapon* pWeapon, int dropType = DropAmmoFair, float speed = 400);

public:
	short m_movementState; // no need to save

	bool m_bSentMessages;
	bool m_bSentVisibilityMessages;

	bool m_bIsClimbing;
	float m_flLastClimbTime;
	EHANDLE m_hRope;
	bool IsOnRope()
	{
		return FBitSet(m_afPhysicsFlags, PFLAG_ONROPE);
	}
	void SetRope( CBaseEntity *pRope )
	{
		m_hRope = pRope;
	}
	void SetOnRopeState(bool onRope);
	CRope* GetRope();
	void LetGoRope(float delay = 2.0f);
	bool SetClosestOriginOnRope(const Vector& vecPos);
	void HandleRopePhysics(CRope* pRope);

	int m_iItemsBits;
	int m_iClientItemsBits;

	BYTE m_timeBasedDmgModifiers[CDMG_TIMEBASED];

	bool m_settingsLoaded;
	bool m_buddha;
	short m_iSatchelControl;
	short m_iPreferNewGrenadePhysics;

	int m_suppressedCapabilities;
	int m_suppressedCapabilitiesClient;
	float m_maxSpeedOverride;
	bool m_maxSpeedOverrideIsAbsolute;

	bool m_movementPrevented;
	float m_movementPreventedTime;

	float m_armorStrength;

	void SetLoopedMp3(string_t loopedMp3);
	string_t m_loopedMp3;

	string_t m_inventoryItems[MAX_INVENTORY_ITEMS];
	short m_inventoryItemCounts[MAX_INVENTORY_ITEMS];
	int FindSlotForItem(string_t item, bool allowOverflow = false, int* result = nullptr);
	bool CanHaveIntenvoryItem(string_t item, bool allowOverflow = false);
	int GiveInventoryItem(string_t item, int count, bool allowOverflow = false);
	int SetInventoryItem(string_t item, int count, bool allowOverflow = false);
	bool RemoveInventoryItem(string_t item, int count);
	void RemoveAllInventoryItems();
	bool HasInventoryItem(string_t item);
	int InventoryItemIndex(string_t item);

	void RemoveCanisters();

	EHANDLE m_camera;
	int m_cameraFlags;

	float m_spriteHintTimeCheck;

	float m_flEjectBrass;
	int m_iLastZoom;
	bool m_bResumeZoom;

	bool m_needSatchelRecheck;

	bool m_forceCollideWithCorpses;
	bool m_hidePickups;

	void NotifyPickup(const char* pickupName);

	bool AddJournalRecord(string_t section, string_t record);
	string_t m_journalSections[MAX_JOURNAL_RECORDS];
	string_t m_journalRecords[MAX_JOURNAL_RECORDS];

	bool AssignPlayerTemplate(string_t templateName);
	bool ApplyPlayerTemplate(string_t templateName);
	void SendPlayerTemplateData();
	bool CanHaveItem(CBaseEntity* pEntity);
	string_t m_playerTemplateName;
	const PlayerTemplate* m_playerTemplate;

	int m_ClientVolume;
	float m_NextClientVolumeUpdate;

	float m_fadeStarted;
	float m_fadeDuration;
	float m_fadeHoldTime;
	int m_fadeColor;
	short m_fadeAlpha;
	short m_fadeFlags;

	EHANDLE m_messageBoxEnts[MAX_MESSAGE_BOXES];
	Vector m_messageBoxOrigins[MAX_MESSAGE_BOXES];
	float m_messageBoxDistances[MAX_MESSAGE_BOXES];
	void RemoveMessageBoxGaps();
	bool AddMessageBox(CBaseEntity* pMessageBoxEnt, const Vector& origin, float distance);
	bool CloseMessageBox(int messageBoxId);
	void ClearMessageBoxByIndex(int i);

	int m_ToolSignalBits;
	int m_ToolStateBits;
	int m_ClientToolStateBits;
	int m_ToolReadyBits;
	int m_ToolUnalignedBits;
	int m_ClientToolUnalignedBits;
	edict_t* m_UseToolTriggers[32];

	int GetMaxAmmo(int ammoIndex);
	int m_maxAmmoOverride[MAX_AMMO_TYPES];

	bool m_bunnyhop;
};

extern int gmsgHudText;
extern bool gInitHUD;

extern bool g_PlayerFullyInitialized[MAX_CLIENTS];

#endif // PLAYER_H
