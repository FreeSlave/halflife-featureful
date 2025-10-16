#ifndef SHOCKBEAM_H
#define SHOCKBEAM_H

#include "mod_features.h"
#include "cbase.h"

class CBeam;
class CSprite;

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;

	static void Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity, EntityOverrides entityOverrides = EntityOverrides());
	static float ShockSpeed() { return 2000.0f; }
	void Touch(CBaseEntity *pOther) override;
	void EXPORT FlyThink();

	int		Save(CSave &save) override;
	int		Restore(CRestore &restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateEffects();
	void ClearEffects();
	void UpdateOnRemove() override;

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
