#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"game.h"

class CDrillSergeant : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("drillsergeant"); }
	const char* DefaultDisplayName() override { return "Drill Sergeant"; }
	void SetYawSpeed() override;
	int DefaultISoundMask() override;
	int DefaultClassify() override;
	void DeathSound() override;
	void PainSound() override;

	Schedule_t *GetSchedule() override;

	const char* DefaultSentenceGroup(int group) override;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
};

LINK_ENTITY_TO_CLASS( monster_drillsergeant, CDrillSergeant )

const NamedSoundScript CDrillSergeant::painSoundScript = {
	CHAN_VOICE,
	{ "barney/ba_pain1.wav", "barney/ba_pain2.wav", "barney/ba_pain3.wav" },
	"DrillSergeant.Pain"
};

const NamedSoundScript CDrillSergeant::dieSoundScript = {
	CHAN_VOICE,
	{ "barney/ba_die1.wav", "barney/ba_die2.wav", "barney/ba_die3.wav" },
	"DrillSergeant.Die"
};

void CDrillSergeant::Precache()
{
	PrecacheMyModel("models/drill.mdl");
	PrecacheMyGibModel();
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	TalkInit();
	CTalkMonster::Precache();
}

void CDrillSergeant::Spawn()
{
	Precache();

	SetMyModel( "models/drill.mdl" );
	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( gSkillData.barneyHealth );
	pev->view_ofs = Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;
	SetMySquadCapabilities();
	SetMyCanOpenDoors(true);

	TalkMonsterInit();
}

void CDrillSergeant::SetYawSpeed()
{
	int ys = 0;
	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

int CDrillSergeant::DefaultISoundMask()
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

int CDrillSergeant::DefaultClassify()
{
	return CLASS_PLAYER_ALLY_MILITARY;
}

void CDrillSergeant::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

//=========================================================
// DeathSound
//=========================================================
void CDrillSergeant::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

const char* CDrillSergeant::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "DR_ANSWER";
	case TLK_QUESTION: return "DR_QUESTION";
	case TLK_IDLE: return "DR_IDLE";
	case TLK_STARE: return "DR_STARE";
	case TLK_USE: return "DR_OK";
	case TLK_UNUSE: return "DR_WAIT";
	case TLK_DECLINE: return "DR_POK";
	case TLK_STOP: return "DR_STOP";
	case TLK_NOSHOOT: return "DR_SCARED";
	case TLK_HELLO: return "DR_HELLO";
	case TLK_PLHURT1: return "!DR_CUREA";
	case TLK_PLHURT2: return "!DR_CUREB";
	case TLK_PLHURT3: return "!DR_CUREC";
	case TLK_PHELLO: return "DR_PHELLO";
	case TLK_PIDLE: return "DR_PIDLE";
	case TLK_PQUESTION: return "DR_PQUEST";
	case TLK_SMELL: return "DR_SMELL";
	case TLK_WOUND: return "DR_WOUND";
	case TLK_MORTAL: return "DR_MORTAL";
	case TLK_SHOT: return "DR_SHOT";
	case TLK_MAD: return "DR_MAD";
	default: return NULL;
	}
}

Schedule_t* CDrillSergeant::GetSchedule()
{
	switch (m_MonsterState) {
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
	}
		break;
	case MONSTERSTATE_COMBAT:
	{
		if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		if( HasConditions( bits_COND_HEAR_SOUND ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );	// Cower and panic from the scary sound!
		return GetScheduleOfType( SCHED_RETREAT_FROM_ENEMY );			// Run & Cower
	}
		break;
	default:
		break;
	}
	return CTalkMonster::GetSchedule();
}

class CDeadDrillSergeant : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/drill.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("drillsergeant"); }
	int	DefaultClassify() override { return	CLASS_PLAYER_ALLY_MILITARY; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[2];
};

const char *CDeadDrillSergeant::m_szPoses[] = { "dead_on_side", "dead_on_stomach" };

const char* CDeadDrillSergeant::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_drillsergeant_dead, CDeadDrillSergeant )

void CDeadDrillSergeant::Spawn()
{
	SpawnHelper();
	MonsterInitDead();
}
