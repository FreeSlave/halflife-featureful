#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "combat.h"
#include "effects.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "shake.h"
#include "displacerball.h"
#include "scripted.h"
#include "visuals_utils.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

LINK_ENTITY_TO_CLASS(displacer_ball, CDisplacerBall)

TYPEDESCRIPTION	CDisplacerBall::m_SaveData[] =
{
	DEFINE_FIELD(CDisplacerBall, m_iBeams, FIELD_INTEGER),
	DEFINE_ARRAY(CDisplacerBall, m_pBeam, FIELD_CLASSPTR, 8),
	DEFINE_FIELD(CDisplacerBall, m_hDisplacedTarget, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CDisplacerBall, CBaseEntity);

const NamedVisual CDisplacerBall::spriteVisual = BuildVisual("DisplacerBall.Sprite")
		.Model("sprites/exit1.spr")
		.Alpha(255)
		.RenderMode(kRenderTransAdd)
		.Scale(0.75f);

const NamedVisual CDisplacerBall::armBeamVisual = BuildVisual("DisplacerBall.ArmBeam")
		.Model("sprites/lgtning.spr")
		.BeamParams(30, 80)
		.RenderColor(96, 128, 16)
		.Alpha(255);

const NamedVisual CDisplacerBall::hitBeamVisual = BuildVisual("DisplacerBall.HitBeam")
		.Model("sprites/lgtning.spr")
		.BeamParams(30, 10)
		.RenderColor(255, 255, 255)
		.Alpha(255);

const NamedVisual CDisplacerBall::ringVisual = BuildVisual("DisplacerBall.Ring")
		.Model("sprites/disp_ring.spr")
		.Life(0.3f)
		.BeamParams(36, 0)
		.RenderColor(255, 255, 255)
		.Alpha(255);

const NamedVisual CDisplacerBall::lightVisual = BuildVisual("DisplacerBall.Light")
		.Radius(160)
		.RenderColor(255, 180, 96)
		.Life(1.0f)
		.Decay(100.0f);

const NamedSoundScript CDisplacerBall::impactSoundScript = {
	CHAN_WEAPON,
	{"weapons/displacer_impact.wav"},
	FloatRange(0.8f, 0.9f),
	ATTN_NORM,
	"DisplacerBall.Impact"
};

const NamedSoundScript CDisplacerBall::explodeSoundScript = {
	CHAN_WEAPON,
	{"weapons/displacer_teleport.wav"},
	FloatRange(0.8f, 0.9f),
	ATTN_NORM,
	"DisplacerBall.Explode"
};

void CDisplacerBall::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("displacer_ball");

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	ApplyVisualWithOwn(GetVisual(spriteVisual));

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->frame = 0;

	SetTouch(&CDisplacerBall::BallTouch);
	SetThink(&CDisplacerBall::FlyThink);
	pev->nextthink = gpGlobals->time + 0.2f;

	m_iBeams = 0;

	SetDefaultProjectileDamage(GetSkillValue("plr_displacer_other"));
}

void CDisplacerBall::Precache()
{
	RegisterVisualAsMineOwn(spriteVisual);
	RegisterVisual(armBeamVisual);
	RegisterVisual(hitBeamVisual);
	RegisterVisual(ringVisual);
	RegisterVisual(lightVisual);

	RegisterAndPrecacheSoundScript(impactSoundScript);
	RegisterAndPrecacheSoundScript(explodeSoundScript);

	PRECACHE_SOUND("weapons/displacer_self.wav");
}

void CDisplacerBall::FlyThink()
{
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::ArmBeam( int iSide )
{
	//This method is identical to the Alien Slave's ArmBeam, except it treats m_pBeam as a circular buffer.
	if( m_iBeams >= static_cast<int>(ARRAYSIZE(m_pBeam)) )
		m_iBeams = 0;

	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = gpGlobals->v_forward * 32.0 + iSide * gpGlobals->v_right * 16.0 + gpGlobals->v_up * 36.0 + pev->origin;
	Vector vecAim = gpGlobals->v_up * RANDOM_FLOAT( -1.0, 1.0 );
	Vector vecEnd = (iSide * gpGlobals->v_right * RANDOM_FLOAT( 0.0, 1.0 ) + vecAim) * 512.0 + vecSrc;
	UTIL_TraceLine( &vecSrc.x, &vecEnd.x, dont_ignore_monsters, ENT( pev ), &tr );

	if( flDist > tr.flFraction )
		flDist = tr.flFraction;

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	CBaseEntity* pHit = Instance( tr.pHit );
	const bool hitSomething = pHit && pHit->pev->takedamage != DAMAGE_NO;

	// The beam might already exist if we've created all beams before.
	if( !m_pBeam[m_iBeams] )
	{
		m_pBeam[m_iBeams] = CreateBeamFromVisual(hitSomething ? GetVisual(hitBeamVisual) : GetVisual(armBeamVisual));
	}

	if( !m_pBeam[m_iBeams] )
		return;

	if( hitSomething )
	{
		//Beam hit something, deal radius damage to it
		m_pBeam[m_iBeams]->EntsInit( pHit->entindex(), entindex() );
		RadiusDamage( tr.vecEndPos, pev, VARS(pev->owner), DamageInfo{GetSkillValue("displacer_beam_dmg"), DMG_ENERGYBEAM}, GetSkillValue("displacer_beam_radius"), CLASS_NONE );
	}
	else
	{
		m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	}
	m_iBeams++;
}

void CDisplacerBall::LaunchAsProjectile(const ProjectileParameters& params)
{
	LaunchAsProjectileImpl(DISPLACERBALL_SPEED, params);
}

void CDisplacerBall::SelfCreate(entvars_t *pevOwner,Vector vecStart)
{
	CDisplacerBall *pSelf = GetClassPtr((CDisplacerBall *)NULL);
	UTIL_SetOrigin(pSelf->pev, vecStart);
	pSelf->Spawn();
	pSelf->ClearBeams();

	pSelf->pev->owner = ENT(pevOwner);
	pSelf->Circle();
	pSelf->SetTouch( NULL );
	pSelf->SetThink(&CDisplacerBall::KillThink);
	pSelf->pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2f : 0.5f );
}

void CDisplacerBall::BallTouch(CBaseEntity *pOther)
{
	pev->velocity = g_vecZero;

	CBaseEntity *pTarget = NULL;

	EmitSoundScript(impactSoundScript);

	if( ( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() ) && pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		pTarget = GetClassPtr( (CBaseEntity *)VARS( EntSelectSpawnPoint( pPlayer ) ) );


		if( pTarget )
		{
			// UTIL_ScreenFade( pPlayer, Vector( 0, 200, 0 ), 0.5, 0.5, 255, FFADE_IN );
			Vector tmp = pTarget->pev->origin;
			UTIL_CleanSpawnPoint( tmp, 100 );

			EMIT_SOUND( pPlayer->edict(), CHAN_BODY, "weapons/displacer_self.wav", 1, ATTN_NORM );

			// make origin adjustments (origin in center, not at feet)
			tmp.z -= pPlayer->pev->mins.z + 36;
			tmp.z++;

			pPlayer->pev->flags &= ~FL_ONGROUND;

			UTIL_SetOrigin( pPlayer->pev, tmp );

			pPlayer->pev->angles = pTarget->pev->angles;

			pPlayer->pev->v_angle = pTarget->pev->angles;

			pPlayer->pev->fixangle = 1;

			pPlayer->pev->velocity = pOther->pev->basevelocity = g_vecZero;
		}
	}

	pev->movetype = MOVETYPE_NONE;

	Circle();

	m_hDisplacedTarget = 0;

	if (pOther && pOther->IsDisplaceable())
	{
		m_hDisplacedTarget = pOther;

		CBaseMonster* pMonster = pOther->MyMonsterPointer();
		if (!pMonster || !pMonster->m_pCine || pMonster->m_pCine->CanInterrupt())
		{
			CBaseEntity* pAttacker = CBaseEntity::Instance( pev->owner );

			if (pMonster && pMonster->m_pCine)
				pMonster->m_pCine->CancelScript(SCRIPT_CANCELLATION_REASON_INTERRUPTED);

			pOther->pev->health = 0;
			pOther->Killed(pev, pAttacker ? pAttacker->pev : pev, GIB_NEVER);
		}
	}

	pev->basevelocity = g_vecZero;
	pev->velocity = g_vecZero;

	SetTouch(NULL);
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CDisplacerBall::KillThink);
	pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2f : 0.5f );
}

void CDisplacerBall::Circle()
{
	const Visual* visual = GetVisual(ringVisual);
	if (visual->modelIndex)
	{
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
			WRITE_BYTE(TE_BEAMCYLINDER);
			WRITE_CIRCLE(pev->origin, 800.0f);
			WriteBeamVisual(visual);
		MESSAGE_END();
	}

	SendDynLight(pev->origin, GetVisual(lightVisual));
}

void CDisplacerBall::KillThink()
{
	CBaseEntity* pTarget = m_hDisplacedTarget;
	if (pTarget)
	{
		CBaseMonster* pMonster = pTarget->MyMonsterPointer();
		if (!pMonster || !pMonster->m_pCine || pMonster->m_pCine->CanInterrupt()) {
			pTarget->SetThink(&CBaseEntity::SUB_Remove);
		}
	}

	SetThink( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}

void CDisplacerBall::ExplodeThink()
{
	ClearBeams();

	pev->effects |= EF_NODRAW;

	EmitSoundScript(explodeSoundScript);

	CBaseEntity* pAttacker = CBaseEntity::Instance( pev->owner );
	pev->owner = NULL;

	::RadiusDamage( pev->origin, pev, pAttacker ? pAttacker->pev : pev, DamageInfo(GetProjectileDamage(), DMG_BLAST).SetGibPolicy(GIB_ALWAYS), GetSkillValue("plr_displacer_radius"), CLASS_NONE );

	UTIL_Remove( this );
}

void CDisplacerBall::ClearBeams()
{
	for( int i = 0;i < 8; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
}
