#pragma once
#ifndef AMMUNITION_H
#define AMMUNITION_H

#include "items.h"

class CBasePlayerAmmo : public CPickup
{
public:
	void Spawn() override;
	void KeyValue(KeyValueData* pkvd) override;
	void Precache() override;
	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	virtual bool AddAmmo( CBaseEntity *pOther );
	void EXPORT DefaultUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	virtual const char* MyModel() = 0;
	virtual int DefaultAmount() = 0;
	int MyAmount();
	void SetCustomAmount(int amount);
	virtual const char* AmmoName() = 0;

	Vector MyRespawnSpot() override;
	float MyRespawnTime() override;
	void OnMaterialize() override;

	CBasePlayerAmmo* MyAmmoPointer() override {return this;}

protected:
	void SetTouchAndUse();
	void ClearTouchAndUse();
	void RemoveMyself();
};

#endif
