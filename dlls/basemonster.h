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
#pragma once
#if !defined(BASEMONSTER_H)
#define BASEMONSTER_H

#include "cbase.h"
#include "ent_templates.h"

class CFollowingMonster;
class CTalkMonster;
class CDeadMonster;

struct CheckMeleeAttackParams
{
	float distance = 64.0f;
	float dot = 0.7f;
};

struct TraceHullAttackParams
{
	float distance = 70.0f;
	optional<float> height;
	Vector punchAngle = g_vecZero;
	float knockForward = 0.0f;
	float knockRight = 0.0f;
	float knockUp = 0.0f;
	bool knockPlayerOnly = false;
	bool skipAllies = false;
	bool useAimVectors = true;
	bool allowRetry = true;
	DamageInfo damageInfo{0.0f, DMG_SLASH};
	bool spawnBlood = false;
	optional<Vector> bloodOrigin;
	float verticalDistance = 0.0f;

	const char* hitSoundScript = nullptr;
	const char* missSoundScript = nullptr;
};

struct TouchAttackParams
{
	DamageInfo damageInfo{0.0f, DMG_SLASH};
};

//
// generic Monster
//
class CBaseMonster : public CBaseToggle
{
private:
	int m_afConditions;

public:
	typedef enum
	{
		SCRIPT_PLAYING = 0,		// Playing the sequence
		SCRIPT_WAIT,				// Waiting on everyone in the script to be ready
		SCRIPT_CLEANUP,					// Cancelling the script / cleaning up
		SCRIPT_WALK_TO_MARK,
		SCRIPT_RUN_TO_MARK
	} SCRIPTSTATE;

	// these fields have been added in the process of reworking the state machine. (sjb)
	EHANDLE m_hEnemy;		 // the entity that the monster is fighting.
	EHANDLE m_hTargetEnt;	 // the entity that the monster is trying to reach. In scripts the entity that the monster should turn to.
	EHANDLE m_hMoveGoalEnt; // the entity the monster is going to (in scripts)
	EHANDLE m_hOldEnemy[MAX_OLD_ENEMIES];
	Vector m_vecOldEnemy[MAX_OLD_ENEMIES];

	float m_flFieldOfView;// width of monster's field of view ( dot product )
	float m_flWaitFinished;// if we're told to wait, this is the time that the wait will be over.
	float m_flMoveWaitFinished;

	Activity m_Activity;// what the monster is doing (animation)
	Activity m_IdealActivity;// monster should switch to this activity

	int m_LastHitGroup; // the last body region that took damage

	MONSTERSTATE m_MonsterState;// monster's current state
	MONSTERSTATE m_IdealMonsterState;// monster should change to this state

	int m_iTaskStatus;
	Schedule_t *m_pSchedule;
	int m_iScheduleIndex;

	WayPoint_t m_Route[ROUTE_SIZE];	// Positions of movement
	int m_movementGoal;			// Goal that defines route
	int m_iRouteIndex;			// index into m_Route[]
	float m_moveWaitTime;			// How long I should wait for something to move

	Vector m_vecMoveGoal; // kept around for node graph moves, so we know our ultimate goal
	Activity m_movementActivity;	// When moving, set this activity

	int m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
	int m_afSoundTypes;

	Vector m_vecLastPosition;// monster sometimes wants to return to where it started after an operation.

	int m_iHintNode; // this is the hint node that the monster is moving towards or performing active idle on.

	int m_afMemory;

	Vector m_vecEnemyLKP;// last known position of enemy. (enemy's origin)

	int m_cAmmoLoaded;		// how much ammo is in the weapon (used to trigger reload anim sequences)

	int m_afCapability;// tells us what a monster can/can't do.

	float m_flNextAttack;		// cannot attack again until this time

	int m_bitsDamageType;	// what types of damage has monster (player) taken
	BYTE m_rgbTimeBasedDamage[CDMG_TIMEBASED];
										// time based damage counters, decr. 1 per 2 seconds
	int m_bloodColor;		// color of blood particless

	int m_failSchedule;				// Schedule type to choose if current schedule fails

	float m_flHungryTime;// set this is a future time to stop the monster from eating for a while. 

	float m_flDistTooFar;	// if enemy farther away than this, bits_COND_ENEMY_TOOFAR set in CheckEnemy
	float m_flDistLook;	// distance monster sees (Default 2048)

	short m_iTriggerCondition;// for scripted AI, this is the condition that will cause the activation of the monster's TriggerTarget
	short m_iTriggerAltCondition;
	string_t m_iszTriggerTarget;// name of target that should be fired. 

	Vector m_HackedGunPos;	// HACK until we can query end of gun

	// Scripted sequence Info
	SCRIPTSTATE m_scriptState;		// internal cinematic state
	CCineMonster *m_pCine;
	
	int m_iClass;
	string_t m_gibModel;

	bool m_reverseRelationship;

	float m_flLastYawTime;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData *pkvd ) override;
	void Activate() override;
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	void SetMySize(const Vector& vecMin, const Vector& vecMax);
	void SetMySize();
	void SetMySquadCapabilities(int defaultCaps = 0);
	void SetMyCanOpenDoors(bool enable);

	// monster use function
	void EXPORT MonsterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CorpseUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// overrideable Monster member functions
	int BloodColor() override { return m_bloodColor; }

	CBaseMonster *MyMonsterPointer() override { return this; }
	virtual CFollowingMonster* MyFollowingMonsterPointer() { return NULL; }
	virtual CTalkMonster* MyTalkMonsterPointer() { return NULL; }

	virtual void Look( int iDistance );// basic sight function for monsters
	virtual void RunAI();// core ai function!
	void Listen();

	bool IsAlive() override { return ( pev->deadflag != DEAD_DEAD ); }
	bool IsFullyAlive() override;
	virtual bool ShouldFadeOnDeath();
	bool ShouldCollide(CBaseEntity *pOther) override;
	bool ShouldCollideWithCorpses() override;

	// Basic Monster AI functions
	virtual float ChangeYaw( int yawSpeed );
	float VecToYaw( Vector vecDir );
	float FlYawDiff();

	float DamageForce( float damage );

	// stuff written for new state machine
	virtual void MonsterThink();
	void EXPORT CallMonsterThink() { this->MonsterThink(); }
	int IRelationship( CBaseEntity *pTarget ) override;
	int IDefaultRelationship(CBaseEntity *pTarget );
	int IDefaultRelationship( int classify );
	
	static int IDefaultRelationship(int classify1, int classify2);
	
	virtual void MonsterInit();
	virtual void MonsterInitDead();	// Call after animation/pose is set up
	void InitRandomSeeds();
	virtual void BecomeDead();
	void EXPORT CorpseFallThink();

	void EXPORT MonsterInitThink();
	virtual void StartMonster();
	virtual CBaseEntity *BestVisibleEnemy();// finds best visible enemy for attack
	virtual bool FInViewCone( CBaseEntity *pEntity );// see if pEntity is in monster's view cone
	virtual bool FInViewCone( Vector *pOrigin );// see if given location is in monster's view cone
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;

	virtual int CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );// check validity of a straight move through space
	virtual void Move( float flInterval = 0.1 );
	virtual void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	virtual bool ShouldAdvanceRoute( float flWaypointDist );

	virtual Activity GetStoppedActivity() { return ACT_IDLE; }
	virtual void Stop() { m_IdealActivity = GetStoppedActivity(); }

	// This will stop animation until you call ResetSequenceInfo() at some point in the future
	inline void StopAnimation() { pev->framerate = 0; }

	// these functions will survey conditions and set appropriate conditions bits for attack types.
	virtual bool CheckRangeAttack1( float flDot, float flDist );
	virtual bool CheckRangeAttack2( float flDot, float flDist );
	bool CheckMeleeAttackImpl(float flDot, float flDist, const CheckMeleeAttackParams& defaults, bool meleeAttack2);
	virtual bool CheckMeleeAttack1( float flDot, float flDist );
	virtual bool CheckMeleeAttack2( float flDot, float flDist );
	virtual bool WantsToGetCloseToEnemy();

	bool FHaveSchedule();
	bool FScheduleValid();
	void ClearSchedule();
	bool FScheduleDone();
	void ChangeSchedule(Schedule_t *pNewSchedule , bool isSuggested = false);
	virtual void OnChangeSchedule( Schedule_t *pNewSchedule ) {}
	void NextScheduledTask();
	Schedule_t *ScheduleInList( const char *pName, Schedule_t **pList, int listCount );

	virtual Schedule_t *ScheduleFromName( const char *pName );
	static Schedule_t *m_scheduleList[];

	bool ShouldGetIdealState();
	void MaintainSchedule();
	virtual void StartTask( Task_t *pTask );
	virtual void RunTask( Task_t *pTask );
	virtual Schedule_t *GetScheduleOfType( int Type );
	virtual Schedule_t *GetSchedule();
	Schedule_t* GetFreeroamSchedule();
	Schedule_t* GetSuggestedSchedule();
	bool SuggestSchedule(int schedule, CBaseEntity *spotEntity = 0, float minDist = 0.0f, float maxDist = 0.0f, int flags = 0);
	float SuggestedMinDist(float defaultValue) const;
	float SuggestedMaxDist(float defaultValue) const;
	bool CalcSuggestedSpot(Vector *outVec, Vector *viewOffset = NULL);
	Activity GetSuggestedMovementActivity(Activity defaultActivity);
	void ClearSuggestedSchedule();

	virtual void ScheduleChange() {}
	// virtual int CanPlaySequence() { return ((m_pCine == NULL) && (m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE)); }
	virtual bool CanPlaySequence( int interruptFlags );
	virtual bool CanPlaySentence( bool fDisregardState ) { return m_MonsterState == MONSTERSTATE_SCRIPT ? IsAlive() : IsFullyAlive(); }
	bool IsAllowedToSpeak() override { return IsAlive(); }

	Task_t *GetTask();
	virtual MONSTERSTATE GetIdealState();
	virtual void SetActivity( Activity NewActivity );
	void SetSequenceByName( const char *szSequence );
	void SetState( MONSTERSTATE State );
	static const char* MonsterStateDisplayString(MONSTERSTATE monsterState);
	virtual void ReportAIState( ALERT_TYPE level );
	void ReportFireAnimEvent(int event);

	void CheckAttacks( CBaseEntity *pTarget, float flDist, float flMeleeDist );
	virtual bool CheckEnemy( CBaseEntity *pEnemy );
	virtual bool PerceiveEnemyAsOccluded(CBaseEntity* pEnemy, CBaseEntity* pOccluder) { return true; }
	void SetEnemy( CBaseEntity* pEnemy );
	void PushEnemy(CBaseEntity *pEnemy, const Vector &vecLastKnownPos );
	bool PopEnemy();

	bool FGetNodeRoute( Vector vecDest, int goalMoveFlag = 0 );
	
	inline void TaskComplete() { if ( !HasConditions( bits_COND_TASK_FAILED ) ) m_iTaskStatus = TASKSTATUS_COMPLETE; }
	void MovementComplete();
	inline void TaskFail( const char* reason = NULL ) { SetConditions( bits_COND_TASK_FAILED ); taskFailReason = reason; }
	inline void TaskBegin() { m_iTaskStatus = TASKSTATUS_RUNNING; }
	bool TaskIsRunning();
	inline bool TaskIsComplete() { return ( m_iTaskStatus == TASKSTATUS_COMPLETE ); }
	inline bool MovementIsComplete() { return ( m_movementGoal == MOVEGOAL_NONE ); }

	int IScheduleFlags();
	bool FRefreshRoute( int buildRouteFlags = 0 );
	bool FRouteClear();
	void RouteSimplify( CBaseEntity *pTargetEnt );
	void AdvanceRoute( float distance );
	int FTriangulate(const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApexes, int n = 1, int tries = 8, bool recursive = false);
	Vector FTriangulateToNearest(const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector& apex);
	void MakeIdealYaw( Vector vecTarget );
	virtual void SetYawSpeed() { return; };// allows different yaw_speeds for each activity
	bool BuildRoute( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget, int buildRouteFlags = 0 );
	virtual bool BuildNearestRoute( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist );
	int RouteClassify( int iMoveFlag );
	void InsertWaypoint( Vector vecLocation, int afMoveFlags );

	bool FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset, float minDist, float maxDist, int flags );
	bool FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset );
	bool FindLateralSpotAway( const Vector &vecThreat, float minDist, float maxDist, int flags );
	bool FindStraightSpotAway( const Vector &vecThreat, float minDist, float maxDist, int flags );
	bool FindSpotAway(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist, int flags, const char* displayName);
	bool FindCover(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist, int flags);
	bool FindCover(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist);
	bool FindSpotAway(Vector vecThreat, float flMinDist, float flMaxDist, int flags);
	virtual bool FValidateCover( const Vector &vecCoverLocation ) { return true; }
	virtual float CoverRadius() { return 784; } // Default cover radius

	virtual bool FCanCheckAttacks();
	virtual void CheckAmmo() { return; };
	virtual int IgnoreConditions();

	inline void SetConditions( int iConditions ) { m_afConditions |= iConditions; }
	inline void ClearConditions( int iConditions ) { m_afConditions &= ~iConditions; }
	inline bool HasConditions( int iConditions ) { if ( m_afConditions & iConditions ) return true; return false; }
	inline bool HasAllConditions( int iConditions ) { if ( (m_afConditions & iConditions) == iConditions ) return true; return false; }

	virtual bool FValidateHintType( short sHint );
	int FindHintNode();
	virtual bool FCanActiveIdle();
	void SetTurnActivity();
	float FLSoundVolume( CSound *pSound );

	bool MoveToNode( Activity movementAct, float waitTime, const Vector &goal );
	bool MoveToTarget( Activity movementAct, float waitTime, bool closest = false );
	bool MoveToLocation( Activity movementAct, float waitTime, const Vector &goal, int buildRouteFlags = 0 );
	bool MoveToLocationClosest( Activity movementAct, float waitTime, const Vector &goal, int buildRouteFlags = 0 );
	bool MoveToEnemy( Activity movementAct, float waitTime );

	// Returns the time when the door will be open
	float OpenDoorAndWait( entvars_t *pevDoor );

	int ISoundMask();
	virtual int DefaultISoundMask();
	virtual CSound* PBestSound();
	virtual CSound* PBestScent();
	virtual float HearingSensitivity() { return 1.0; }

	bool FBecomeProne() override;
	virtual void BarnacleVictimBitten( entvars_t *pevBarnacle );
	virtual void BarnacleVictimReleased();

	void SetEyePosition();

	bool FShouldEat();// see if a monster is 'hungry'
	void Eat( float flFullDuration );// make the monster 'full' for a while.

	void SetTouchAttackFromTemplate(TouchAttackParams& params);
	void PerformTouchAttack(const TouchAttackParams& params, CBaseEntity* pOther);
	bool SetTraceHullAttackParamsFromTemplate(int eventIndex, TraceHullAttackParams& params);
	CBaseEntity *CheckTraceHullAttack(const TraceHullAttackParams& params, float height, const Vector& aimAngles);
	CBaseEntity* PerformTraceHullAttack(const TraceHullAttackParams& params);
	bool FacingIdeal();

	bool FCheckAITrigger();// checks and, if necessary, fires the monster's trigger target.
	bool FCheckAITrigger( short condition );// checks and, if necessary, fires the monster's trigger target.

	bool BBoxFlat();

	// PrescheduleThink 
	virtual void PrescheduleThink() { return; }

	bool GetEnemy( bool forcePopping );
	void MakeDamageBloodDecal( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir );
	virtual float HeadHitGroupDamageMultiplier();
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;

	// combat functions
	float UpdateTarget( entvars_t *pevTarget );
	virtual Activity GetDeathActivity();
	Activity GetSmallFlinchActivity();
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	virtual void OnDying(bool gibbed);
	virtual void GibMonster();
	void UpdateOnRemove() override;
	bool ShouldGibMonster( int iGib );
	void CallGibMonster();
	virtual bool HasHumanGibs();
	virtual bool HasAlienGibs();
	virtual void FadeMonster();	// Called instead of GibMonster() when gibs are disabled

	Vector ShootAtEnemy( const Vector &shootOrigin );
	Vector SpitAtEnemy(const Vector& vecSpitOrigin, float dirRandomDeviation = 0.05f, float* distance = 0 );
	Vector BodyTarget( const Vector &posSrc ) override { return Center() * 0.75 + EyePosition() * 0.25; }		// position to shoot at

	virtual	Vector GetGunPosition();

	int TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType ) override;
	TakeDamageResult TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo) override;
	void ReactToDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, TakeDamageResult& takeDamageResult);
	void PainReaction(const DamageInfo& damageInfo);
	TakeDamageResult DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo );

	void RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, int iClassIgnore );
	void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, int iClassIgnore );
	bool IsMoving() override { return m_movementGoal != MOVEGOAL_NONE; }

	void RouteClear();
	void RouteNew();

	virtual void DeathSound() { return; }
	virtual void AlertSound() { return; }
	virtual void IdleSound() { return; }
	virtual PainSoundRule DefaultPainSoundRule() { return PainSoundRule{}; }
	virtual void PainSound() { return; }

	virtual void StopFollowing( bool clearSchedule, bool saySentence = true ) {}

	inline void Remember( int iMemory ) { m_afMemory |= iMemory; }
	inline void Forget( int iMemory ) { m_afMemory &= ~iMemory; }
	inline bool HasMemory( int iMemory ) { if ( m_afMemory & iMemory ) return true; return false; }
	inline bool HasAllMemories( int iMemory ) { if ( (m_afMemory & iMemory) == iMemory ) return true; return false; }

	bool ExitScriptedSequence();
	bool CineCleanup();
	void SetScriptedMoveGoal(CBaseEntity* pEntity);
	CBaseEntity* ScriptedMoveGoal();

	Schedule_t* StartPatrol( CBaseEntity* path );
	CBaseEntity* DropItem ( const char *pszItemName, const Vector &vecPos, const Vector &vecAng );// drop an item.
	void FixupDropItemPosition(Vector& pos);

	bool CalcRatio(CBaseEntity* pLocus, float* outResult) override
	{
		if (IsFullyAlive())
			*outResult = pev->health / pev->max_health;
		else
			*outResult = 0;
		return true;
	}

	const char* MyNonDefaultGibModel();
	const Visual* MyGibVisual();
	int PrecacheMyGibModel( const char* model = nullptr );
	void SetMyBloodColor( int defaultBloodColor );
	void SetMyFieldOfView(const float defaultFieldOfView );

	int Classify() override;
	int DefaultClassify() override;
	void FixChildClassify(CBaseMonster* pChild);
	virtual const char* ReverseRelationshipModel() { return nullptr; }

	virtual CDeadMonster* MyDeadMonsterPointer() {return nullptr;}

	virtual const char* DefaultGibModel();
	const char* GibModel();
	virtual int DefaultGibCount();
	int GibCount();

	bool IsAlienMonster() override;

	virtual Vector DefaultMinHullSize();
	virtual Vector DefaultMaxHullSize();

	int SizeForGrapple() override;

	// Allows to set a head via monstermaker before spawn
	virtual void SetHead(int head) {}

	virtual bool HandleBlocker(CBaseEntity* pBlocker, bool duringMovement);
	virtual bool CanBeMadeMoveAway(CBaseEntity* pPusher);
	bool MakeMyBlockerMoveAway();

	bool IsFreeToManipulate();

	virtual bool CanRoamAfterCombat() { return true; }

	bool HandleDoorBlockage(CBaseEntity* pDoor) override;

	int SharedRandomLong(int low, int high);
	float SharedRandomFloat(float low, float high);

	//
	// Glowshell effects
	//
	void GlowShellOn( const Visual* visual );

	void GlowShellOff();
	void GlowShellUpdate();

	float m_glowShellTime;
	bool m_glowShellUpdate;

	Vector m_prevRenderColor;
	short m_prevRenderFx;
	short m_prevRenderMode;
	int m_prevRenderAmt;

	float m_nextPatrolPathCheck;

	// Custom hull sizes
	Vector m_minHullSize;
	Vector m_maxHullSize;

	int m_customSoundMask;
	short m_prisonerTo;
	short m_ignoredBy;
	short m_freeRoam;
	short m_activeAfterCombat;
	short m_huntActivitiesCount;

	float m_flLastTimeObservedEnemy;

	short m_sizeForGrapple;

	short m_suggestedSchedule;
	EHANDLE m_suggestedScheduleEntity;
	Vector m_suggestedScheduleOrigin;
	float m_suggestedScheduleMinDist;
	float m_suggestedScheduleMaxDist;
	int m_suggestedScheduleFlags;
	EHANDLE m_lastMoveBlocker;

	short m_gibPolicy;
	bool m_bForceConditionsGather;
	float m_flNextPainTime;
	float m_equalDislikeTime;
	int m_lootRandomSeed;

	float m_clearOwnerTime;

	const char* taskFailReason;
};

#define FREEROAM_MAPDEFAULT 0
#define FREEROAM_NEVER 1
#define FREEROAM_ALWAYS 2

#define ACTIVE_ALERT_DEFAULT 0
#define ACTIVE_ALERT_NEVER 1
#define ACTIVE_ALERT_ALWAYS 2

#define SF_DEADMONSTER_NOTSOLID 4

class CDeadMonster : public CBaseMonster
{
public:
	void Precache() override;
	void SpawnHelper(int bloodColor = BLOOD_COLOR_RED, int health = 8);
	void MonsterInitDead() override;
	void KeyValue( KeyValueData *pkvd ) override;
	virtual const char* DefaultModel() {
		return nullptr;
	}
	bool ShouldCollide(CBaseEntity* pOther) override;

	CDeadMonster* MyDeadMonsterPointer() override {return this;}
	virtual const char* getPos(int pose) const = 0;
	int	m_iPose;// which sequence to display	-- temporary, don't need to save

protected:
	void SpawnHelper(const char* defaultModel, int bloodColor = BLOOD_COLOR_RED, int health = 8);
};

#endif // BASEMONSTER_H
