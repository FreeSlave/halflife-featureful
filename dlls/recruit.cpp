#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"game.h"

class CRecruit : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("recruit"); }
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

LINK_ENTITY_TO_CLASS( monster_recruit, CRecruit )

const NamedSoundScript CRecruit::painSoundScript = {
	CHAN_VOICE,
	{ "barney/ba_pain1.wav", "barney/ba_pain2.wav", "barney/ba_pain3.wav" },
	"Recruit.Pain"
};

const NamedSoundScript CRecruit::dieSoundScript = {
	CHAN_VOICE,
	{ "barney/ba_die1.wav", "barney/ba_die2.wav", "barney/ba_die3.wav" },
	"Recruit.Die"
};

void CRecruit::Precache()
{
	PrecacheMyModel("models/recruit.mdl");
	PrecacheMyGibModel();
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	TalkInit();
	CTalkMonster::Precache();
}

void CRecruit::Spawn()
{
	Precache();

	SetMyModel( "models/recruit.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

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

void CRecruit::SetYawSpeed()
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

int CRecruit::DefaultISoundMask()
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

int CRecruit::DefaultClassify()
{
	return CLASS_PLAYER_ALLY_MILITARY;
}

void CRecruit::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

void CRecruit::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

const char* CRecruit::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "RC_ANSWER";
	case TLK_QUESTION: return "RC_QUESTION";
	case TLK_IDLE: return "RC_IDLE";
	case TLK_STARE: return "RC_STARE";
	case TLK_USE: return "RC_OK";
	case TLK_UNUSE: return "RC_WAIT";
	case TLK_DECLINE: return "RC_POK";
	case TLK_STOP: return "RC_STOP";
	case TLK_NOSHOOT: return "RC_SCARED";
	case TLK_HELLO: return "RC_HELLO";
	case TLK_PLHURT1: return "!RC_CUREA";
	case TLK_PLHURT2: return "!RC_CUREB";
	case TLK_PLHURT3: return "!RC_CUREC";
	case TLK_PHELLO: return "RC_PHELLO";
	case TLK_PIDLE: return "RC_PIDLE";
	case TLK_PQUESTION: return "RC_PQUEST";
	case TLK_SMELL: return "RC_SMELL";
	case TLK_WOUND: return "RC_WOUND";
	case TLK_MORTAL: return "RC_MORTAL";
	case TLK_SHOT: return "RC_SHOT";
	case TLK_MAD: return "RC_MAD";
	default: return NULL;
	}
}

Schedule_t* CRecruit::GetSchedule()
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
