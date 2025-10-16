#ifndef SPORE_H
#define SPORE_H

#include "ggrenade.h"

// Contact/Timed spore grenade
class CSpore : public CGrenade
{
public:
	enum SporeType
	{
		ROCKET = 1,
		GRENADE = 2
	};

public:
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;
	void Spawn() override;
	void UpdateOnRemove() override;

	void EXPORT IgniteThink();
	void EXPORT FlyThink();
	void EXPORT RocketTouch(CBaseEntity* pOther);
	void EXPORT MyBounceTouch(CBaseEntity* pOther);

	static CSpore* CreateSpore(const Vector& vecOrigin, const Vector& vecAngles, const Vector &vecDirection, CBaseEntity* pOwner, SporeType sporeType, bool bIsAI = false, bool bPuked = false, EntityOverrides entityOverrides = EntityOverrides());
	static CSpore* ShootContact(CBaseEntity *pOwner, const Vector& vecOrigin, const Vector &vecAngles , const Vector &vecVelocity);
	static CSpore* ShootTimed(CBaseEntity *pOwner, const Vector &vecOrigin, const Vector& vecAngles, const Vector &vecVelocity, bool bIsAI = false);

	static float SporeRocketSpeed() { return 1200.0f; }
	static float SporeGrenadeSpeed() { return 800.0f; }

	static const NamedSoundScript bounceSoundScript;
	static const NamedSoundScript impactSoundScript;

	static const NamedVisual modelVisual;
	static const NamedVisual spriteVisual;
	static const NamedVisual blowVisual;
	static const NamedVisual blowAltVisual;
	static const NamedVisual sprayVisual;
	static const NamedVisual trailVisual;
	static const NamedVisual lightVisual;

private:
	SporeType m_SporeType;

	float m_flIgniteTime;
	float m_flSoundDelay;

	bool m_bIsAI;
	EHANDLE m_hSprite;
	bool m_bPuked;
};
#endif
