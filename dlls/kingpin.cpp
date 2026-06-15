#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"shake.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"
#include	"game.h"
#include	"followingmonster.h"

#define KINGPIN_AE_LEFT 1
#define KINGPIN_AE_RIGHT 2
#define KINGPIN_AE_PLASMA_START 3
#define KINGPIN_AE_PLASMA_LAUNCH 4
#define KINGPIN_AE_PLASMA_END 5

#define KINGPIN_PLASMABALL_LIFETIME 4.5
#define KINGPIN_PLASMABALL_DELAY 6
#define KINGPIN_PLASMABALL_RADIUS 190.0f
#define KINGPIN_PLASMABALL_LIMIT_SPEED 275.0f
#define KINGPIN_REDRAW_BEAMTRAIL_TIME 0.5f
#define KINGPIN_TELEPORT_INTERVAL 8.0f
#define KINGPIN_TELEPORT_DELAY 1.5f
#define KINGPIN_PLASMACLUSTER_DELAY 4
#define KINGPIN_PLASMACLUSTER_ATTACK_DISTANCE 512

enum
{
	SCHED_KINGPIN_TELEPORT = LAST_COMMON_SCHEDULE + 1,
};

enum
{
	TASK_KINGPIN_TELEPORT = LAST_COMMON_TASK + 1,
};

#define KINGPIN_PLASMA_BALL_SCALE 1.5f

class CKingpinPlasmaBall : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void Launch();
	void EXPORT HuntThink();
	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT AnimateThink();
	void Animate();
	void MovetoTarget( Vector vecTarget );
	Vector m_vecIdeal;

	static const NamedSoundScript impactSoundScript;
	static const NamedSoundScript ambienceSoundScript;

	static const NamedVisual ballVisual;
	static const NamedVisual trailVisual;
	static const NamedVisual shockVisual;
	static const NamedVisual shockInnerVisual;
	static const NamedVisual shockOuterVisual;
	static const NamedVisual entLightVisual;

protected:
	void Explode(CBaseEntity *pEnemy);
	void RemoveMySelf();
	void PlayShockWave(const Visual* visual, int type, int radius);
	void DrawTrailingBeam();

	bool m_shouldRestartSound;
};

LINK_ENTITY_TO_CLASS( kingpin_plasma_ball, CKingpinPlasmaBall )

const NamedSoundScript CKingpinPlasmaBall::impactSoundScript = {
	CHAN_STATIC,
	{"debris/beamstart14.wav"},
	0.5f,
	ATTN_NORM,
	IntRange(140, 160),
	"Kingpin.PlasmbaBall.Impact"
};

const NamedSoundScript CKingpinPlasmaBall::ambienceSoundScript = {
	CHAN_WEAPON,
	{"kingpin/kingpin_seeker_amb.wav"},
	"Kingpin.PlasmbaBall.Ambience"
};

const NamedVisual CKingpinPlasmaBall::ballVisual = BuildVisual("Kingpin.PlasmaBall")
	.Model("sprites/nhth1.spr")
	.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 255)
	.Scale(0.5f);

const NamedVisual CKingpinPlasmaBall::trailVisual = BuildVisual("Kingpin.PlasmaBall.Trail")
	.Model("sprites/smoke.spr")
	.Life(0.6f)
	.BeamWidth(12)
	.RenderColor(255, 220, 255)
	.Alpha(180);

const NamedVisual CKingpinPlasmaBall::shockVisual = BuildVisual("Kingpin.PlasmaBall.Shock")
	.Model("sprites/shockwave.spr")
	.Life(0.2f)
	.BeamWidth(32)
	.BeamNoise(10)
	.RenderColor(255, 200, 255)
	.Alpha(255);

const NamedVisual CKingpinPlasmaBall::shockInnerVisual = BuildVisual("Kingpin.PlasmaBall.ShockInner")
	.Mixin(&CKingpinPlasmaBall::shockVisual);

const NamedVisual CKingpinPlasmaBall::shockOuterVisual = BuildVisual("Kingpin.PlasmaBall.ShockOuter")
	.Mixin(&CKingpinPlasmaBall::shockVisual);

const NamedVisual CKingpinPlasmaBall::entLightVisual = BuildVisual("Kingpin.PlasmaBall.EntLight")
	.Radius(128)
	.RenderColor(128, 128, 255)
	.Life(1.0f)
	.Decay(128);

void CKingpinPlasmaBall::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	ApplyVisualWithOwn(GetVisual(ballVisual));

	UTIL_SetSize(pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );
	m_vecIdeal = Vector( 0, 0, 0 );

	SetThink(&CKingpinPlasmaBall::AnimateThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CKingpinPlasmaBall::Launch()
{
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CKingpinPlasmaBall::HuntThink );
	SetTouch( &CKingpinPlasmaBall::BounceTouch );

	EmitSoundScript(ambienceSoundScript);

	pev->dmgtime = gpGlobals->time + KINGPIN_PLASMABALL_LIFETIME;
	DrawTrailingBeam();
	pev->air_finished = gpGlobals->time + KINGPIN_REDRAW_BEAMTRAIL_TIME;
}

void CKingpinPlasmaBall::Precache()
{
	RegisterVisual(ballVisual);
	RegisterAndPrecacheSoundScript(impactSoundScript);
	RegisterAndPrecacheSoundScript(ambienceSoundScript);
	PRECACHE_SOUND( "kingpin/kingpin_seeker1.wav" );
	PRECACHE_SOUND( "kingpin/kingpin_seeker2.wav" );
	PRECACHE_SOUND( "kingpin/kingpin_seeker3.wav" );

	RegisterVisual(trailVisual);
	RegisterVisual(shockInnerVisual);
	RegisterVisual(shockOuterVisual);
	RegisterVisual(entLightVisual);
}

void CKingpinPlasmaBall::Activate()
{
	CBaseMonster::Activate();
	if (pev->velocity != g_vecZero)
		m_shouldRestartSound = true; // restart sound
}

void CKingpinPlasmaBall::HuntThink()
{
	if (m_shouldRestartSound)
	{
		EmitSoundScript(ambienceSoundScript);
		m_shouldRestartSound = false;
	}

	pev->nextthink = gpGlobals->time + 0.1f;

	// check world boundaries
	if (!IsInWorld())
	{
		RemoveMySelf();
		return;
	}

	if (gpGlobals->time > pev->dmgtime || m_hEnemy == 0 || !m_hEnemy->IsAlive())
	{
		Explode(NULL);
		return;
	}

	MovetoTarget( m_hEnemy->Center() );

	if( ( m_hEnemy->Center() - pev->origin ).Length() < 96.0f )
	{
		Explode(m_hEnemy);
		return;
	}

	if (pev->air_finished <= gpGlobals->time)
	{
		pev->air_finished = gpGlobals->time + KINGPIN_REDRAW_BEAMTRAIL_TIME;
		DrawTrailingBeam();
	}
	Animate();
}

void CKingpinPlasmaBall::AnimateThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	Animate();
}

void CKingpinPlasmaBall::Animate()
{
	pev->frame = (int)( pev->frame + 1 ) % 11;

	if (pev->scale < KINGPIN_PLASMA_BALL_SCALE)
		pev->scale += 0.1f;

	SendEntLight(entindex(), pev->origin, GetVisual(entLightVisual));
}

void CKingpinPlasmaBall::Explode(CBaseEntity* pEnemy)
{
	int classify = CLASS_NONE;
	CBaseEntity* pOwner = CBaseEntity::OwnInstance(pev->owner);
	if (pOwner)
		classify = pOwner->Classify();
	::RadiusDamage(pev->origin, pev, pOwner ? pOwner->pev : pev, DamageInfo(GetSkillValue("kingpin_plasma_blast"), DMG_SHOCK), KINGPIN_PLASMABALL_RADIUS, classify );

	PlayShockWave(GetVisual(shockInnerVisual), TE_BEAMDISK, 600);
	PlayShockWave(GetVisual(shockOuterVisual), TE_BEAMCYLINDER, 750);

	EmitSoundScriptAmbient(pev->origin, impactSoundScript);

	RemoveMySelf();
}

void CKingpinPlasmaBall::PlayShockWave(const Visual* visual, int type, int radius)
{
	SendBeamWave(pev->origin, radius, visual, MSG_PVS, pev->origin);
}

void CKingpinPlasmaBall::DrawTrailingBeam()
{
	SendBeamFollow(entindex(), GetVisual(trailVisual));
}

void CKingpinPlasmaBall::RemoveMySelf()
{
	SetTouch(NULL);
	StopSoundScript(ambienceSoundScript);
	UTIL_Remove( this );
}

void CKingpinPlasmaBall::MovetoTarget( Vector vecTarget )
{
	// accelerate
	float flSpeed = m_vecIdeal.Length();
	if( flSpeed == 0.0f )
	{
		m_vecIdeal = pev->velocity;
		flSpeed = m_vecIdeal.Length();
	}

	if( flSpeed > KINGPIN_PLASMABALL_LIMIT_SPEED )
	{
		m_vecIdeal = m_vecIdeal.Normalize() * KINGPIN_PLASMABALL_LIMIT_SPEED;
	}
	m_vecIdeal = m_vecIdeal + ( vecTarget - pev->origin ).Normalize() * 100.0f;
	pev->velocity = m_vecIdeal;
}

void CKingpinPlasmaBall::BounceTouch(CBaseEntity *pOther)
{
	if (pOther->pev->takedamage && FBitSet(pOther->pev->flags, FL_CLIENT|FL_MONSTER))
	{
		Explode(pOther);
	}
	else
	{
		Vector vecDir = m_vecIdeal.Normalize();

		TraceResult tr = UTIL_GetGlobalTrace();
		float n = -DotProduct( tr.vecPlaneNormal, vecDir );
		vecDir = 2.0 * tr.vecPlaneNormal * n + vecDir;

		m_vecIdeal = vecDir * m_vecIdeal.Length();
	}
}

#define KINGPIN_CLUSTER_SPRITE_INITIAL_AMT 50
#define KINGPIN_CLUSTER_PARTICLE_COUNT 12
#define KINGPIN_CLUSTER_PARTICLE_SPEED 280.0f
#define KINGPIN_CLUSTER_PARTICLE_SPEED_STEP 10.0f
#define KINGPIN_CLUSTER_DISPERSE_TIME 0.8f
#define KINGPIN_CLUSTER_FALL_TIME 4.5f

class CKingpinPlasmaCluster : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void UpdateOnRemove() override;

	void EXPORT StartUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT AnimateThink();
	void EXPORT DisperseThink();
	void EXPORT FallThink();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	static const NamedSoundScript startSoundScript;
	static const NamedSoundScript burstSoundScript;
	static const NamedSoundScript zapSoundScript;

	static const NamedVisual spriteVisual;
	static const NamedVisual leftoverVisual;
	static const NamedVisual beamVisual;
	static const NamedVisual beamAltVisual;
	static const NamedVisual entLightVisual;

	static const NamedVisual particleVisual;
	static const NamedVisual particleAltVisual;

private:
	void Start();
	void Explode();
	void MakeParticleBurst();
	void RemoveEffects();
	void BeamDamage(float flDamage);
	void TurnOffRandomBeams(int count);
	void MakeELight();

	float m_lastTime;
	float m_maxFrame;
	CBaseEntity* m_particles[KINGPIN_CLUSTER_PARTICLE_COUNT];
	CBeam* m_beams[KINGPIN_CLUSTER_PARTICLE_COUNT];
	float zapTime;
	float zapSoundTime;
	int lastZapSoundChannel;
};

const NamedSoundScript CKingpinPlasmaCluster::startSoundScript = {
	CHAN_BODY,
	{"debris/beamstart1.wav"},
	1.0f,
	ATTN_STATIC,
	"Kingpin.PlasmaCluster.Start"
};

const NamedSoundScript CKingpinPlasmaCluster::burstSoundScript = {
	CHAN_BODY,
	{"debris/beamstart14.wav"},
	1.0f,
	ATTN_STATIC,
	"Kingpin.PlasmaCluster.Burst"
};

const NamedSoundScript CKingpinPlasmaCluster::zapSoundScript = {
	CHAN_ITEM,
	{"debris/zap1.wav", "debris/zap3.wav", "debris/zap8.wav"},
	1.0f,
	ATTN_STATIC,
	120,
	"Kingpin.PlasmaCluster.Zap"
};

const NamedVisual CKingpinPlasmaCluster::spriteVisual = BuildVisual("Kingpin.PlasmaCluster.Sprite")
	.Model("sprites/c-tele1.spr")
	.RenderMode(kRenderTransAdd)
	.Alpha(KINGPIN_CLUSTER_SPRITE_INITIAL_AMT)
	.RenderColor(225, 50, 175)
	.Framerate(20.0f);

const NamedVisual CKingpinPlasmaCluster::leftoverVisual = BuildVisual("Kingpin.PlasmaCluster.Leftover")
	.Model("sprites/redflare2.spr")
	.RenderMode(kRenderTransAdd)
	.Alpha(255)
	.RenderColor(225, 0, 150)
	.Scale(1.5f);

const NamedVisual CKingpinPlasmaCluster::beamVisual = BuildVisual("Kingpin.PlasmaCluster.Beam")
	.Model("sprites/plasma.spr")
	.Alpha(220)
	.RenderColor(180, 70, 140)
	.BeamParams(80, 80, 30);

const NamedVisual CKingpinPlasmaCluster::beamAltVisual = BuildVisual("Kingpin.PlasmaCluster.BeamAlt")
	.RenderColor(140, 100, 225)
	.Mixin(&CKingpinPlasmaCluster::beamVisual);

const NamedVisual CKingpinPlasmaCluster::entLightVisual = BuildVisual("Kingpin.PlasmaCluster.EntLight")
	.Radius(256)
	.RenderColor(180, 70, 140)
	.Life(2.5f);

const NamedVisual CKingpinPlasmaCluster::particleVisual = BuildVisual("Kingpin.PlasmaCluster.Particle")
	.Model("sprites/flare1.spr")
	.Scale(0.4f)
	.RenderProps(kRenderTransAdd, Color3(240, 120, 200), 220);

const NamedVisual CKingpinPlasmaCluster::particleAltVisual = BuildVisual("Kingpin.PlasmaCluster.ParticleAlt")
	.Mixin(&CKingpinPlasmaCluster::particleVisual)
	.RenderColor(160, 130, 255);

LINK_ENTITY_TO_CLASS( env_plasmacluster, CKingpinPlasmaCluster )
LINK_ENTITY_TO_CLASS( kingpin_plasma_cluster, CKingpinPlasmaCluster )

TYPEDESCRIPTION	CKingpinPlasmaCluster::m_SaveData[] =
{
	DEFINE_FIELD( CKingpinPlasmaCluster, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CKingpinPlasmaCluster, m_maxFrame, FIELD_FLOAT ),
	DEFINE_ARRAY( CKingpinPlasmaCluster, m_particles, FIELD_CLASSPTR, KINGPIN_CLUSTER_PARTICLE_COUNT ),
	DEFINE_ARRAY( CKingpinPlasmaCluster, m_beams, FIELD_CLASSPTR, KINGPIN_CLUSTER_PARTICLE_COUNT ),
	DEFINE_FIELD( CKingpinPlasmaCluster, zapTime, FIELD_TIME ),
	DEFINE_FIELD( CKingpinPlasmaCluster, zapSoundTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CKingpinPlasmaCluster, CBaseEntity )

void CKingpinPlasmaCluster::Spawn()
{
	pev->solid = SOLID_NOT;
	Precache();
	if (FStringNull(pev->targetname))
		Start();
	else
		SetUse(&CKingpinPlasmaCluster::StartUse);
}

void CKingpinPlasmaCluster::Precache()
{
	RegisterVisual(spriteVisual);
	RegisterVisual(leftoverVisual);
	RegisterVisual(beamVisual);
	RegisterVisual(beamAltVisual);
	RegisterVisual(particleVisual);
	RegisterVisual(particleAltVisual);

	RegisterAndPrecacheSoundScript(startSoundScript);
	RegisterAndPrecacheSoundScript(burstSoundScript);
	RegisterAndPrecacheSoundScript(zapSoundScript);
}

void CKingpinPlasmaCluster::Start()
{
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	ApplyVisual(GetVisual(spriteVisual));
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	m_lastTime = gpGlobals->time;

	SetThink(&CKingpinPlasmaCluster::AnimateThink);
	pev->nextthink = gpGlobals->time;

	EmitSoundScript(startSoundScript);
}

void CKingpinPlasmaCluster::StartUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetUse(NULL);
	Start();
}

void CKingpinPlasmaCluster::RemoveEffects()
{
	for (int i=0; i<KINGPIN_CLUSTER_PARTICLE_COUNT; ++i) {
		UTIL_RemoveAndClean(m_particles[i]);
		UTIL_RemoveAndClean(m_beams[i]);
	}
}

void CKingpinPlasmaCluster::UpdateOnRemove()
{
	RemoveEffects();
	CBaseEntity::UpdateOnRemove();
}

void CKingpinPlasmaCluster::AnimateThink()
{
	pev->frame += pev->framerate * ( gpGlobals->time - m_lastTime );

	if (pev->frame <= m_maxFrame*0.5f)
	{
		pev->renderamt = KINGPIN_CLUSTER_SPRITE_INITIAL_AMT + (255.0f - KINGPIN_CLUSTER_SPRITE_INITIAL_AMT) * pev->frame / (m_maxFrame * 0.5f);
	}
	else
	{
		pev->renderamt = 255.0f;
	}

	if( pev->frame >= m_maxFrame )
	{
		Explode();
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		m_lastTime = gpGlobals->time;
	}
}

void CKingpinPlasmaCluster::Explode()
{
	pev->movetype = MOVETYPE_NONE;
	pev->frame = 0;

	pev->model = iStringNull;
	ApplyVisual(GetVisual(leftoverVisual), nullptr, 0);
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	MakeParticleBurst();
	pev->dmgtime = gpGlobals->time + KINGPIN_CLUSTER_DISPERSE_TIME;
	MakeELight();
	SetThink(&CKingpinPlasmaCluster::DisperseThink);
	pev->nextthink = gpGlobals->time + 0.05f;

	EmitSoundScript(burstSoundScript);
}

class CPlasmaClusterParticle : public CBaseEntity
{
public:
	void Precache()
	{
		RegisterVisual(CKingpinPlasmaCluster::particleVisual);
		RegisterVisual(CKingpinPlasmaCluster::particleAltVisual);
	}

	void Spawn()
	{
		pev->movetype = MOVETYPE_FLY;
		pev->solid = SOLID_NOT;

		ApplyVisualWithOwn(m_altColor ? GetVisual(CKingpinPlasmaCluster::particleAltVisual) : GetVisual(CKingpinPlasmaCluster::particleVisual));

		UTIL_SetSize(pev, Vector(-4,-4,-4), Vector(4,4,4));

		pev->gravity = RANDOM_FLOAT(0.09f, 0.11f);
	}

	bool m_altColor;
};

LINK_ENTITY_TO_CLASS( plasma_cluster_particle, CPlasmaClusterParticle )

void CKingpinPlasmaCluster::MakeParticleBurst()
{
	const Vector center = pev->origin;

	EntityOverrides entityOverrides;
	entityOverrides.entTemplate = m_entTemplate;
	entityOverrides.ownerEntTemplate = m_ownerEntTemplate;

	float theta = 0.0f;
	for (int i=0; i<KINGPIN_CLUSTER_PARTICLE_COUNT; ++i) {
		CPlasmaClusterParticle *pParticle = (CPlasmaClusterParticle*)CreateNoSpawn("plasma_cluster_particle", center, pev->angles, edict(), entityOverrides);
		if (pParticle)
		{
			const bool altColor = i%2 == 1;
			pParticle->m_altColor = altColor;
			if (DispatchSpawnAutoClean(pParticle))
			{
				m_particles[i] = pParticle;

				const float phi = RANDOM_FLOAT(0, M_PI_F * 0.5f);
				theta += 2.0f*M_PI_F/KINGPIN_CLUSTER_PARTICLE_COUNT;
				const Vector direction(cos(theta)*sin(phi), sin(theta)*sin(phi), cos(phi));
				pParticle->pev->velocity = direction * KINGPIN_CLUSTER_PARTICLE_SPEED;
				pParticle->pev->velocity.z /= 2.0f;

				CBeam* beam = CreateBeamFromVisual(altColor ? GetVisual(beamAltVisual) : GetVisual(beamVisual));
				if (beam)
				{
					beam->EntsInit(entindex(), pParticle->entindex());
					beam->pev->spawnflags |= SF_BEAM_TRANSIT;
					m_beams[i] = beam;
				}
			}
		}
	}
}

void CKingpinPlasmaCluster::BeamDamage(float flDamage)
{
	ClearMultiDamage();

	entvars_t *pevAttacker = pev;
	if (!FNullEnt(pev->owner))
	{
		CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
		if (pOwner)
			pevAttacker = pOwner->pev;
	}

	CBaseEntity* pLastHit = nullptr;

	for (int i=0; i<KINGPIN_CLUSTER_PARTICLE_COUNT; ++i) {
		CBaseEntity *pParticle = m_particles[i];
		CBeam* pBeam = m_beams[i];
		const bool altColor = i%2 == 1;

		if (pParticle && pBeam && pBeam->pev->renderamt > 0) {
			TraceResult tr;
			UTIL_TraceHull( pev->origin, pParticle->pev->origin, dont_ignore_monsters, head_hull, ENT(pevAttacker), &tr );
			if( tr.flFraction != 1.0f && tr.pHit ) {
				CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
				if( pHit && pHit->MyMonsterPointer() && pHit->IsFullyAlive() )
				{
					const Visual* visual = altColor ? GetVisual(beamAltVisual) : GetVisual(beamVisual);
					const int beamRenderAmt = RandomizeNumberFromRange(visual->renderamt);

					float damage = beamRenderAmt <= 0 ? 0 : ceil(flDamage * pBeam->pev->renderamt / beamRenderAmt);
					if (damage >= 2) {
						TraceResult* ptr = &tr;
						DamageInfo beamDamageInfo{damage, DMG_SHOCK};

						if (pHit == pLastHit)
							beamDamageInfo.SetNoBlood();

						pLastHit = pHit;

						TraceResult tr2;
						UTIL_TraceLine( pev->origin, pParticle->pev->origin, dont_ignore_monsters, ENT(pevAttacker), &tr2 );
						if (tr.pHit == tr2.pHit)
							ptr = &tr2;
						else
							beamDamageInfo.noBlood = true;

						pHit->TraceAttack(pev, pevAttacker, beamDamageInfo, (tr.vecEndPos - pev->origin).Normalize(), ptr);

						if (zapSoundTime < gpGlobals->time) {
							SoundScriptParamOverride paramOverride;
							if (lastZapSoundChannel == CHAN_ITEM)
							{
								lastZapSoundChannel = CHAN_WEAPON;
								paramOverride.OverrideChannel(CHAN_WEAPON);
							}
							else
							{
								lastZapSoundChannel = CHAN_ITEM;
							}
							paramOverride.OverrideVolumeRelative(0.9f);
							pParticle->EmitSoundScript(GetSoundScript(zapSoundScript), paramOverride);
							zapSoundTime = gpGlobals->time + 0.5f;
						}
					}
				}
			}
		}
	}
	ApplyMultiDamage( pev, pevAttacker );
}

void CKingpinPlasmaCluster::DisperseThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	const bool shouldFade = pev->dmgtime <= gpGlobals->time;

	for (int i=0; i<KINGPIN_CLUSTER_PARTICLE_COUNT; ++i) {
		CBaseEntity *pParticle = m_particles[i];
		if (pParticle) {
			if (pParticle->pev->movetype == MOVETYPE_FLY)
			{
				pParticle->pev->velocity = pParticle->pev->velocity - pParticle->pev->velocity.Normalize() * KINGPIN_CLUSTER_PARTICLE_SPEED_STEP;
				const float particleSpeed = pParticle->pev->velocity.Length();
				if (particleSpeed < KINGPIN_CLUSTER_PARTICLE_SPEED_STEP) {
					pParticle->pev->velocity = g_vecZero;
					pParticle->pev->movetype = MOVETYPE_TOSS;
				}
			}
			if (shouldFade)
			{
				pParticle->pev->velocity = g_vecZero;
				pParticle->pev->movetype = MOVETYPE_TOSS;
			}
		}
	}

	if (shouldFade) {
		pev->movetype = MOVETYPE_TOSS;
		pev->gravity = 0.01f;

		MakeELight();
		pev->dmgtime = gpGlobals->time + KINGPIN_CLUSTER_FALL_TIME;
		SetThink(&CKingpinPlasmaCluster::FallThink);
		pev->nextthink = gpGlobals->time;
	} else {
		BeamDamage(GetSkillValue("kingpin_lightning"));
	}
}

void CKingpinPlasmaCluster::TurnOffRandomBeams(int count)
{
	for (int i=0; i<count; ++i)
	{
		int index = RANDOM_LONG(0, KINGPIN_CLUSTER_PARTICLE_COUNT - 1);
		CBeam* beam = m_beams[index];
		if (beam) {
			beam->SetBrightness(0);
		}
	}
}

void CKingpinPlasmaCluster::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.05f;

	float coef = (pev->dmgtime - gpGlobals->time)/KINGPIN_CLUSTER_FALL_TIME;
	if (coef <= 0.0f) {
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;
		return;
	}

	pev->renderamt = coef * 255.0f;
	for (int i=0; i<KINGPIN_CLUSTER_PARTICLE_COUNT; ++i) {
		const bool altColor = i%2 == 1;

		CBaseEntity *pParticle = m_particles[i];
		if (pParticle) {
			const Visual* visual = altColor ? GetVisual(particleAltVisual) : GetVisual(particleVisual);
			if (visual)
				pParticle->pev->renderamt = coef * RandomizeNumberFromRange(visual->renderamt);
		}
		CBeam* pBeam = m_beams[i];
		if (pBeam && (pBeam->pev->renderamt > 0 || zapTime < gpGlobals->time)) {
			const Visual* visual = altColor ? GetVisual(beamAltVisual) : GetVisual(beamVisual);
			if (visual)
				pBeam->SetBrightness(coef * RandomizeNumberFromRange(visual->renderamt));
		}
	}

	if (zapTime < gpGlobals->time && coef > 0.2f) {
		if (zapSoundTime < gpGlobals->time) {
			SoundScriptParamOverride paramOverride;
			if (lastZapSoundChannel == CHAN_ITEM)
			{
				lastZapSoundChannel = CHAN_WEAPON;
				paramOverride.OverrideChannel(CHAN_WEAPON);
			}
			else
			{
				lastZapSoundChannel = CHAN_ITEM;
			}
			paramOverride.OverrideVolumeRelative(coef);

			EmitSoundScript(zapSoundScript, paramOverride);
			zapSoundTime = gpGlobals->time + 0.6f;
		}
		zapTime = gpGlobals->time + 0.3f;

		TurnOffRandomBeams(2);
	}

	BeamDamage(GetSkillValue("kingpin_lightning") * 0.5f);
}

void CKingpinPlasmaCluster::MakeELight()
{
	SendEntLight(entindex(), pev->origin + Vector(0, 0, -32), GetVisual(entLightVisual));
}

#define SF_KINGPIN_ESCAPE SF_MONSTER_SPECIAL_FLAG
#define bits_MEMORY_GOING_TO_USE_SECOND_CHANCE bits_MEMORY_CUSTOM2

class CKingpin : public CFollowingMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("kingpin"); }
	void SetYawSpeed() override { pev->yaw_speed = 140; }
	int DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Kingpin"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void KeyValue(KeyValueData *pkvd) override;
	float HeadHitGroupDamageMultiplier() override;
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	float MaximumShield() const { return pev->armortype; }

	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	void OnDying(bool gibbed) override;
	void BecomeDead() override;
	void UpdateOnRemove() override;
	int DefaultGibCount() override { return 10; }

	void IdleSound() override;
	void AlertSound() override;
	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	void DeathSound() override;
	void PlayUseSentence() override;
	void PlayUnUseSentence() override;

	bool CheckRangeAttack1(float flDot, float flDist) override { return false; }
	bool CheckRangeAttack2( float flDot, float flDist ) override;

	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType(int Type) override;
	void StartTask( Task_t *pTask ) override;
	void RunTask( Task_t *pTask ) override;
	void PrescheduleThink() override;
	void OnChangeSchedule( Schedule_t *pNewSchedule ) override;

	int SizeForGrapple() override { return GRAPPLE_LARGE; }
	Vector DefaultMinHullSize() override { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() override { return VEC_HUMAN_HULL_MAX; }
	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector( -24.0f, -24.0f, 0.0f );
		pev->absmax = pev->origin + Vector( 24.0f, 24.0f, 96.0f );
	}

	float m_plasmaBallTime;
	float m_plasmaClusterTime;
	CSprite* m_Glows[4];
	CKingpinPlasmaBall* m_plasmaBall;
	string_t m_sTeleportTarget;
	bool m_isTeleporting;
	bool m_canUseSecondChance;

	FloatRange DefaultPowerShieldStrength() override {
		return GetSkillValueRange("kingpin_shield");
	}
	FloatRange DefaultPowerShieldRegenResourceAmount() override {
		return GetSkillValueRange("kingpin_shield_reserve");
	}
	const NamedVisual& PowerShieldVisual() override {
		return shieldVisual;
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

	void ReportAIState(ALERT_TYPE level) override;
protected:
	void UpdateGlows(int target, int speed);
	void ClearGlows();
	void DeathBeams(const Visual* visual);
	void ClearPlasmaBall();
	Vector PlasmaBallPos();
	void TryMakePlasmaCluster(const Vector& pos);
	bool CanTeleportNow();

	int m_iFunnelParticle;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;

	static constexpr const char* attackHitSoundScript = "Kingpin.AttackHit";
	static constexpr const char* attackMissSoundScript = "Kingpin.AttackMiss";

	static const NamedSoundScript teleportEnterSoundScript;
	static const NamedSoundScript teleportExitSoundScript;
	static const NamedSoundScript escapeSoundScript;
	static const NamedSoundScript escapeEndSoundScript;
	static const NamedSoundScript clusterAttackSoundScript;

	static const NamedSoundScript useSoundScript;
	static const NamedSoundScript unuseSoundScript;

	static const NamedVisual shieldVisual;
	static const NamedVisual glowVisual;
	static const NamedVisual teleportVisual;
	static const NamedVisual teleportEnterVisual;
	static const NamedVisual teleportExitVisual;
	static const NamedVisual teleportTrailBeamVisual;
	static const NamedVisual escapeVisual;
	static const NamedVisual escapeEndVisual;

	static const NamedVisual deathBaseBeamVisual;
	static const NamedVisual deathBeamVisual;
	static const NamedVisual deathEndBeamVisual;
	static const NamedVisual escapeBaseBeamVisual;
	static const NamedVisual escapeBeamVisual;
	static const NamedVisual escapeEndBeamVisual;
	static const NamedVisual escapeWaveVisual;

	static const NamedVisual clusterBeamVisual;

	const Visual* m_shieldDebrisVisual;
	const Visual* m_glowVisual;
};

LINK_ENTITY_TO_CLASS( monster_kingpin, CKingpin )

TYPEDESCRIPTION	CKingpin::m_SaveData[] =
{
	DEFINE_FIELD( CKingpin, m_plasmaBallTime, FIELD_TIME ),
	DEFINE_FIELD( CKingpin, m_plasmaClusterTime, FIELD_TIME ),
	DEFINE_ARRAY( CKingpin, m_Glows, FIELD_CLASSPTR, 4 ),
	DEFINE_FIELD( CKingpin, m_plasmaBall, FIELD_CLASSPTR ),
	DEFINE_FIELD( CKingpin, m_sTeleportTarget, FIELD_STRING ),
	DEFINE_FIELD( CKingpin, m_isTeleporting, FIELD_BOOLEAN ),
	DEFINE_FIELD( CKingpin, m_canUseSecondChance, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CKingpin, CFollowingMonster )

const NamedSoundScript CKingpin::idleSoundScript = {
	CHAN_VOICE,
	{"kingpin/kingpin_idle1.wav", "kingpin/kingpin_idle2.wav", "kingpin/kingpin_idle3.wav"},
	"Kingpin.Idle"
};

const NamedSoundScript CKingpin::alertSoundScript = {
	CHAN_VOICE,
	{"kingpin/kingpin_alert1.wav", "kingpin/kingpin_alert2.wav", "kingpin/kingpin_alert3.wav"},
	"Kingpin.Alert"
};

const NamedSoundScript CKingpin::painSoundScript = {
	CHAN_VOICE,
	{"kingpin/kingpin_pain1.wav", "kingpin/kingpin_pain2.wav", "kingpin/kingpin_pain3.wav"},
	"Kingpin.Pain"
};

const NamedSoundScript CKingpin::dieSoundScript = {
	CHAN_VOICE,
	{"kingpin/kingpin_death1.wav", "kingpin/kingpin_death2.wav"},
	"Kingpin.Die"
};

const NamedSoundScript CKingpin::teleportEnterSoundScript = {
	CHAN_BODY,
	{"ambience/port_suckin1.wav"},
	"Kingpin.TeleportEnter"
};

const NamedSoundScript CKingpin::teleportExitSoundScript = {
	CHAN_BODY,
	{"debris/beamstart7.wav"},
	1.0f,
	ATTN_STATIC,
	"Kingpin.TeleportExit"
};

const NamedSoundScript CKingpin::escapeSoundScript = {
	CHAN_ITEM,
	{"ambience/particle_suck1.wav"},
	"Kingpin.Escape"
};

const NamedSoundScript CKingpin::escapeEndSoundScript = {
	CHAN_ITEM,
	{"ambience/alien_humongo.wav"},
	150,
	"Kingpin.EscapeEnd"
};

const NamedSoundScript CKingpin::clusterAttackSoundScript = {
	CHAN_ITEM,
	{"debris/beamstart10.wav"},
	0.7f,
	ATTN_NORM,
	"Kingpin.PlasmaClusterAttack"
};

const NamedSoundScript CKingpin::useSoundScript = {
	CHAN_VOICE,
	{},
	"Kingpin.Use"
};

const NamedSoundScript CKingpin::unuseSoundScript = {
	CHAN_VOICE,
	{},
	"Kingpin.UnUse"
};

const NamedVisual CKingpin::shieldVisual = BuildVisual("Kingpin.Shield")
	.RenderColor(255, 170, 255)
	.Mixin(&powerShieldRenderVisual);

const NamedVisual CKingpin::glowVisual = BuildVisual("Kingpin.Glow")
	.Model("sprites/boss_glow.spr")
	.RenderProps(kRenderTransAdd, Color3(0,0,0), 255, kRenderFxNoDissipation)
	.Scale(0.2f)
	.Framerate(10.0f);

const NamedVisual CKingpin::teleportVisual = BuildVisual("Kingpin.Teleport")
	.Model("sprites/b-tele1.spr")
	.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 255, kRenderFxNoDissipation)
	.Framerate(15.0f);

const NamedVisual CKingpin::teleportEnterVisual = BuildVisual("Kingpin.TeleportEnter")
	.Mixin(&CKingpin::teleportVisual);

const NamedVisual CKingpin::teleportExitVisual = BuildVisual("Kingpin.TeleportExit")
	.Mixin(&CKingpin::teleportVisual);

const NamedVisual CKingpin::teleportTrailBeamVisual = BuildVisual("Kingpin.TeleportTrailBeam")
	.Model("sprites/shockwave.spr")
	.RenderColor(225, 225, 255)
	.Alpha(120)
	.BeamParams(120, 64)
	.Life(0.7f);

const NamedVisual CKingpin::escapeVisual = BuildVisual("Kingpin.Escape")
	.Model("sprites/d-tele1.spr")
	.Scale(2.5)
	.RenderProps(kRenderTransAdd, Color3(170, 170, 255), 70, kRenderFxNoDissipation)
	.Framerate(10.0f);

const NamedVisual CKingpin::escapeEndVisual = BuildVisual("Kingpin.EscapeEnd")
	.Model("sprites/xflare3.spr")
	.Scale(1.25f)
	.RenderProps(kRenderTransAdd, Color3(170, 255, 127), 255, kRenderFxNoDissipation)
	.Framerate(10.0f);

const NamedVisual CKingpin::deathBaseBeamVisual = BuildVisual("Kingpin.DeathBaseBeam")
	.Model("sprites/lgtning.spr")
	.BeamNoise(80)
	.BeamScrollRate(35)
	.Framerate(10.0f)
	.Alpha(255);

const NamedVisual CKingpin::deathBeamVisual = BuildVisual("Kingpin.DeathBeam")
	.Mixin(&CKingpin::deathBaseBeamVisual)
	.RenderColor(170, 85, 127)
	.BeamWidth(70)
	.Life(2.0f);

const NamedVisual CKingpin::deathEndBeamVisual = BuildVisual("Kingpin.DeathEndBeam")
	.Mixin(&CKingpin::deathBaseBeamVisual)
	.RenderColor(170, 170, 255)
	.BeamWidth(40)
	.Life(1.0f);

const NamedVisual CKingpin::escapeBaseBeamVisual = BuildVisual("Kingpin.EscapeBaseBeam")
	.Model("sprites/lgtning.spr")
	.BeamNoise(80)
	.BeamScrollRate(35)
	.Framerate(10.0f)
	.Alpha(255);

const NamedVisual CKingpin::escapeBeamVisual = BuildVisual("Kingpin.EscapeBeam")
	.Mixin(&CKingpin::escapeBaseBeamVisual)
	.RenderColor(215, 225, 145)
	.BeamWidth(40)
	.Life(2.0f);

const NamedVisual CKingpin::escapeEndBeamVisual = BuildVisual("Kingpin.EscapeEndBeam")
	.Mixin(&CKingpin::escapeBaseBeamVisual)
	.RenderColor(75, 210, 130)
	.BeamWidth(40)
	.Life(2.0f);

const NamedVisual CKingpin::escapeWaveVisual = BuildVisual("Kingpin.EscapeWave")
	.Model("sprites/shockwave.spr")
	.Life(0.2f)
	.BeamWidth(255)
	.RenderColor(85, 255, 127)
	.Alpha(255);

const NamedVisual CKingpin::clusterBeamVisual = BuildVisual("Kingpin.PlasmaClusterBeam")
	.Model("sprites/plasma.spr")
	.Framerate(10.0f)
	.Life(1.0f)
	.BeamParams(40, 80, 10)
	.RenderColor(180, 70, 140)
	.Alpha(220);

void CKingpin::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "teleport_target"))
	{
		m_sTeleportTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq(pkvd->szKeyName, "second_chance"))
	{
		m_canUseSecondChance = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = true;
	}
	else
		CFollowingMonster::KeyValue(pkvd);
}

Task_t	tlKingpinTeleport[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_KINGPIN_TELEPORT, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slKingpinTeleport[] =
{
	{
		tlKingpinTeleport,
		ARRAYSIZE(tlKingpinTeleport),
		0,
		0,
		"Kingpin Teleport"
	},
};

Task_t tlKingpinFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)1 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slKingpinFail[] =
{
	{
		tlKingpinFail,
		ARRAYSIZE( tlKingpinFail ),
		bits_COND_CAN_ATTACK,
		0,
		"Kingpin Fail"
	},
};

Task_t tlKingpinCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 1.0f },
	{ TASK_WAIT_PVS, 0.0f },
};

Schedule_t slKingpinCombatFail[] =
{
	{
		tlKingpinCombatFail,
		ARRAYSIZE( tlKingpinCombatFail ),
		bits_COND_CAN_ATTACK,
		0,
		"Kingpin Combat Fail"
	},
};

Task_t tlKingpinRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK2, (float)0 },
};

Schedule_t slKingpinRangeAttack2[] =
{
	{
		tlKingpinRangeAttack2,
		ARRAYSIZE( tlKingpinRangeAttack2 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Kingpin Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES(CKingpin)
{
	slKingpinTeleport,
	slKingpinFail,
	slKingpinCombatFail,
	slKingpinRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES(CKingpin, CBaseMonster)

void CKingpin::Spawn()
{
	Precache();
	SetMyModel( "models/kingpin.mdl" );
	SetMySize();

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_YELLOW );
	SetMyHealth( GetSkillValue("kingpin_health") );
	SetMyFieldOfView(VIEW_FIELD_FULL);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	for (size_t i=0; i<ARRAYSIZE(m_Glows); ++i)
	{
		m_Glows[i] = CreateSpriteFromVisual(m_glowVisual, pev->origin);
		if (m_Glows[i])
		{
			m_Glows[i]->SetBrightness(0);
			m_Glows[i]->SetAttachment(edict(), i+1);
			m_Glows[i]->pev->spawnflags |= SF_SPRITE_TRANSIT;
		}
	}

	FollowingMonsterInit();
}

void CKingpin::Precache()
{
	PrecacheMyModel("models/kingpin.mdl");
	PRECACHE_MODEL("models/stickygibpink.mdl");
	PRECACHE_SOUND("kingpin/kingpin_moveslow.wav");
	PRECACHE_SOUND("kingpin/kingpin_move.wav");

	RegisterVisual(teleportEnterVisual);
	RegisterVisual(teleportExitVisual);
	RegisterAndPrecacheSoundScript(teleportEnterSoundScript);
	RegisterAndPrecacheSoundScript(teleportExitSoundScript);
	RegisterVisual(teleportTrailBeamVisual);

	RegisterVisual(escapeVisual);
	RegisterVisual(escapeEndVisual);
	RegisterAndPrecacheSoundScript(escapeSoundScript);
	RegisterAndPrecacheSoundScript(escapeEndSoundScript);

	RegisterAndPrecacheSoundScript(clusterAttackSoundScript);

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);

	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);

	RegisterAndPrecacheSoundScript(useSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript);

	RegisterVisual(deathBeamVisual);
	RegisterVisual(deathEndBeamVisual);
	RegisterVisual(escapeBeamVisual);
	RegisterVisual(escapeEndBeamVisual);
	RegisterVisual(escapeWaveVisual);

	RegisterVisual(clusterBeamVisual);

	m_iFunnelParticle = PRECACHE_MODEL( "sprites/redflare2.spr" );

	UTIL_PrecacheOther("kingpin_plasma_ball", GetProjectileOverrides());
	UTIL_PrecacheOther("kingpin_plasma_cluster", GetProjectileOverrides());

	m_glowVisual = RegisterVisual(glowVisual);
}

int CKingpin::DefaultClassify()
{
	return CLASS_GARGANTUA;
}

void CKingpin::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch ( pEvent->event ) {
	case KINGPIN_AE_LEFT:
	{
		TraceHullAttackParams params;
		params.damageInfo.damage = GetSkillValue("kingpin_melee");
		params.knockRight = 100;
		params.punchAngle.z = 18;
		params.punchAngle.x = 5;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
		PerformTraceHullAttack(params);
	}
		break;
	case KINGPIN_AE_RIGHT:
	{
		TraceHullAttackParams params;
		params.damageInfo.damage = GetSkillValue("kingpin_melee");
		params.knockRight = -100;
		params.punchAngle.z = -18;
		params.punchAngle.x = 5;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
		PerformTraceHullAttack(params);
	}
		break;
	case KINGPIN_AE_PLASMA_START:
	{
		m_plasmaBall = (CKingpinPlasmaBall*)Create("kingpin_plasma_ball", PlasmaBallPos(), pev->angles, edict(), GetProjectileOverrides());
	}
		break;
	case KINGPIN_AE_PLASMA_LAUNCH:
		if (m_plasmaBall)
		{
			UTIL_MakeAimVectors(pev->angles);
			m_plasmaBall->m_hEnemy = m_hEnemy;
			m_plasmaBall->pev->velocity = gpGlobals->v_forward * 32;
			m_plasmaBall->Launch();
			m_plasmaBall = 0;
			m_plasmaBallTime = gpGlobals->time + KINGPIN_PLASMABALL_DELAY;
		}
		break;
	case KINGPIN_AE_PLASMA_END:
		break;
	default:
		CFollowingMonster::HandleAnimEvent(pEvent);
		break;
	}
}

float CKingpin::HeadHitGroupDamageMultiplier()
{
	const float kingpinMultiplier = GetSkillValue("kingpin_head");
	const float defaultMultiplier = CFollowingMonster::HeadHitGroupDamageMultiplier();
	if (kingpinMultiplier > 0.0f)
		return Q_min(defaultMultiplier, kingpinMultiplier);
	else
		return defaultMultiplier;
}

void CKingpin::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	if (m_isTeleporting || pev->takedamage == DAMAGE_NO)
		return;

	CFollowingMonster::TraceAttack(pevInflictor, pevAttacker, inputDamageInfo, vecDir, ptr);
}

TakeDamageResult CKingpin::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	if (m_isTeleporting)
		return TakeDamageResult{};

	return CFollowingMonster::TakeDamage(pevInflictor, pevAttacker, damageInfo);
}

KilledResult CKingpin::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	// Never gib, always wait for death animation
	return CFollowingMonster::Killed( pevInflictor, pevAttacker, GIB_NEVER );
}

void CKingpin::OnDying(bool gibbed)
{
	ClearPlasmaBall();
	ClearGlows();
	CFollowingMonster::OnDying(gibbed);
}

void CKingpin::BecomeDead()
{
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_TOSS;
}

void CKingpin::UpdateOnRemove()
{
	ClearPlasmaBall();
	ClearGlows();
	CFollowingMonster::UpdateOnRemove();
}

bool CKingpin::CheckRangeAttack2(float flDot, float flDist)
{
	if (m_plasmaBallTime > gpGlobals->time)
		return false;
	return CFollowingMonster::CheckRangeAttack2(flDot, flDist);
}

Schedule_t* CKingpin::GetSchedule()
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
		{
			if (CanTeleportNow())
			{
				return GetScheduleOfType(SCHED_KINGPIN_TELEPORT);
			}
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}
	switch(m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
	{
		Schedule_t* utilitySchedule = GetUtilitySchedule();
		if (utilitySchedule)
			return utilitySchedule;
	}
		break;
	default:
		break;
	}

	return CFollowingMonster::GetSchedule();
}

extern Schedule_t slChaseEnemyFailed[];

Schedule_t* CKingpin::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_ALERT_SMALL_FLINCH:
	case SCHED_ALERT_BIG_FLINCH:
	case SCHED_SMALL_FLINCH:
	{
		if( HasConditions ( bits_COND_HEAR_SOUND ) )
		{
			return GetScheduleOfType( SCHED_ALERT_FACE );
		}
		else
		{
			return GetScheduleOfType( SCHED_ALERT_STAND );
		}
	}
	case SCHED_TAKE_COVER_FROM_ORIGIN:
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_SPOT);
	case SCHED_FAIL:
		{
			if (m_hEnemy != 0)
				return slKingpinCombatFail;
			else
				return slKingpinFail;
		}
		break;
	case SCHED_RANGE_ATTACK2:
		return slKingpinRangeAttack2;
	case SCHED_CHASE_ENEMY_FAILED:
		if (CanTeleportNow())
		{
			return GetScheduleOfType(SCHED_KINGPIN_TELEPORT);
		}
		else
		{
			m_failSchedule = SCHED_FREEROAM;
			return slChaseEnemyFailed;
		}
		break;
	case SCHED_KINGPIN_TELEPORT:
		return slKingpinTeleport;
	default:
		break;
	}
	return CFollowingMonster::GetScheduleOfType(Type);
}

void CKingpin::StartTask( Task_t *pTask )
{
	ClearPlasmaBall();
	switch( pTask->iTask )
	{
	case TASK_KINGPIN_TELEPORT:
		{
			m_IdealActivity = ACT_DIESIMPLE;
			m_isTeleporting = true;
			RemovePowerShield();

			pev->rendermode = kRenderTransAdd;
			pev->renderfx = kRenderFxPulseFastWide;
			pev->renderamt = 225;

			m_flWaitFinished = gpGlobals->time + KINGPIN_TELEPORT_DELAY;
			CSprite* enterTeleportSprite = CreateSpriteFromVisual(GetVisual(teleportEnterVisual), Center()+Vector(0,0,pev->size.z/2));
			if (enterTeleportSprite)
			{
				enterTeleportSprite->AnimateAndDie(enterTeleportSprite->pev->framerate);
				enterTeleportSprite->EmitSoundScript(GetSoundScript(teleportEnterSoundScript));
			}
			break;
		}
	case TASK_DIE:
		pev->framerate = 0.5;
		if (FBitSet(pev->spawnflags, SF_KINGPIN_ESCAPE))
		{
			DeathBeams(GetVisual(escapeBeamVisual));

			CSprite* escapeSprite = CreateSpriteFromVisual(GetVisual(escapeVisual), Center()+Vector(0,0,pev->size.z/2));
			if (escapeSprite)
			{
				escapeSprite->AnimateAndDie(escapeSprite->pev->framerate);
				escapeSprite->EmitSoundScript(GetSoundScript(escapeSoundScript));
			}
		}
		else
		{
			DeathBeams(GetVisual(deathBeamVisual));
		}
		m_flWaitFinished = gpGlobals->time + 2.0;
		// FALL THROUGH
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

void CKingpin::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_KINGPIN_TELEPORT:
		{
			if (!IsFullyAlive())
			{
				TaskFail("died, can't teleport");
			}
			else if( gpGlobals->time >= m_flWaitFinished )
			{
				m_IdealActivity = ACT_IDLE;
				if (FStringNull(m_sTeleportTarget))
				{
					TaskFail("no teleport target defined");
				}
				else
				{
					CBaseEntity* pEntity = NULL;
					CBaseEntity* pBestSpot = NULL;
					int bestDistance = 8192;
					while( ( pEntity = UTIL_FindEntityByTargetname( pEntity, STRING(m_sTeleportTarget) ) ) != NULL )
					{
						TraceResult tr;
						UTIL_TraceHull( pEntity->pev->origin + Vector(0,0,pev->size.z/2.0f), pEntity->pev->origin + Vector(0,0,pev->size.z/2.0f+1), dont_ignore_monsters, human_hull, edict(), &tr );
						if (!tr.fStartSolid && !tr.fAllSolid)
						{
							int distance;
							int distanceToSpot = (pev->origin - pEntity->pev->origin).Length();
							if (m_hEnemy != 0)
							{
								distance = (m_hEnemy->pev->origin - pEntity->pev->origin).Length();
							}
							else
							{
								distance = distanceToSpot;
							}

							if (distanceToSpot >= 192 && distance < bestDistance)
							{
								pBestSpot = pEntity;
								bestDistance = distance;
							}
						}
					}
					if (pBestSpot)
					{
						pev->flags &= ~FL_ONGROUND;
						pev->effects |= EF_NOINTERP;
						if (m_hEnemy != 0)
						{
							pev->angles.y = pev->ideal_yaw = UTIL_VecToYaw( m_hEnemy->pev->origin - pBestSpot->pev->origin );
						}
						const Vector startBeamPos = Center()+Vector(0,0,pev->size.z/4);
						UTIL_SetOrigin(pev, pBestSpot->pev->origin + Vector(0,0,1));
						const Vector endBeamPos = Center()+Vector(0,0,pev->size.z/4);

						SendBeam(startBeamPos, endBeamPos, GetVisual(teleportTrailBeamVisual));

						CSprite* exitTeleportSprite = CreateSpriteFromVisual(GetVisual(teleportExitVisual), Center()+Vector(0,0,pev->size.z/4));
						if (exitTeleportSprite)
						{
							exitTeleportSprite->AnimateAndDie(exitTeleportSprite->pev->framerate);
							exitTeleportSprite->EmitSoundScript(GetSoundScript(teleportExitSoundScript));
						}

						if (m_canUseSecondChance && HasMemory( bits_MEMORY_GOING_TO_USE_SECOND_CHANCE ))
						{
							ALERT(at_aiconsole, "%s used second chance. Health: %f. Armor: %f\n", STRING(pev->classname), pev->health, pev->armorvalue);
							m_canUseSecondChance = false;
							pev->armorvalue = MaximumPowerShield();
						}

						UTIL_ScreenShake( pev->origin, 6.0f, 50.0f, 1.0f, 400 );

						m_plasmaBallTime = Q_max(gpGlobals->time + 0.5f, m_plasmaBallTime);
						m_plasmaClusterTime = Q_max(gpGlobals->time + 0.5f, m_plasmaClusterTime);

						TaskComplete();
					}
					else
					{
						TaskFail("could not find a valid teleport spot");
					}
				}
			}
		}
		break;
	case TASK_DIE:
		if( gpGlobals->time > m_flWaitFinished )
		{
			if ( FBitSet(pev->spawnflags, SF_KINGPIN_ESCAPE) )
			{
				DeathBeams(GetVisual(escapeEndBeamVisual));
				StopAnimation();

				CSprite* escapeFlare = CreateSpriteFromVisual(GetVisual(escapeEndVisual), Center()+Vector(0,0,pev->size.z/2));
				if (escapeFlare)
				{
					escapeFlare->AnimateAndDie(escapeFlare->pev->framerate);
					escapeFlare->EmitSoundScript(GetSoundScript(escapeEndSoundScript));
				}

				RadiusDamage(pev, pev, DamageInfo{GetSkillValue("kingpin_plasma_blast")/2, DMG_SHOCK}, Classify());
				UTIL_ScreenFadeAll( pev->origin, Vector(85, 255, 127), 2.0f, 0.0f, 200, FFADE_IN );

				const Vector shockWavePos = Center();
				SendBeamWave(shockWavePos, 1500, GetVisual(escapeWaveVisual), MSG_PVS, pev->origin);

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_LARGEFUNNEL );
					WRITE_COORD( shockWavePos.x );
					WRITE_COORD( shockWavePos.y );
					WRITE_COORD( shockWavePos.z );
					WRITE_SHORT( m_iFunnelParticle );
					WRITE_SHORT( 1 );
				MESSAGE_END();

				SetThink( &CBaseEntity::SUB_Remove );
				pev->nextthink = gpGlobals->time;
			}
			else
			{
				DeathBeams(GetVisual(deathEndBeamVisual));
				StopAnimation();
				g_vecAttackDir = Vector(0,0,-1);
				GibMonster();
				RadiusDamage(pev, pev, DamageInfo{GetSkillValue("kingpin_plasma_blast"), DMG_SHOCK}, Classify());
				m_bloodColor = BLOOD_COLOR_RED; // HACK to change blood color of pink gibs
				CGib::SpawnRandomGibs( pev, DefaultGibCount(), "models/stickygibpink.mdl" );
			}
			return;
		}
		else
			CFollowingMonster::RunTask( pTask );
		break;
	default:
		CFollowingMonster::RunTask( pTask );
		break;
	}
}

void CKingpin::PrescheduleThink()
{
	int glowAmtStep = (m_glowVisual->renderamt.Middle())/10 + 1;
	int targetGlowAmt = 0;

	if (m_isTeleporting)
	{
		targetGlowAmt = 0;
	}
	else if (m_IdealMonsterState == MONSTERSTATE_ALERT || m_IdealMonsterState == MONSTERSTATE_HUNT)
	{
		targetGlowAmt = RandomizeNumberFromRange(m_glowVisual->renderamt) / 6;
	}
	else if (m_IdealMonsterState == MONSTERSTATE_COMBAT)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			targetGlowAmt = RandomizeNumberFromRange(m_glowVisual->renderamt);
		}
		else
		{
			targetGlowAmt = RandomizeNumberFromRange(m_glowVisual->renderamt) / 4;
		}
	}
	UpdateGlows(targetGlowAmt, glowAmtStep);

	if (IsMoving())
	{
		ClearPlasmaBall();
	}
	if (m_plasmaBall)
	{
		UTIL_SetOrigin(m_plasmaBall->pev, PlasmaBallPos());
	}

	if (IsFullyAlive())
	{
		if (!m_isTeleporting && m_canUseSecondChance &&
				pev->health <= pev->max_health / 2 && pev->armorvalue <= MaximumShield()/4 &&
				CanTeleportNow())
		{
			Remember(bits_MEMORY_GOING_TO_USE_SECOND_CHANCE);
			ChangeSchedule( GetScheduleOfType(SCHED_KINGPIN_TELEPORT) );
		} else if (m_plasmaClusterTime <= gpGlobals->time) {
			CBaseEntity* pEnemy = m_hEnemy;
			if (pEnemy)
			{
				Vector enemyVelocity = pEnemy->pev->velocity * 0.75f;
				enemyVelocity.z = 0.0f;
				const Vector checkPos = pEnemy->pev->origin + enemyVelocity;
				if ((checkPos - pev->origin).Length() < KINGPIN_PLASMACLUSTER_ATTACK_DISTANCE )
					TryMakePlasmaCluster(pEnemy->EyePosition() + enemyVelocity + Vector(0,0,32));
			}
		}
	}

	CFollowingMonster::PrescheduleThink();
}

void CKingpin::OnChangeSchedule(Schedule_t *pNewSchedule)
{
	if (pNewSchedule == slKingpinTeleport) {
		pev->teleport_time = gpGlobals->time + KINGPIN_TELEPORT_INTERVAL;
	}
	if (m_isTeleporting)
	{
		m_isTeleporting = false;
		pev->rendermode = kRenderNormal;
		pev->renderfx = kRenderFxNone;
	}
	CFollowingMonster::OnChangeSchedule(pNewSchedule);
}

void CKingpin::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

void CKingpin::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

PainSoundRule CKingpin::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.chance = 1.0f / 3.0f;
	return rule;
}

void CKingpin::PainSound()
{
	EmitSoundScript(painSoundScript);
}

void CKingpin::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

void CKingpin::PlayUseSentence()
{
	EmitSoundScript(useSoundScript);
}

void CKingpin::PlayUnUseSentence()
{
	EmitSoundScript(unuseSoundScript);
}

void CKingpin::UpdateGlows(int target, int speed)
{
	for (CSprite* glow : m_Glows)
	{
		if (glow)
		{
			glow->pev->renderamt = UTIL_Approach(target, glow->pev->renderamt, speed);
		}
	}
}

void CKingpin::ClearGlows()
{
	for (CSprite*& glow : m_Glows)
	{
		UTIL_RemoveAndClean(glow);
	}
}

void CKingpin::DeathBeams(const Visual* visual)
{
	int iTimes = 0;
	int iDrawn = 0;
	const int iBeams = 8;
	while (iDrawn < iBeams && iTimes < ( iBeams * 3 ))
	{
		TraceResult tr;
		const Vector vecOrigin = Center() + Vector(0,0,pev->size.z * 0.5);
		const Vector vecDest = 1024 * ( Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize() );
		UTIL_TraceLine( vecOrigin, vecOrigin + vecDest, ignore_monsters, ENT( pev ), &tr );
		if( tr.flFraction != 1.0 )
		{
			// we hit something.
			iDrawn++;
			SendBeam(vecOrigin, tr.vecEndPos, visual);
		}
		iTimes++;
	}
}

void CKingpin::ClearPlasmaBall()
{
	UTIL_RemoveAndClean(m_plasmaBall);
}

Vector CKingpin::PlasmaBallPos()
{
	UTIL_MakeVectors(pev->angles);
	Vector vecStart, angleGun;
	GetAttachment( 0, vecStart, angleGun );
	vecStart.z -= 16;
	vecStart = vecStart + gpGlobals->v_forward * 16;
	return vecStart;
}

void CKingpin::ReportAIState(ALERT_TYPE level)
{
	CFollowingMonster::ReportAIState(level);
	if (m_plasmaBallTime <= gpGlobals->time)
		ALERT(level, "Can throw an energy ball; ");
	if (m_plasmaClusterTime <= gpGlobals->time)
		ALERT(level, "Can create plasma cluster; ");
}

void CKingpin::TryMakePlasmaCluster(const Vector &pos)
{
	m_plasmaClusterTime = gpGlobals->time + 0.5f;

	const float checkDistance = 32.0f;

	const Vector eyePosition = EyePosition();
	const Vector dir = (pos - eyePosition).Normalize();

	TraceResult tr;
	UTIL_TraceHull(eyePosition, pos + dir * checkDistance, ignore_monsters, head_hull, edict(), &tr);
	if (tr.flFraction == 1.0f)
	{
		UTIL_TraceHull(pos, pos + Vector(0,0,checkDistance), ignore_monsters, head_hull, edict(), &tr);
		if (tr.flFraction == 1.0f)
		{
			CBaseEntity::Create("kingpin_plasma_cluster", pos, g_vecZero, edict(), GetProjectileOverrides());
			m_plasmaClusterTime = gpGlobals->time + KINGPIN_PLASMACLUSTER_DELAY;

			float minDistanceFromEye = 4096.0f;
			int eyeAttachmentIndex = 0;
			for (size_t i=0; i<ARRAYSIZE(m_Glows); ++i)
			{
				CSprite* glow = m_Glows[i];
				if (glow)
				{
					float distanceFromEye = (glow->pev->origin - pos).Length();
					if (distanceFromEye < minDistanceFromEye) {
						minDistanceFromEye = distanceFromEye;
						eyeAttachmentIndex = i+1;
					}
				}
			}

			if (eyeAttachmentIndex > 0)
			{
				EmitSoundScript(clusterAttackSoundScript);
				SendBeam(entindex() + 0x1000 * eyeAttachmentIndex, pos, GetVisual(clusterBeamVisual));
			}
		}
	}
}

bool CKingpin::CanTeleportNow()
{
	return pev->teleport_time <= gpGlobals->time && !FStringNull(m_sTeleportTarget);
}
