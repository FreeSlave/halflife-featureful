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
//=========================================================
// Houndeye - spooky sonic dog. 
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"combat.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"nodes.h"
#include	"squadmonster.h"
#include	"soundent.h"
#include	"game.h"
#include	"visuals_utils.h"

// houndeye does 20 points of damage spread over a sphere 384 units in diameter, and each additional 
// squad member increases the BASE damage by 110%, per the spec.
#define HOUNDEYE_MAX_SQUAD_SIZE			4
#define	HOUNDEYE_MAX_ATTACK_RADIUS		384.0f
#define	HOUNDEYE_SQUAD_BONUS			1.1f

#define SF_HOUNDEYE_START_SLEEPING SF_MONSTER_SPECIAL_FLAG

enum
{
	HOUNDEYE_FORCE_LAZY_WAKING = -2,
	HOUNDEYE_FORCE_URGENT_WAKING = -1,
	HOUNDEYE_AWAKE = 0,
	HOUNDEYE_SLEEPING,
	HOUNDEYE_DEEP_SLEEPING
};

enum
{
	HOUNDEYE_EYE_OPEN = 0,
	HOUNDEYE_EYE_HALFCLOSED,
	HOUNDEYE_EYE_CLOSED,
	HOUNDEYE_EYE_FRAMES
};

enum
{
	HOUNDEYE_BLINK,
	HOUNDEYE_DONT_BLINK,
	HOUNDEYE_HALF_BLINK,
};

#define HOUNDEYE_SOUND_STARTLE_VOLUME	128 // how loud a sound has to be to badly scare a sleeping houndeye

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_HOUND_CLOSE_EYE = LAST_COMMON_TASK + 1,
	TASK_HOUND_OPEN_EYE,
	TASK_HOUND_THREAT_DISPLAY,
	TASK_HOUND_FALL_ASLEEP,
	TASK_HOUND_WAKE_UP,
	TASK_HOUND_HOP_BACK,
	TASK_HOUND_HALF_ASLEEP,
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_HOUND_AGITATED = LAST_COMMON_SCHEDULE + 1,
	SCHED_HOUND_HOP_RETREAT,
	SCHED_HOUND_FAIL,
	SCHED_HOUND_EAT,
	SCHED_HOUND_DEEPSLEEP
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HOUND_AE_WARN			1
#define		HOUND_AE_STARTATTACK		2
#define		HOUND_AE_THUMP			3
#define		HOUND_AE_ANGERSOUND1		4
#define		HOUND_AE_ANGERSOUND2		5
#define		HOUND_AE_HOPBACK		6
#define		HOUND_AE_CLOSE_EYE		7

class CHoundeye : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Houndeye"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void SetYawSpeed() override;
	void WarmUpSound();
	void AlertSound() override;
	void DeathSound() override;
	void WarnSound();
	void PainSound() override;
	void IdleSound() override;
	void StartTask( Task_t *pTask ) override;
	void RunTask( Task_t *pTask ) override;
	void SonicAttack();
	void PrescheduleThink() override;
	void Activate() override;
	int LookupActivity(int activity) override;
	void SetActivity( Activity NewActivity ) override;
	const Visual* GetWaveVisual();
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool WantsToGetCloseToEnemy() override;
	bool FValidateHintType( short sHint ) override;
	bool FCanActiveIdle() override;
	Schedule_t *GetScheduleOfType( int Type ) override;
	Schedule_t *GetSchedule() override;
	int IgnoreConditions() override;
	int DefaultISoundMask() override;
	float HearingSensitivity() override;
	bool FInViewCone( CBaseEntity *pEntity ) override;
	void EXPORT TouchSleeping( CBaseEntity* pToucher );
	void EXPORT UseSleeping( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	int DefaultSizeForGrapple() override { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() override { return true; }
	Vector DefaultMinHullSize() override { return Vector( -16.0f, -16.0f, 0.0f ); }
	Vector DefaultMaxHullSize() override { return Vector( 16.0f, 16.0f, 36.0f ); }

	short m_iAsleep;// some houndeyes sleep in idle mode if this is set, the houndeye is lying down
	short m_iBlink;
	Vector	m_vecPackCenter; // the center of the pack. The leader maintains this by averaging the origins of all pack members.

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript warnSoundScript;
	static const NamedSoundScript warmupSoundScript;
	static const NamedSoundScript blastSoundScript;

	static const NamedSoundScript anger1SoundScript;
	static const NamedSoundScript anger2SoundScript;

	static const NamedVisual waveVisual;
	static const NamedVisual wave1Visual;
	static const NamedVisual wave2Visual;
	static const NamedVisual wave3Visual;
	static const NamedVisual wave4Visual;
};

LINK_ENTITY_TO_CLASS( monster_houndeye, CHoundeye )

TYPEDESCRIPTION	CHoundeye::m_SaveData[] =
{
	//DEFINE_FIELD( CHoundeye, m_iSpriteTexture, FIELD_INTEGER ), // Restored in precache
	DEFINE_FIELD( CHoundeye, m_iAsleep, FIELD_SHORT ),
	DEFINE_FIELD( CHoundeye, m_iBlink, FIELD_SHORT ),
	DEFINE_FIELD( CHoundeye, m_vecPackCenter, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CHoundeye, CSquadMonster )

const NamedSoundScript CHoundeye::idleSoundScript = {
	CHAN_VOICE,
	{"houndeye/he_idle1.wav", "houndeye/he_idle2.wav", "houndeye/he_idle3.wav"},
	"HoundEye.Idle"
};

const NamedSoundScript CHoundeye::alertSoundScript = {
	CHAN_VOICE,
	{"houndeye/he_alert1.wav", "houndeye/he_alert2.wav", "houndeye/he_alert3.wav"},
	"HoundEye.Alert"
};

const NamedSoundScript CHoundeye::painSoundScript = {
	CHAN_VOICE,
	{"houndeye/he_pain3.wav", "houndeye/he_pain4.wav", "houndeye/he_pain5.wav"},
	"HoundEye.Pain"
};

const NamedSoundScript CHoundeye::dieSoundScript = {
	CHAN_VOICE,
	{"houndeye/he_die1.wav", "houndeye/he_die2.wav", "houndeye/he_die3.wav"},
	"HoundEye.Die"
};

const NamedSoundScript CHoundeye::warnSoundScript = {
	CHAN_VOICE,
	{"houndeye/he_hunt1.wav", "houndeye/he_hunt2.wav", "houndeye/he_hunt3.wav"},
	"HoundEye.Warn"
};

const NamedSoundScript CHoundeye::warmupSoundScript = {
	CHAN_WEAPON,
	{"houndeye/he_attack1.wav", "houndeye/he_attack3.wav" },
	0.7f,
	ATTN_NORM,
	"HoundEye.Warmup"
};

const NamedSoundScript CHoundeye::blastSoundScript = {
	CHAN_WEAPON,
	{"houndeye/he_blast1.wav", "houndeye/he_blast2.wav", "houndeye/he_blast3.wav"},
	"HoundEye.Sonic"
};

const NamedSoundScript CHoundeye::anger1SoundScript = {
	CHAN_VOICE,
	{"houndeye/he_pain3.wav" },
	"HoundEye.Anger1"
};

const NamedSoundScript CHoundeye::anger2SoundScript = {
	CHAN_VOICE,
	{"houndeye/he_pain1.wav"},
	"HoundEye.Anger2"
};

const NamedVisual CHoundeye::waveVisual = BuildVisual("Houndeye.WaveBase")
		.Model("sprites/shockwave.spr")
		.Life(0.2f)
		.BeamParams(16, 0)
		.Alpha(255);

const NamedVisual CHoundeye::wave1Visual = BuildVisual("Houndeye.Wave1")
		.RenderColor(188, 220, 255)
		.Mixin(&CHoundeye::waveVisual);

const NamedVisual CHoundeye::wave2Visual = BuildVisual("Houndeye.Wave2")
		.RenderColor(101, 133, 221)
		.Mixin(&CHoundeye::waveVisual);

const NamedVisual CHoundeye::wave3Visual = BuildVisual("Houndeye.Wave3")
		.RenderColor(67, 85, 255)
		.Mixin(&CHoundeye::waveVisual);

const NamedVisual CHoundeye::wave4Visual = BuildVisual("Houndeye.Wave4")
		.RenderColor(62, 33, 211)
		.Mixin(&CHoundeye::waveVisual);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHoundeye::DefaultClassify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
//  FValidateHintType 
//=========================================================
bool CHoundeye::FValidateHintType( short sHint )
{
	size_t i;

	static short sHoundHints[] =
	{
		HINT_WORLD_MACHINERY,
		HINT_WORLD_BLINKING_LIGHT,
		HINT_WORLD_HUMAN_BLOOD,
		HINT_WORLD_ALIEN_BLOOD,
	};

	for( i = 0; i < ARRAYSIZE( sHoundHints ); i++ )
	{
		if( sHoundHints[i] == sHint )
		{
			return true;
		}
	}

	ALERT( at_aiconsole, "%s couldn't validate hint type\n", STRING(pev->classname) );
	return false;
}

//=========================================================
// FCanActiveIdle
//=========================================================
bool CHoundeye::FCanActiveIdle()
{
	if( InSquad() )
	{
		CSquadMonster *pSquadLeader = MySquadLeader();

		for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
		{
			CSquadMonster *pMember = pSquadLeader->MySquadMember( i );

			if( pMember != NULL && pMember != this && pMember->m_iHintNode != NO_NODE )
			{
				// someone else in the group is active idling right now!
				return false;
			}
		}

		return true;
	}

	return true;
}

//=========================================================
// CheckRangeAttack1 - overridden for houndeyes so that they
// try to get within half of their max attack radius before
// attacking, so as to increase their chances of doing damage.
//=========================================================
bool CHoundeye::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= ( HOUNDEYE_MAX_ATTACK_RADIUS * 0.5f ) && flDot >= 0.3f )
	{
		return true;
	}
	return false;
}

bool CHoundeye::WantsToGetCloseToEnemy()
{
	return true;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHoundeye::SetYawSpeed()
{
	int ys = 90;

	switch( m_Activity )
	{
	case ACT_CROUCHIDLE://sleeping!
		ys = 0;
		break;
	case ACT_IDLE:	
		ys = 60;
		break;
	case ACT_WALK:
	case ACT_RUN:	
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		break;
	default:
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// SetActivity 
//=========================================================
int CHoundeye::LookupActivity(int activity)
{
	if( m_MonsterState == MONSTERSTATE_COMBAT && activity == ACT_IDLE && RANDOM_LONG( 0, 1 ) )
	{
		// play pissed idle.
		return LookupSequence( "madidle" );
	}
	return CSquadMonster::LookupActivity(activity);
}

void CHoundeye::SetActivity( Activity NewActivity )
{
	if( NewActivity == m_Activity )
		return;

	CSquadMonster::SetActivity( NewActivity );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHoundeye::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HOUND_AE_WARN:
			// do stuff for this event.
			WarnSound();
			break;
		case HOUND_AE_STARTATTACK:
			WarmUpSound();
			break;
		case HOUND_AE_HOPBACK:
			{
				float flGravity = g_psv_gravity->value;

				pev->flags &= ~FL_ONGROUND;

				pev->velocity = gpGlobals->v_forward * -200.0f;
				pev->velocity.z += ( 0.6f * flGravity ) * 0.5f;
				break;
			}
		case HOUND_AE_THUMP:
			// emit the shockwaves
			ReportFireAnimEvent(pEvent->event);
			SonicAttack();
			break;
		case HOUND_AE_ANGERSOUND1:
			EmitSoundScript(anger1SoundScript);
			break;
		case HOUND_AE_ANGERSOUND2:
			EmitSoundScript(anger2SoundScript);
			break;
		case HOUND_AE_CLOSE_EYE:
			if( m_iBlink == HOUNDEYE_BLINK )
			{
				pev->skin = HOUNDEYE_EYE_CLOSED;
			}
			break;
		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHoundeye::Spawn()
{
	Precache();

	SetMyModel( "models/houndeye.mdl" );
	SetMySize();

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_YELLOW );
	pev->effects		= 0;
	SetMyHealth( gSkillData.houndeyeHealth );
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_iAsleep		= HOUNDEYE_AWAKE; // everyone spawns awake
	m_iBlink		= HOUNDEYE_BLINK;
	SetMySquadCapabilities(bits_CAP_SQUAD|bits_CAP_SQUAD_SAME_CLASSNAME);
	SetMyCanOpenDoors(false);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHoundeye::Precache()
{
	PrecacheMyModel( "models/houndeye.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(warnSoundScript);
	RegisterAndPrecacheSoundScript(warmupSoundScript);
	RegisterAndPrecacheSoundScript(blastSoundScript);

	RegisterAndPrecacheSoundScript(anger1SoundScript);
	RegisterAndPrecacheSoundScript(anger2SoundScript);

	RegisterVisual(waveVisual);
	RegisterVisual(wave1Visual);
	RegisterVisual(wave2Visual);
	RegisterVisual(wave3Visual);
	RegisterVisual(wave4Visual);
}

//=========================================================
// IdleSound
//=========================================================
void CHoundeye::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// IdleSound
//=========================================================
void CHoundeye::WarmUpSound()
{
	EmitSoundScript(warmupSoundScript);
}

//=========================================================
// WarnSound 
//=========================================================
void CHoundeye::WarnSound()
{
	EmitSoundScript(warnSoundScript);
}

//=========================================================
// AlertSound 
//=========================================================
void CHoundeye::AlertSound()
{
	if( InSquad() && !IsLeader() )
	{
		return; // only leader makes ALERT sound.
	}

	EmitSoundScript(alertSoundScript);
}

//=========================================================
// DeathSound 
//=========================================================
void CHoundeye::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

//=========================================================
// PainSound 
//=========================================================
void CHoundeye::PainSound()
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// WriteBeamColor - writes a color vector to the network 
// based on the size of the group. 
//=========================================================
const Visual* CHoundeye::GetWaveVisual()
{
	const int squadSize = SquadCount();
	switch( squadSize )
	{
	default:
		ALERT( at_aiconsole, "Unsupported Houndeye SquadSize %d!\n", squadSize );
	case 0:
	case 1:
		// solo houndeye - weakest beam
		return GetVisual(wave1Visual);
	case 2:
		return GetVisual(wave2Visual);
	case 3:
		return GetVisual(wave3Visual);
	case 4:
	case 5:
		return GetVisual(wave4Visual);
	}
}

//=========================================================
// SonicAttack
//=========================================================
void CHoundeye::SonicAttack()
{
	EmitSoundScript(blastSoundScript);

	const Visual* visual = GetWaveVisual();

	const Vector blastOrigin = pev->origin + Vector(0, 0, 16.0f);
	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_CIRCLE( blastOrigin, HOUNDEYE_MAX_ATTACK_RADIUS / 0.2f );
		WriteBeamVisual(visual);
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_CIRCLE( blastOrigin, ( HOUNDEYE_MAX_ATTACK_RADIUS / 2.0f ) / 0.2f );
		WriteBeamVisual(visual);
	MESSAGE_END();

	const int squadCount = SquadCount();

	float flDamage = gSkillData.houndeyeDmgBlast;
	if( squadCount > 1 )
	{
		// squad gets attack bonus.
		flDamage = gSkillData.houndeyeDmgBlast + gSkillData.houndeyeDmgBlast * ( HOUNDEYE_SQUAD_BONUS * ( squadCount - 1 ) );
	}

	::RadiusDamage(this, pev->origin, pev, pev, DamageInfo(flDamage, DMG_SONIC).SetGibPolicy(GIB_ALWAYS),
				   HOUNDEYE_MAX_ATTACK_RADIUS,
				   RADIUSDAMAGE_SPOT_IS_TARGET_CENTER|RADIUSDAMAGE_APPLY_FALLOFF,
				   [this](CBaseEntity* pEntity) {
		return !FClassnameIs( pEntity->pev, "monster_houndeye" ) || IRelationship(pEntity) >= R_DL;
	});
}

//=========================================================
// start task
//=========================================================
void CHoundeye::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_HOUND_HALF_ASLEEP:
		{
			pev->skin = HOUNDEYE_EYE_HALFCLOSED;
			m_iBlink = HOUNDEYE_HALF_BLINK;
			TaskComplete();
			break;
		}
	case TASK_HOUND_FALL_ASLEEP:
		{
			if (FBitSet(pev->spawnflags, SF_HOUNDEYE_START_SLEEPING))
			{
				ClearBits(pev->spawnflags, SF_HOUNDEYE_START_SLEEPING);
				SetTouch(&CHoundeye::TouchSleeping);
				SetUse(&CHoundeye::UseSleeping);
				m_iAsleep = HOUNDEYE_DEEP_SLEEPING;
			}
			else
			{
				m_iAsleep = HOUNDEYE_SLEEPING; // signal that hound is lying down (must stand again before doing anything else!)
			}
			TaskComplete();
			break;
		}
	case TASK_HOUND_WAKE_UP:
		{
			m_iAsleep = HOUNDEYE_AWAKE; // signal that hound is standing again
			TaskComplete();
			break;
		}
	case TASK_HOUND_OPEN_EYE:
		{
			pev->skin = HOUNDEYE_EYE_OPEN;
			m_iBlink = HOUNDEYE_BLINK; // turn blinking back on and that code will automatically open the eye
			TaskComplete();
			break;
		}
	case TASK_HOUND_CLOSE_EYE:
		{
			m_iBlink = HOUNDEYE_DONT_BLINK; // tell blink code to leave the eye alone.
			break;
		}
	case TASK_HOUND_THREAT_DISPLAY:
		{
			m_IdealActivity = ACT_IDLE_ANGRY;
			break;
		}
	case TASK_HOUND_HOP_BACK:
		{
			m_IdealActivity = ACT_LEAP;
			break;
		}
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;

/*
			if( InSquad() )
			{
				// see if there is a battery to connect to. 
				CSquadMonster *pSquad = m_pSquadLeader;

				while( pSquad )
				{
					if( pSquad->m_iMySlot == bits_SLOT_HOUND_BATTERY )
					{
						// draw a beam.
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
							WRITE_BYTE( TE_BEAMENTS );
							WRITE_SHORT( ENTINDEX( this->edict() ) );
							WRITE_SHORT( ENTINDEX( pSquad->edict() ) );
							WRITE_SHORT( m_iSpriteTexture );
							WRITE_BYTE( 0 ); // framestart
							WRITE_BYTE( 0 ); // framerate
							WRITE_BYTE( 10 ); // life
							WRITE_BYTE( 40 );  // width
							WRITE_BYTE( 10 );   // noise
							WRITE_BYTE( 0 );   // r, g, b
							WRITE_BYTE( 50 );   // r, g, b
							WRITE_BYTE( 250);   // r, g, b
							WRITE_BYTE( 255 );	// brightness
							WRITE_BYTE( 30 );		// speed
						MESSAGE_END();
						break;
					}

					pSquad = pSquad->m_pSquadNext;
				}
			}
*/
			break;
		}
	case TASK_SPECIAL_ATTACK1:
		{
			m_IdealActivity = ACT_SPECIAL_ATTACK1;
			break;
		}
	case TASK_GUARD:
		{
			m_IdealActivity = ACT_GUARD;
			break;
		}
	default: 
		{
			CSquadMonster::StartTask( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask 
//=========================================================
void CHoundeye::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_HOUND_THREAT_DISPLAY:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_HOUND_CLOSE_EYE:
		{
			if( pev->skin < HOUNDEYE_EYE_CLOSED )
			{
				pev->skin++;
			}
			if ( pev->skin == HOUNDEYE_EYE_CLOSED )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_HOUND_HOP_BACK:
		{
			if( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_SPECIAL_ATTACK1:
		{
			pev->skin = RANDOM_LONG( 0, HOUNDEYE_EYE_FRAMES - 1 );

			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			float life;
			life = ( ( 255 - pev->frame ) / ( pev->framerate * m_flFrameRate ) );
			if( life < 0.1f )
				life = 0.1f;

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_IMPLOSION );
				WRITE_VECTOR( pev->origin + Vector(0, 0, 16.0f) );
				WRITE_BYTE( 50.0f * life + 100.0f );  // radius
				WRITE_BYTE( pev->frame / 25.0f ); // count
				WRITE_BYTE( life * 10.0f ); // life
			MESSAGE_END();

			if( m_fSequenceFinished )
			{
				SonicAttack(); 
				TaskComplete();
			}
			break;
		}
	default:
		{
			CSquadMonster::RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PrescheduleThink
//=========================================================
void CHoundeye::PrescheduleThink()
{
	// if the hound is mad and is running, make hunt noises.
	if( m_MonsterState == MONSTERSTATE_COMBAT && m_Activity == ACT_RUN && RANDOM_FLOAT( 0, 1 ) < 0.2f )
	{
		WarnSound();
	}

	// at random, initiate a blink if not already blinking or sleeping
	if( m_iBlink == HOUNDEYE_BLINK )
	{
		if( ( pev->skin == HOUNDEYE_EYE_OPEN ) && RANDOM_LONG( 0, 0x7F ) == 0 )
		{
			// start blinking!
			pev->skin = HOUNDEYE_EYE_FRAMES - 1;
		}
		else if( pev->skin != HOUNDEYE_EYE_OPEN )
		{
			// already blinking
			pev->skin--;
		}
	}
	else if ( m_iBlink == HOUNDEYE_HALF_BLINK )
	{
		if ( pev->skin == HOUNDEYE_EYE_HALFCLOSED && RANDOM_LONG( 0, 0x3F ) == 0 )
		{
			pev->skin = HOUNDEYE_EYE_CLOSED;
		}
		else if (pev->skin != HOUNDEYE_EYE_HALFCLOSED)
		{
			pev->skin = HOUNDEYE_EYE_HALFCLOSED;
		}
	}

	// if you are the leader, average the origins of each pack member to get an approximate center.
	if( IsLeader() )
	{
		CSquadMonster *pSquadMember;
		int iSquadCount = 0;

		for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
		{
			pSquadMember = MySquadMember( i );

			if( pSquadMember )
			{
				iSquadCount++;
				m_vecPackCenter = m_vecPackCenter + pSquadMember->pev->origin;
			}
		}

		m_vecPackCenter = m_vecPackCenter / iSquadCount;
	}
}

void CHoundeye::Activate()
{
	if (m_iAsleep == HOUNDEYE_DEEP_SLEEPING)
	{
		SetBits(pev->spawnflags, SF_HOUNDEYE_START_SLEEPING);
	}
	CSquadMonster::Activate();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t tlHoundGuardPack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GUARD		},
};

Schedule_t	slHoundGuardPack[] =
{
	{ 
		tlHoundGuardPack,
		ARRAYSIZE ( tlHoundGuardPack ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_PROVOKED		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_COMBAT		|// sound flags
		bits_SOUND_WORLD		|
		bits_SOUND_MEAT			|
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"GuardPack"
	},
};

// primary range attack
Task_t	tlHoundYell1[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_SET_SCHEDULE,		(float)SCHED_HOUND_AGITATED	},
};

Task_t	tlHoundYell2[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slHoundRangeAttack[] =
{
	{ 
		tlHoundYell1,
		ARRAYSIZE ( tlHoundYell1 ), 
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"HoundRangeAttack1"
	},
	{ 
		tlHoundYell2,
		ARRAYSIZE ( tlHoundYell2 ), 
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"HoundRangeAttack2"
	},
};

// lie down and fall asleep
Task_t	tlHoundSleep[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE			},
	{ TASK_WAIT_RANDOM,			(float)5				},
	{ TASK_HOUND_HALF_ASLEEP,	(float)0				},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_CROUCH		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE	},
	{ TASK_HOUND_FALL_ASLEEP,	(float)0				},
	{ TASK_HOUND_CLOSE_EYE,		(float)0				},
	{ TASK_WAIT,				(float)5				},
	{ TASK_WAIT_RANDOM,			(float)20				},
};

Schedule_t	slHoundSleep[] =
{
	{
		tlHoundSleep,
		ARRAYSIZE ( tlHoundSleep ), 
		bits_COND_HEAR_SOUND	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_COMBAT		|
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_WORLD,
		"Hound Sleep"
	},
};

// wake and stand up lazily
Task_t	tlHoundWakeLazy[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_HOUND_HALF_ASLEEP,	(float)0			},
	{ TASK_WAIT_RANDOM,			(float)2.5			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_STAND	},
	{ TASK_HOUND_OPEN_EYE,		(float)0			},
	{ TASK_HOUND_WAKE_UP,		(float)0			},
};

Schedule_t	slHoundWakeLazy[] =
{
	{
		tlHoundWakeLazy,
		ARRAYSIZE ( tlHoundWakeLazy ),
		0,
		0,
		"WakeLazy"
	},
};

// wake and stand up lazily, without random waiting
Task_t	tlHoundWakeLazyNoWait[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_HOUND_HALF_ASLEEP,	(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_STAND	},
	{ TASK_HOUND_OPEN_EYE,		(float)0			},
	{ TASK_HOUND_WAKE_UP,		(float)0			},
};

Schedule_t	slHoundWakeLazyNoWait[] =
{
	{
		tlHoundWakeLazyNoWait,
		ARRAYSIZE ( tlHoundWakeLazyNoWait ),
		0,
		0,
		"WakeLazyNoWait"
	},
};

// wake and stand up with great urgency!
Task_t	tlHoundWakeUrgent[] =
{
	{ TASK_HOUND_OPEN_EYE,		(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_HOP		},
	{ TASK_HOUND_WAKE_UP,		(float)0			},
	{ TASK_FACE_IDEAL,			(float)0			},
};

Schedule_t	slHoundWakeUrgent[] =
{
	{
		tlHoundWakeUrgent,
		ARRAYSIZE ( tlHoundWakeUrgent ),
		0,
		0,
		"WakeUrgent"
	},
};

// Sleep indefinitely
Task_t	tlHoundDeepSleep[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_HOUND_FALL_ASLEEP,	(float)0				},
	{ TASK_HOUND_CLOSE_EYE,		(float)0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE	},
	{ TASK_WAIT_INDEFINITE,		(float)0				},
};

Schedule_t	slHoundDeepSleep[] =
{
	{
		tlHoundDeepSleep,
		ARRAYSIZE ( tlHoundDeepSleep ),
		bits_COND_HEAR_SOUND	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_COMBAT		|
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER,
		"Hound Deep Sleep"
	},
};

Task_t	tlHoundSpecialAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_SPECIAL_ATTACK1,		(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_IDLE_ANGRY },
};

Schedule_t	slHoundSpecialAttack1[] =
{
	{ 
		tlHoundSpecialAttack1,
		ARRAYSIZE ( tlHoundSpecialAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED,
		
		0,
		"Hound Special Attack1"
	},
};

Task_t	tlHoundAgitated[] =
{
	{ TASK_STOP_MOVING,				0		},
	{ TASK_HOUND_THREAT_DISPLAY,	0		},
};

Schedule_t	slHoundAgitated[] =
{
	{ 
		tlHoundAgitated,
		ARRAYSIZE ( tlHoundAgitated ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Hound Agitated"
	},
};

Task_t	tlHoundHopRetreat[] =
{
	{ TASK_STOP_MOVING,				0											},
	{ TASK_HOUND_HOP_BACK,			0											},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slHoundHopRetreat[] =
{
	{ 
		tlHoundHopRetreat,
		ARRAYSIZE ( tlHoundHopRetreat ), 
		0,
		0,
		"Hound Hop Retreat"
	},
};

// hound fails in combat with client in the PVS
Task_t	tlHoundCombatFailPVS[] =
{
	{ TASK_STOP_MOVING,				0			},
	{ TASK_HOUND_THREAT_DISPLAY,	0			},
	{ TASK_WAIT_FACE_ENEMY,			1.0f	},
};

Schedule_t	slHoundCombatFailPVS[] =
{
	{ 
		tlHoundCombatFailPVS,
		ARRAYSIZE( tlHoundCombatFailPVS ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"HoundCombatFailPVS"
	},
};

// hound fails in combat with no client in the PVS. Don't keep peeping!
Task_t	tlHoundCombatFailNoPVS[] =
{
	{ TASK_STOP_MOVING,				0				},
	{ TASK_HOUND_THREAT_DISPLAY,	0				},
	{ TASK_WAIT_FACE_ENEMY,			(float)2		},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE	},
	{ TASK_WAIT_PVS,				0				},
};

Schedule_t	slHoundCombatFailNoPVS[] =
{
	{ 
		tlHoundCombatFailNoPVS,
		ARRAYSIZE ( tlHoundCombatFailNoPVS ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"HoundCombatFailNoPVS"
	},
};

// hound walks to something tasty and eats it.
Task_t tlHoundEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the hound can't get to the food
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.5f },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slHoundEat[] =
{
	{
		tlHoundEat,
		ARRAYSIZE( tlHoundEat ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"HoundEat"
	}
};

DEFINE_CUSTOM_SCHEDULES( CHoundeye )
{
	slHoundGuardPack,
	slHoundRangeAttack,
	&slHoundRangeAttack[ 1 ],
	slHoundEat,
	slHoundSleep,
	slHoundWakeLazy,
	slHoundWakeLazyNoWait,
	slHoundWakeUrgent,
	slHoundDeepSleep,
	slHoundSpecialAttack1,
	slHoundAgitated,
	slHoundHopRetreat,
	slHoundCombatFailPVS,
	slHoundCombatFailNoPVS,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHoundeye, CSquadMonster )

//=========================================================
// GetScheduleOfType 
//=========================================================
Schedule_t *CHoundeye::GetScheduleOfType( int Type ) 
{
	if (FBitSet(pev->spawnflags, SF_HOUNDEYE_START_SLEEPING))
	{
		return &slHoundDeepSleep[0];
	}
	if( m_iAsleep )
	{
		if (m_iAsleep == HOUNDEYE_FORCE_URGENT_WAKING)
		{
			return &slHoundWakeUrgent[0];
		}
		if (m_iAsleep == HOUNDEYE_FORCE_LAZY_WAKING)
		{
			return &slHoundWakeLazyNoWait[0];
		}
		// if the hound is sleeping, must wake and stand!
		if( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pWakeSound = PBestSound();
			ASSERT( pWakeSound != NULL );
			if( pWakeSound )
			{
				MakeIdealYaw( pWakeSound->m_vecOrigin );

				if( FLSoundVolume( pWakeSound ) >= HOUNDEYE_SOUND_STARTLE_VOLUME )
				{
					// awakened by a loud sound
					return &slHoundWakeUrgent[0];
				}
			}
			// sound was not loud enough to scare the bejesus out of houndeye
			return &slHoundWakeLazy[0];
		}
		else if( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			MakeIdealYaw( m_vecEnemyLKP );
			// get up fast, to fight.
			return &slHoundWakeUrgent[0];
		}
		else
		{
			// hound is waking up on its own
			return &slHoundWakeLazy[0];
		}
	}
	if ( m_iBlink == HOUNDEYE_HALF_BLINK )
	{
		pev->skin = HOUNDEYE_EYE_OPEN;
		m_iBlink = HOUNDEYE_BLINK;
	}
	switch( Type )
	{
	case SCHED_IDLE_STAND:
		{
			// we may want to sleep instead of stand!
			if( InSquad() && !IsLeader() && !m_iAsleep && RANDOM_LONG( 0, 29 ) < 1 )
			{
				return &slHoundSleep[0];
			}
			else if ( IsLeader() && !m_iAsleep && RANDOM_LONG(0, 14) < 1 )
			{
				return GetScheduleOfType( SCHED_GUARD );
			}
			else
			{
				return CSquadMonster::GetScheduleOfType( Type );
			}
		}
	case SCHED_RANGE_ATTACK1:
		{
			return &slHoundRangeAttack[0];
/*
			if( InSquad() )
			{
				return &slHoundRangeAttack[RANDOM_LONG( 0, 1 )];
			}

			return &slHoundRangeAttack[1];
*/
		}
	case SCHED_SPECIAL_ATTACK1:
		{
			return &slHoundSpecialAttack1[0];
		}
	case SCHED_GUARD:
		{
			return &slHoundGuardPack[0];
		}
	case SCHED_HOUND_AGITATED:
		{
			return &slHoundAgitated[0];
		}
	case SCHED_HOUND_HOP_RETREAT:
		{
			return &slHoundHopRetreat[0];
		}
	case SCHED_FAIL:
		{
			if( m_MonsterState == MONSTERSTATE_COMBAT )
			{
				if( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
				{
					// client in PVS
					return &slHoundCombatFailPVS[0];
				}
				else
				{
					// client has taken off! 
					return &slHoundCombatFailNoPVS[0];
				}
			}
			else
			{
				return CSquadMonster::GetScheduleOfType( Type );
			}
		}
	case SCHED_HOUND_EAT:
		{
			return &slHoundEat[0];
		}
	case SCHED_HOUND_DEEPSLEEP:
		{
			return &slHoundDeepSleep[0];
		}
	default:
		{
			return CSquadMonster::GetScheduleOfType( Type );
		}
	}
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CHoundeye::GetSchedule()
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}
			if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			{
				if( RANDOM_FLOAT( 0.0f, 1.0f ) <= 0.4f )
				{
					TraceResult tr;
					UTIL_MakeVectors( pev->angles );
					UTIL_TraceHull( pev->origin, pev->origin + gpGlobals->v_forward * -128, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

					if( tr.flFraction == 1.0f )
					{
						// it's clear behind, so the hound will jump
						return GetScheduleOfType( SCHED_HOUND_HOP_RETREAT );
					}
				}

				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if( OccupySlot( bits_SLOTS_HOUND_ATTACK ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}

				return GetScheduleOfType( SCHED_HOUND_AGITATED );
			}
			break;
		}
	case MONSTERSTATE_ALERT:
		{
			if( HasConditions( bits_COND_SMELL_FOOD ) )
			{
				CSound *pSound = PBestScent();
				if( pSound && FVisible( pSound->m_vecOrigin ) ) {
					// scent is not occluded
					return GetScheduleOfType( SCHED_HOUND_EAT );
				}
			}
			break;
		}
	default:
			break;
	}

	return CSquadMonster::GetSchedule();
}

int CHoundeye::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	// Houndeyes eat only if injured
	if (pev->health >= pev->max_health) {
		iIgnore |= bits_COND_SMELL_FOOD;
	} else {
		if (InSquad() && !IsLeader()) {
			// Let leader to eat first if he is injured
			if (MySquadLeader()->pev->health < MySquadLeader()->pev->max_health) {
				iIgnore |= bits_COND_SMELL_FOOD;
			}
		}
	}
	return iIgnore;
}

int CHoundeye::DefaultISoundMask()
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_PLAYER;
}

float CHoundeye::HearingSensitivity()
{
	if (m_iAsleep == HOUNDEYE_DEEP_SLEEPING)
		return 0.6;
	return CSquadMonster::HearingSensitivity();
}

bool CHoundeye::FInViewCone(CBaseEntity *pEntity)
{
	if (m_iAsleep == HOUNDEYE_DEEP_SLEEPING)
		return false;
	return CSquadMonster::FInViewCone(pEntity);
}

void CHoundeye::TouchSleeping(CBaseEntity *pToucher)
{
	if (pToucher->MyMonsterPointer() != 0)
	{
		if (m_iAsleep == HOUNDEYE_DEEP_SLEEPING)
		{
			ClearSchedule();
			m_iAsleep = HOUNDEYE_FORCE_URGENT_WAKING;
			MakeIdealYaw( pToucher->pev->origin );
		}
		SetTouch(NULL);
		SetUse( &CBaseMonster::MonsterUse );
	}
}

void CHoundeye::UseSleeping(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_iAsleep == HOUNDEYE_DEEP_SLEEPING)
	{
		ClearSchedule();
		m_iAsleep = useType == USE_ON ? HOUNDEYE_FORCE_URGENT_WAKING : HOUNDEYE_FORCE_LAZY_WAKING;
	}
	SetTouch(NULL);
	SetUse( &CBaseMonster::MonsterUse );
}

class CDeadHoundeye : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/houndeye_dead.mdl"; }
	int	DefaultClassify() override { return CLASS_ALIEN_MONSTER; }

	const char* getPos(int pos) const override;
};

const char* CDeadHoundeye::getPos(int pos) const
{
	return "dead";
}

LINK_ENTITY_TO_CLASS( monster_houndeye_dead, CDeadHoundeye )

void CDeadHoundeye::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
}
