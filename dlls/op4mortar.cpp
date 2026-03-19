#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "combat.h"
#include "explode.h"
#include "ggrenade.h"
#include "global_models.h"
#include "monsters.h"
#include "player.h"
#include "soundent.h"
#include "decals.h"
#include "visuals_utils.h"

class CMortarShell : public CGrenade
{
public:
	void Precache() override;
	void EXPORT BurnThink();
	void EXPORT MortarExplodeTouch(CBaseEntity *pOther);
	void Spawn() override;
	void EXPORT FlyThink();

	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	int FireballDeciScaleFromDamage(float dmg) override {
		int result = (dmg - Q_min(50.0f, dmg/2)) * 0.8f;
		return clamp(result, 1, 255);
	}
	int FireballFramerate() override {
		return 10;
	}

	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_iSoundedOff;
	float m_flIgniteTime;
	float m_dangerSoundTime;

	static const NamedSoundScript flySoundScript;
	static const NamedVisual trailVisual;
};

LINK_ENTITY_TO_CLASS(mortar_shell, CMortarShell)

TYPEDESCRIPTION CMortarShell::m_SaveData[] =
{
	DEFINE_FIELD(CMortarShell, m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(CMortarShell, m_iSoundedOff, FIELD_BOOLEAN),
	DEFINE_FIELD(CMortarShell, m_dangerSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CMortarShell, CGrenade)

const NamedSoundScript CMortarShell::flySoundScript = {
	CHAN_VOICE,
	{"weapons/ofmortar.wav"},
	FloatRange(0.8f, 0.9f),
	ATTN_NONE,
	"Op4Mortar.Fly"
};

const NamedVisual CMortarShell::trailVisual = BuildVisual::Spray("Op4Mortar.Trail")
	.Model("sprites/wep_smoke_01.spr");

void CMortarShell::Precache()
{
	PrecacheBaseGrenadeSounds();
	PrecacheMyModel("models/mortarshell.mdl");
	RegisterVisual(trailVisual);
	RegisterAndPrecacheSoundScript(flySoundScript);
}

void CMortarShell::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetMyModel("models/mortarshell.mdl");

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CMortarShell::BurnThink);
	SetTouch(&CMortarShell::MortarExplodeTouch);

	pev->gravity = 1;

	SetDefaultProjectileDamage(GetSkillValue("op4mortar"));

	pev->nextthink = gpGlobals->time + 0.01f;
	m_flIgniteTime = gpGlobals->time;
	m_iSoundedOff = false;
}

void CMortarShell::MortarExplodeTouch(CBaseEntity *pOther)
{
	pev->enemy = pOther->edict();

	const Vector direction = pev->velocity.Normalize();
	const Vector vecSpot = pev->origin - direction * 32;

	TraceResult tr;
	UTIL_TraceLine(vecSpot, vecSpot + direction * 64, ignore_monsters, edict(), &tr);

	Explode(&tr, DMG_BLAST);
}

void CMortarShell::BurnThink()
{
	pev->angles = UTIL_VecToAngles(pev->velocity);

	pev->angles.x -= 90;

	SendSpray(pev->origin, Vector(0,0,1), GetVisual(trailVisual), 1, 12, 120);

	if (gpGlobals->time > m_flIgniteTime + 0.2f)
	{
		SetThink(&CMortarShell::FlyThink);
		m_dangerSoundTime = gpGlobals->time + 0.5f;
	}

	pev->nextthink = gpGlobals->time + 0.01f;
}

void CMortarShell::FlyThink()
{
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->angles.x -= 90.0f;

	if(pev->velocity.z < 20.0f && !m_iSoundedOff)
	{
		m_iSoundedOff = true;
		EmitSoundScript(flySoundScript);
	}

	if (m_dangerSoundTime <= gpGlobals->time)
	{
		InsertAISound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5f, GetProjectileDamage() * DEFAULT_EXPLOSION_RADIUS_MULTIPLIER, 0.2f );
		m_dangerSoundTime = gpGlobals->time + 0.2f;
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CMortarShell::LaunchAsProjectile(const ProjectileParameters &params)
{
	const float speed = params.speedOverride ? params.speedOverride : 600.0f;

	if (params.variant == 1) // spawned by op4mortar
	{
		UTIL_MakeVectors(pev->angles);
		pev->velocity = -(gpGlobals->v_forward * speed);
	}
	else
	{
		pev->velocity = params.direction * speed;
		pev->angles = UTIL_VecToAngles(pev->velocity);
		pev->angles.x -= 90.0f;
	}

	SetMyProjectileEffectFlags();
}

#define SF_MORTAR_ACTIVE (1 << 0)
#define SF_MORTAR_LINE_OF_SIGHT (1 << 4)
#define SF_MORTAR_CONTROLLABLE (1 << 5)

class COp4Mortar : public CBaseMonster
{
public:
	void Spawn() override;
	TakeDamageResult TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void Precache() override;
	void KeyValue(KeyValueData *pvkd) override;
	void UpdatePosition(int direction, int controller);
	void AIUpdatePosition();
	int IRelationship( CBaseEntity* pTarget ) override;
	CBaseEntity *FindTarget();
	void EXPORT MortarThink();
	int ObjectCaps() override { return 0; }
	void PlaySound();
	void CreateMortarProjectile(CBaseEntity* pOwner, float speed);

	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int d_x;
	int d_y;
	float m_lastupdate;
	int m_direction;
	Vector m_start;
	Vector m_end;
	int m_velocity;
	int m_hmin;
	int m_hmax;
	float m_fireLast;
	float m_maxRange;
	float m_minRange;
	float m_fireDelay;
	float m_trackDelay;
	float m_zeroYaw;
	Vector m_vGunAngle;
	Vector m_vIdealGunVector;
	Vector m_vIdealGunAngle;

	float m_enemyRecheckTime;
	float m_lastTimePlayedSound;

	static const NamedSoundScript rotateSoundScript;
	static const NamedSoundScript launchSoundScript;
};

LINK_ENTITY_TO_CLASS(op4mortar, COp4Mortar)

TYPEDESCRIPTION	COp4Mortar::m_SaveData[] =
{
	DEFINE_FIELD(COp4Mortar, d_x, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, d_y, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_lastupdate, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_direction, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_start, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_end, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_velocity, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_hmin, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_hmax, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_fireLast, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_maxRange, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_minRange, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_fireDelay, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_trackDelay, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_zeroYaw, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_vGunAngle, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_vIdealGunVector, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_vIdealGunAngle, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_enemyRecheckTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE( COp4Mortar, CBaseMonster )

const NamedSoundScript COp4Mortar::rotateSoundScript = {
	CHAN_ITEM,
	{"player/pl_grate1.wav"},
	"Op4Mortar.Rotate"
};

const NamedSoundScript COp4Mortar::launchSoundScript = {
	CHAN_VOICE,
	{"weapons/mortarhit.wav"},
	1.0f,
	ATTN_NONE,
	"Op4Mortar.Launch"
};

void COp4Mortar::Precache()
{
	PRECACHE_MODEL("models/mortar.mdl");
	RegisterAndPrecacheSoundScript(rotateSoundScript);
	RegisterAndPrecacheSoundScript(launchSoundScript);
	UTIL_PrecacheOther("mortar_shell");
}

void COp4Mortar::Spawn()
{
	Precache();

	UTIL_SetOrigin(pev, pev->origin);

	SET_MODEL(edict(), "models/mortar.mdl");

	pev->health = 1;
	pev->sequence = LookupSequence("idle");

	ResetSequenceInfo();

	pev->frame = 0;
	pev->framerate = 1;

	if (m_fireDelay < 0.5)
		m_fireDelay = 5;

	if (m_minRange == 0)
		m_minRange = 128;

	if (m_maxRange == 0)
		m_maxRange = 2048;

	InitBoneControllers();

	m_vGunAngle = g_vecZero;

	m_lastupdate = gpGlobals->time;

	m_zeroYaw = UTIL_AngleMod(pev->angles.y + 180.0);

	m_fireLast = gpGlobals->time;
	m_trackDelay = gpGlobals->time;

	m_hEnemy = NULL;

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink(&COp4Mortar::MortarThink);
}

void COp4Mortar::PlaySound()
{
	if (gpGlobals->time > m_lastTimePlayedSound + 0.12f)
	{
		EmitSoundScript(rotateSoundScript);
		m_lastTimePlayedSound = gpGlobals->time + 0.12f;
	}
}

void COp4Mortar::UpdatePosition(int direction, int controller)
{
	if (gpGlobals->time - m_lastupdate >= 0.06)
	{
		switch (controller)
		{
		case 0:
			d_x = 3 * direction;
			break;

		case 1:
			d_y = 3 * direction;
			break;
		}

		m_vGunAngle.x = d_x + m_vGunAngle.x;
		m_vGunAngle.y = d_y + m_vGunAngle.y;

		if (m_hmin > m_vGunAngle.y)
		{
			m_vGunAngle.y = m_hmin;
			d_y = 0;
		}

		if (m_vGunAngle.y > m_hmax)
		{
			m_vGunAngle.y = m_hmax;
			d_y = 0;
		}

		if (m_vGunAngle.x < 10)
		{
			m_vGunAngle.x = 10;
			d_x = 0;
		}
		else if (m_vGunAngle.x > 90)
		{
			m_vGunAngle.x = 90;
			d_x = 0;
		}

		if (0 != d_x || 0 != d_y)
		{
			PlaySound();
		}

		SetBoneController(0, m_vGunAngle.x);
		SetBoneController(1, m_vGunAngle.y);

		d_x = 0;
		d_y = 0;

		m_lastupdate = gpGlobals->time;
	}
}

void COp4Mortar::MortarThink()
{
	const float flInterval = StudioFrameAdvance();

	if (m_fSequenceFinished)
	{
		if (pev->sequence != LookupSequence("idle"))
		{
			pev->frame = 0;
			pev->sequence = LookupSequence("idle");
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents(flInterval);

	//GlowShellUpdate();

	pev->nextthink = gpGlobals->time + 0.1;

	if ((pev->spawnflags & SF_MORTAR_ACTIVE) != 0)
	{
		if (!m_hEnemy)
		{
			m_hEnemy = FindTarget();
		}
		else
		{
			if (m_enemyRecheckTime <= gpGlobals->time)
			{
				CBaseEntity* pOtherEnemy = FindTarget();
				if (pOtherEnemy && pOtherEnemy != m_hEnemy)
					m_hEnemy = pOtherEnemy; // better enemy
				m_enemyRecheckTime = gpGlobals->time + 1.0f;
			}
		}

		CBaseEntity* pEnemy = m_hEnemy;

		if (pEnemy)
		{
			const float distanceSqr = (pEnemy->pev->origin - pev->origin).LengthSqr();

			if (pEnemy->IsAlive() && m_minRange*m_minRange <= distanceSqr && distanceSqr <= m_maxRange*m_maxRange)
			{
				bool properVec = m_vIdealGunVector != g_vecZero;

				if (gpGlobals->time - m_trackDelay > 0.5f)
				{
					Vector vecPos, vecAngle;
					GetAttachment(0, vecPos, vecAngle);

					m_vIdealGunVector = VecCheckThrow(pev, vecPos, pEnemy->pev->origin, m_velocity / 2);
					properVec = m_vIdealGunVector != g_vecZero;

					m_vIdealGunAngle = UTIL_VecToAngles(m_vIdealGunVector);

					m_trackDelay = gpGlobals->time;
				}

				if (properVec)
					AIUpdatePosition();

				const float idealDistance = m_vIdealGunVector.Length();

				if (properVec && idealDistance > 1.0)
				{
					if (gpGlobals->time - m_fireLast > m_fireDelay)
					{
						CreateMortarProjectile(this, idealDistance);
						m_fireLast = gpGlobals->time;
					}
				}
				else
				{
					m_fireLast = gpGlobals->time;
				}
			}
			else
			{
				m_hEnemy = NULL;
			}
		}
	}
}

int	COp4Mortar::IRelationship( CBaseEntity* pTarget )
{
	if (m_iClass == 0)
	{
		if (pTarget->IsPlayer())
			return R_HT;
		else
			return R_NO;
	}
	return CBaseMonster::IRelationship(pTarget);
}

CBaseEntity *COp4Mortar::FindTarget()
{
	Vector barrelEnd, barrelAngle;
	GetAttachment(0, barrelEnd, barrelAngle);

	auto isValidSight = [this, &barrelEnd](CBaseEntity* pEntity)
	{
		if (FBitSet(pev->spawnflags, SF_MORTAR_LINE_OF_SIGHT))
		{
			TraceResult tr;
			UTIL_TraceLine(barrelEnd, pEntity->pev->origin + pEntity->pev->view_ofs, dont_ignore_monsters, edict(), &tr);
			return tr.pHit == pEntity->edict();
		}
		return true;
	};

	CBaseEntity	*pReturn = nullptr;
	int			iBestRelationship = R_DL;
	float		lookDistance = m_maxRange?m_maxRange:512;
	float		nearestDistance = lookDistance + 1;

	CBaseEntity *pList[100];

	Vector delta = Vector( lookDistance, lookDistance, lookDistance );

	int count = 0;

	if (m_iClass == 0)
	{
		for(int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);
			if (pPlayer && pPlayer->IsFullyAlive())
			{
				pList[count] = pPlayer;
				count++;
			}
		}
	}
	else
	{
		count = UTIL_EntitiesInBox( pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT|FL_MONSTER );
	}

	for (int i = 0; i < count; i++ )
	{
		CBaseEntity* pEntity = pList[i];
		const int iRelationship = IRelationship(pEntity);
		if (iRelationship >= iBestRelationship && pEntity->IsFullyAlive())
		{
			const float distance = (pEntity->pev->origin - pev->origin).Length();
			if (distance >= m_minRange && lookDistance >= distance)
			{
				if (iRelationship > iBestRelationship && isValidSight(pEntity))
				{
					iBestRelationship = iRelationship;
					nearestDistance = distance;
					pReturn = pEntity;
				}
				else if (iRelationship == iBestRelationship)
				{
					if (distance <= nearestDistance && isValidSight(pEntity))
					{
						nearestDistance = distance;
						pReturn = pEntity;
					}
				}
			}
		}
	}
	return pReturn;
}

TakeDamageResult COp4Mortar::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	return TakeDamageResult();
}

void COp4Mortar::KeyValue(KeyValueData *pvkd)
{
	if(FStrEq(pvkd->szKeyName, "h_max"))
	{
		m_hmax = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else if(FStrEq(pvkd->szKeyName, "h_min"))
	{
		m_hmin = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else if(FStrEq(pvkd->szKeyName, "mortar_velocity"))
	{
		m_velocity = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else if(FStrEq(pvkd->szKeyName, "mindist"))
	{
		m_minRange = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else if(FStrEq(pvkd->szKeyName, "maxdist"))
	{
		m_maxRange = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else if(FStrEq(pvkd->szKeyName, "firedelay"))
	{
		m_fireDelay = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pvkd);
	}
}

void COp4Mortar::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_TOGGLE && (!pActivator || pActivator->IsPlayer()))
	{
		if ((pev->spawnflags & SF_MORTAR_ACTIVE) == 0 && (pev->spawnflags & SF_MORTAR_CONTROLLABLE) != 0)
		{
			CreateMortarProjectile(pActivator ? pActivator : this, m_velocity);
			return;
		}
	}

	//Toggle AI active state
	if (ShouldToggle(useType, (pev->spawnflags & SF_MORTAR_ACTIVE) != 0))
	{
		pev->spawnflags ^= SF_MORTAR_ACTIVE;

		m_fireLast = 0;
		m_hEnemy = NULL;
		m_trackDelay = gpGlobals->time;
	}
}

void COp4Mortar::AIUpdatePosition()
{
	if (fabs(m_vGunAngle.x - m_vIdealGunAngle.x) >= 3.0)
	{
		const float angle = UTIL_AngleDiff(m_vGunAngle.x, m_vIdealGunAngle.x);

		if (angle != 0)
		{
			const float absolute = fabs(angle);
			if (absolute <= 3.0)
				d_x = static_cast<int>(-absolute);
			else
				d_x = angle > 0 ? -3 : 3;
		}
	}

	const float yawAngle = UTIL_AngleMod(m_zeroYaw + m_vGunAngle.y);

	if (fabs(yawAngle - m_vIdealGunAngle.y) >= 3.0)
	{
		const float angle = UTIL_AngleDiff(yawAngle, m_vIdealGunAngle.y);

		if (angle != 0)
		{
			const float absolute = fabs(angle);
			if (absolute <= 3.0)
				d_y = static_cast<int>(-absolute);
			else
				d_y = angle > 0 ? -3 : 3;
		}
	}

	m_vGunAngle.x += d_x;
	m_vGunAngle.y += d_y;

	if (m_hmin > m_vGunAngle.y)
	{
		m_vGunAngle.y = m_hmin;
		d_y = 0;
	}

	if (m_vGunAngle.y > m_hmax)
	{
		m_vGunAngle.y = m_hmax;
		d_y = 0;
	}

	if (m_vGunAngle.x < 10.0)
	{
		m_vGunAngle.x = 10.0;
		d_x = 0;
	}
	else if (m_vGunAngle.x > 90.0)
	{
		m_vGunAngle.x = 90.0;
		d_x = 0;
	}

	if (0 != d_x || 0 != d_y)
	{
		PlaySound();
	}

	SetBoneController(0, m_vGunAngle.x);
	SetBoneController(1, m_vGunAngle.y);

	d_y = 0;
	d_x = 0;
}

void COp4Mortar::CreateMortarProjectile(CBaseEntity* pOwner, float speed)
{
	EmitSoundScript(launchSoundScript);
	UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 1000.0);

	Vector vecPos, vecAngle;
	GetAttachment(0, vecPos, vecAngle);

	vecAngle = m_vGunAngle;
	vecAngle.y = UTIL_AngleMod(pev->angles.y + m_vGunAngle.y);

	ProjectileParameters projectileParams("mortar_shell", vecPos, vecAngle, speed, pOwner);
	projectileParams.variant = 1;
	CBaseEntity* pMortarShell = CreateAndLaunchAsProjectile(projectileParams);
	if (pMortarShell)
	{
		pev->sequence = LookupSequence("fire");
		pev->frame = 0;
		ResetSequenceInfo();
	}
}

//========================================================
// COp4MortarController
//========================================================

class COp4MortarController : public CBaseToggle
{
public:
	void Spawn() override;
	int Restore(CRestore &restore) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return FCAP_CONTINUOUS_USE; }

	int Save(CSave &save) override;
	void KeyValue(KeyValueData *pvkd) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_direction;
	int m_controller;
	float m_lastpush;
};

LINK_ENTITY_TO_CLASS(func_op4mortarcontroller, COp4MortarController)

TYPEDESCRIPTION	COp4MortarController::m_SaveData[] =
{
	DEFINE_FIELD(COp4MortarController, m_controller, FIELD_INTEGER),
	DEFINE_FIELD(COp4MortarController, m_direction, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE( COp4MortarController, CBaseToggle )

void COp4MortarController::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);

	SET_MODEL(ENT(pev), STRING(pev->model));
	m_direction = -1;
	m_lastpush = gpGlobals->time;
}

void COp4MortarController::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = nullptr;
	if (pCaller && pCaller->IsPlayer())
	{
		pPlayer = (CBasePlayer*)pCaller;
	}

	if (pPlayer)
	{
		if (FBitSet(pPlayer->m_afButtonPressed, IN_USE))
			m_direction = -m_direction;
	}
	else if (gpGlobals->time - m_lastpush > 0.5f)
	{
		m_direction = -m_direction;
	}

	CBaseEntity* ent = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
	if (ent) {
		if (FClassnameIs(ent->pev, "op4mortar")) {
			COp4Mortar *Mortar = (COp4Mortar*)ent;
			Mortar->UpdatePosition(m_direction, m_controller);
		} else {
			ALERT(at_console, "Found %s, but it's not op4mortar!\n", STRING(pev->target));
		}
	}

	m_lastpush = gpGlobals->time;
}

void COp4MortarController::KeyValue(KeyValueData *pvkd)
{
	if(FStrEq(pvkd->szKeyName, "mortar_axis"))
	{
		m_controller = atoi(pvkd->szValue);
		pvkd->fHandled = true;
	}
	else
		CBaseToggle::KeyValue(pvkd);
}
