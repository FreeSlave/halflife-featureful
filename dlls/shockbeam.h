#ifndef SHOCKBEAM_H
#define SHOCKBEAM_H

#include "cbase.h"

class CBeam;
class CSprite;

#define SHOCKBEAM_SPEED 2000.0f

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;

	void Touch(CBaseEntity *pOther) override;
	void EXPORT FlyThink();

	int		Save(CSave &save) override;
	int		Restore(CRestore &restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateEffects();
	void ClearEffects();
	void UpdateOnRemove() override;
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;

	static const NamedSoundScript impactSoundScript;

	static const NamedVisual spriteVisual;
	static const NamedVisual beam1Visual;
	static const NamedVisual beam2Visual;
	static const NamedVisual lightVisual;
	static const NamedVisual shellVisual;
};
#endif
