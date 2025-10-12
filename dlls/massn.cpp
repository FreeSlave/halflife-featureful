#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"combat.h"
#include	"ggrenade.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"hgrunt.h"
#include	"mod_features.h"
#include	"gamerules.h"
#include	"game.h"
#include	"common_soundscripts.h"

#if FEATURE_MASSN

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	MASSN_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

// Weapon flags
#define MASSN_9MMAR					(1 << 0)
#define MASSN_HANDGRENADE			(1 << 1)
#define MASSN_GRENADELAUNCHER		(1 << 2)
#define MASSN_SNIPERRIFLE			(1 << 3)

// Body groups.
#define MASSN_HEAD_GROUP					1
#define MASSN_GUN_GROUP					2

// Head values
enum
{
	MASSN_HEAD_WHITE,
	MASSN_HEAD_BLACK,
	MASSN_HEAD_GOOGLES,
	MASSN_HEAD_COUNT,
};

// Gun values
#define MASSN_GUN_MP5				0
#define MASSN_GUN_SNIPERRIFLE				1
#define MASSN_GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MASSN_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

class CMassn : public CHGrunt
{
public:
	const char* DefaultDisplayName() { return "Male Assassin"; }
	const char* ReverseRelationshipModel() { return "models/massnf.mdl"; }
	void KeyValue(KeyValueData* pkvd);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	bool CheckRangeAttack2(float flDot, float flDist) override;
	void Sniperrifle(void);
	void GibMonster();
	void PlayUseSentence();
	void PlayUnUseSentence();
	int	DefaultClassify ( void )
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return CHGrunt::DefaultClassify();
	}

	bool FOkToSpeak(void);

	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("male_assassin"); }
	void MonsterInit();

	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;

	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override {
		return inputDamageInfo;
	}

	void SetHead(int head);

	void DropMyItems(bool isGibbed);

	int m_iHead;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript useSoundScript;
	static const NamedSoundScript unuseSoundScript;

	static constexpr const char* reloadSoundScript = "Massn.Reload";
	static constexpr const char* burst9mmSoundScript = "Massn.9MM";
	static constexpr const char* grenadeLaunchSoundScript = "Massn.GrenadeLaunch";
	static constexpr const char* sniperSoundScript = "Massn.Sniper";

protected:
	virtual void PlayFirstBurstSounds() {
		EmitSoundScript(burst9mmSoundScript);
	}
	virtual void PlayReloadSound() {
		EmitSoundScript(reloadSoundScript);
	}
	virtual void PlayGrenadeLaunchSound() {
		EmitSoundScript(grenadeLaunchSoundScript);
	}
	virtual void PlayShogtunSound() {
		EmitSoundScript(shotgunSoundScript);
	}
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMassn)

const NamedSoundScript CMassn::painSoundScript = {
	CHAN_VOICE,
	{},
	"Massn.Pain"
};

const NamedSoundScript CMassn::dieSoundScript = {
	CHAN_VOICE,
	{},
	"Massn.Die"
};

const NamedSoundScript CMassn::useSoundScript = {
	CHAN_VOICE,
	{},
	"Massn.Use"
};

const NamedSoundScript CMassn::unuseSoundScript = {
	CHAN_VOICE,
	{},
	"Massn.UnUse"
};

void CMassn::PlayUseSentence()
{
	PlaySentenceSoundScript(useSoundScript);
}

void CMassn::PlayUnUseSentence()
{
	PlaySentenceSoundScript(unuseSoundScript);
}

bool CMassn::FOkToSpeak(void)
{
	return false;
}

void CMassn::IdleSound(void)
{
}

void CMassn::Sniperrifle(void)
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, gSkillData.monDmg762, 1);

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CMassn::GibMonster( void )
{
	if( GetBodygroup( MASSN_GUN_GROUP ) != MASSN_GUN_NONE )
	{
		DropMyItems(true);
	}

	CBaseMonster::GibMonster();
}

void CMassn::DropMyItems(bool isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );

		FixupDropItemPosition(vecGunPos);

		if (!isGibbed) {
			SetBodygroup( MASSN_GUN_GROUP, MASSN_GUN_NONE );
		}
		if( FBitSet( pev->weapons, MASSN_SNIPERRIFLE ) ) {
			DropMyItem( "weapon_sniperrifle", vecGunPos, vecGunAngles, isGibbed );
		} else if ( FBitSet( pev->weapons, MASSN_9MMAR ) ) {
			DropMyItem( "weapon_9mmAR", vecGunPos, vecGunAngles, isGibbed );
		}
		if( FBitSet( pev->weapons, MASSN_GRENADELAUNCHER ) ) {
			DropMyItem( "ammo_ARgrenades", isGibbed ? vecGunPos : BodyTarget( pev->origin ), vecGunAngles, isGibbed );
		}
#if FEATURE_MONSTERS_DROP_HANDGRENADES
		if ( FBitSet (pev->weapons, MASSN_HANDGRENADE ) ) {
			CBaseEntity* pGrenadeEnt = DropMyItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles, isGibbed );
			if (pGrenadeEnt)
			{
				CBasePlayerWeapon* pGrenadeWeap = pGrenadeEnt->MyWeaponPointer();
				if (pGrenadeWeap)
					pGrenadeWeap->m_iDefaultAmmo = 1;
			}
		}
#endif
	}
	pev->weapons = 0;
}

void CMassn::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CFollowingMonster::KeyValue( pkvd );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MASSN_AE_DROP_GUN:
	{
		if(GetBodygroup(MASSN_GUN_GROUP) != MASSN_GUN_NONE)
			DropMyItems(false);
	}
	break;


	case MASSN_AE_BURST1:
	{
		ReportFireAnimEvent(pEvent->event);
		if (FBitSet(pev->weapons, MASSN_9MMAR))
		{
			Shoot();
			PlayFirstBurstSounds();
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
		}
		else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
		{
			Sniperrifle();
			EmitSoundScript(sniperSoundScript);
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 512, 0.3);

			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecGunPos );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex() + 0x1000 );		// entity, attachment
				WRITE_VECTOR( vecGunPos );		// origin
				WRITE_COORD( 24 );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 255 );	// G
				WRITE_BYTE( 192 );	// B
				WRITE_BYTE( 3 );	// life * 10
				WRITE_COORD( 0 ); // decay
			MESSAGE_END();
		}

	}
	break;

	case MASSN_AE_KICK:
	{
		PerformKick(pEvent->event, gSkillData.massnDmgKick);
	}
	break;

	case MASSN_AE_CAUGHT_ENEMY:
		break;

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
bool CMassn::CheckRangeAttack2( float flDot, float flDist )
{
	if( !FBitSet( pev->weapons, ( MASSN_HANDGRENADE | MASSN_GRENADELAUNCHER ) ) )
	{
		return false;
	}
	return CheckRangeAttack2Impl(gSkillData.massnGrenadeSpeed, flDot, flDist, FBitSet(pev->weapons, MASSN_GRENADELAUNCHER));
}

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
	SpawnHelper("models/massn.mdl", gSkillData.massnHealth);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
	}

	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_SNIPERRIFLE);
		m_cClipSize = 1;
	}
	else
	{
		m_cClipSize = MASSN_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (m_iHead == -1 || m_iHead >= MASSN_HEAD_COUNT) {
		m_iHead = RANDOM_LONG(MASSN_HEAD_WHITE, MASSN_HEAD_BLACK); // never random night googles
	}
	SetBodygroup(MASSN_HEAD_GROUP, m_iHead);

	FollowingMonsterInit();
}

void CMassn::MonsterInit()
{
	CHGrunt::MonsterInit();
	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		m_flDistTooFar = 2048.0f;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
	PrecacheHelper("models/massn.mdl");

	// Note: these are optional
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(useSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript);

	RegisterAndPrecacheSoundScript(reloadSoundScript, NPC::reloadSoundScript);
	RegisterAndPrecacheSoundScript(burst9mmSoundScript, NPC::burst9mmSoundScript);
	RegisterAndPrecacheSoundScript(grenadeLaunchSoundScript, NPC::grenadeLaunchSoundScript);
	RegisterAndPrecacheSoundScript(sniperSoundScript, NPC::sniperSoundScript);

	UTIL_PrecacheOther("grenade", GetProjectileOverrides());

	m_voicePitch = 100;
	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// PainSound
//=========================================================
void CMassn::PainSound()
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// DeathSound
//=========================================================
void CMassn::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

void CMassn::SetHead(int head)
{
	m_iHead = head;
}

//=========================================================
// CAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CAssassinRepel : public CHGruntRepel
{
public:
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("male_assassin"); }
	void KeyValue(KeyValueData* pkvd);
	const char* TrooperName() {
		return "monster_male_assassin";
	}
	void PrepareBeforeSpawn(CBaseEntity* pEntity);

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int head;
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CAssassinRepel)

TYPEDESCRIPTION	CAssassinRepel::m_SaveData[] =
{
	DEFINE_FIELD( CAssassinRepel, head, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAssassinRepel, CHGruntRepel )

void CAssassinRepel::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		head = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CHGruntRepel::KeyValue( pkvd );
}

void CAssassinRepel::PrepareBeforeSpawn(CBaseEntity *pEntity)
{
	CMassn* massn = (CMassn*)pEntity;
	massn->m_iHead = head;
}

class CDeadMassn : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/massn.mdl"; }
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("male_assassin"); }
	int	DefaultClassify()
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return CLASS_HUMAN_MILITARY;
	}

	void KeyValue( KeyValueData *pkvd );
	const char* getPos(int pos) const;

	int	m_iHead;
	static const char *m_szPoses[3];
};

const char *CDeadMassn::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

const char* CDeadMassn::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

void CDeadMassn::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CDeadMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_male_assassin_dead, CDeadMassn )
LINK_ENTITY_TO_CLASS( monster_massassin_dead, CDeadMassn )

void CDeadMassn::Spawn( )
{
	SpawnHelper();

	if ( pev->weapons <= 0 )
	{
		SetBodygroup( MASSN_GUN_GROUP, MASSN_GUN_NONE );
	}
	if (FBitSet( pev->weapons, MASSN_9MMAR ))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_MP5);
	}
	if (FBitSet( pev->weapons, MASSN_SNIPERRIFLE ))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_SNIPERRIFLE);
	}

	if ( m_iHead < 0 || m_iHead >= MASSN_HEAD_COUNT ) {
		m_iHead = RANDOM_LONG(MASSN_HEAD_WHITE, MASSN_HEAD_BLACK);  // never random night googles
	}

	SetBodygroup( MASSN_HEAD_GROUP, m_iHead );

	MonsterInitDead();
}

#endif
