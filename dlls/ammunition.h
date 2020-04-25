#pragma once
#ifndef AMMUNITION_H
#define AMMUNITION_H

class CBasePlayerAmmo : public CBaseDelay
{
public:
	virtual void Spawn( void );
	void Precache();
	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	void EXPORT FallThink();
	virtual BOOL AddAmmo( CBaseEntity *pOther );
	int ObjectCaps();
	void SetObjectCollisionBox();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	CBaseEntity* Respawn( void );
	void EXPORT Materialize( void );

	virtual const char* MyModel() = 0;
	virtual int MyAmount() = 0;
	virtual const char* AmmoName() = 0;

	int ItemCategory() { return ITEM_CATEGORY_AMMO; }
};

#endif
