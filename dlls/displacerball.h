#ifndef DISPLACERBALL_H
#define DISPLACERBALL_H

#include "cbase.h"

#define DISPLACERBALL_SPEED 500.0f

class CBeam;
//=========================================================
// Displacement field
//=========================================================
class CDisplacerBall : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;

	static void SelfCreate(entvars_t *pevOwner, Vector vecStart);
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;

	void EXPORT BallTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink();
	void EXPORT KillThink();
	void Circle();

	int		Save(CSave &save) override;
	int		Restore(CRestore &restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	CBeam* m_pBeam[8];

	void EXPORT FlyThink();
	void ClearBeams();
	void ArmBeam( int iSide );

	int m_iBeams;

	EHANDLE m_hDisplacedTarget;

	static const NamedVisual spriteVisual;
	static const NamedVisual armBeamVisual;
	static const NamedVisual hitBeamVisual;
	static const NamedVisual ringVisual;
	static const NamedVisual lightVisual;

	static const NamedSoundScript impactSoundScript;
	static const NamedSoundScript explodeSoundScript;
};
#endif
