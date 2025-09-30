/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// skill.h - skill level concerns
//=========================================================
#pragma once
#if !defined(SKILL_H)
#define SKILL_H

#include "mod_features.h"
#include "util.h"

struct skilldata_t
{
	int iSkillLevel; // game skill level

	// Monster Health & Damage
	float agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;

#if FEATURE_BABYGARG
	float babygargantuaHealth;
	float babygargantuaDmgSlash;
	float babygargantuaDmgFire;
	float babygargantuaDmgStomp;
#endif

	float barnacleHealth;

	float barneyHealth;

	float bigmommaHealthFactor;		// Multiply each node's health by this
	float bigmommaDmgSlash;			// melee attack damage
	float bigmommaDmgBlast;			// mortar attack damage
	float bigmommaRadiusBlast;		// mortar attack radius

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;
	float bullsquidToxicity;
	float bullsquidDmgToxicPoison;
	float bullsquidDmgToxicImpact;

#if FEATURE_CLEANSUIT_SCIENTIST
	float cleansuitScientistHealth;
#endif

	float flybeeHealth;
	float flybeeDmgKick;
	float flybeeDmgBeam;
	float flybeeDmgFlyball;
	float flybeeMaxspeed;

#if FEATURE_OPFOR_GRUNT
	float fgruntHealth;
	float fgruntDmgKick;
	float fgruntShotgunPellets;
	float fgruntGrenadeSpeed;

	float medicHealth;
	float medicDmgKick;
	float medicGrenadeSpeed;
	float medicHeal;

	float torchHealth;
	float torchDmgKick;
	float torchGrenadeSpeed;
#endif
	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;
	float hassassinCloaking;

	float headcrabHealth;
	float headcrabDmgBite;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

#if FEATURE_HWGRUNT
	float hwgruntHealth;
#endif

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;
	float slaveZapRate;
	float slaveRevival;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

#if FEATURE_MASSN
	float massnHealth;
	float massnDmgKick;
	float massnGrenadeSpeed;
#endif

	float nihilanthHealth;
	float nihilanthZap;

	float panthereyeHealth;
	float panthereyeDmgClaw;

#if FEATURE_PITDRONE
	float pitdroneHealth;
	float pitdroneDmgBite;
	float pitdroneDmgWhip;
	float pitdroneDmgSpit;
#endif

#if FEATURE_PITWORM
	float pwormHealth;
	float pwormDmgSwipe;
	float pwormDmgBeam;
#endif

#if FEATURE_GENEWORM
	float gwormHealth;
	float gwormDmgSpit;
	float gwormDmgHit;
#endif

	float ospreyHealth;

#if FEATURE_BLACK_OSPREY
	float blackopsOspreyHealth;
#endif

#if FEATURE_OTIS
	float otisHealth;
#endif

#if FEATURE_KATE
	float kateHealth;
#endif

#if FEATURE_ROBOGRUNT
	float rgruntExplode;
#endif

	float scientistHealth;

#if FEATURE_SHOCKTROOPER
	float sroachHealth;
	float sroachDmgBite;
	float sroachLifespan;

	float strooperHealth;
	float strooperDmgKick;
	float strooperGrenadeSpeed;
	float strooperMaxCharge;
	float strooperRchgSpeed;
#endif

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

#if FEATURE_VOLTIFORE
	float voltigoreHealth;
	float voltigoreDmgPunch;
	float voltigoreDmgBeam;
	float voltigoreDmgExplode;

	float babyVoltigoreHealth;
	float babyVoltigoreDmgPunch;
#endif

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

#if FEATURE_ZOMBIE_BARNEY
	float zombieBarneyHealth;
	float zombieBarneyDmgOneSlash;
	float zombieBarneyDmgBothSlash;
#endif
#if FEATURE_ZOMBIE_SOLDIER
	float zombieSoldierHealth;
	float zombieSoldierDmgOneSlash;
	float zombieSoldierDmgBothSlash;
#endif
#if FEATURE_GONOME
	float gonomeHealth;
	float gonomeDmgOneSlash;
	float gonomeDmgGuts;
	float gonomeDmgOneBite;
#endif

	float floaterHealth;
	float floaterExplode;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;

	float robocopHealth;
	float robocopDmgMortar;
	float robocopDmgFist;
	float robocopSWRadius;

	float torHealth;
	float torDmgPunch;
	float torDmgEnergyBeam;
	float torDmgSonicBlast;

	float zaptrapSenseRadius;
	float zaptrapRespawnTime;

	// Player Weapons
	float plrDmgCrowbar;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgSatchel;
	float plrDmgTripmine;
#if FEATURE_DESERT_EAGLE
	float plrDmgEagle;
#endif
#if FEATURE_PIPEWRENCH
	float plrDmgPWrench;
#endif
#if FEATURE_KNIFE
	float plrDmgKnife;
#endif
#if FEATURE_GRAPPLE
	float plrDmgGrapple;
#endif
#if FEATURE_M249
	float plrDmg556;
#endif
#if FEATURE_SNIPERRIFLE
	float plrDmg762;
#endif
#if FEATURE_SHOCKBEAM
	float plrDmgShockroach;
	float plrDmgShockroachM;
	float monDmgShockroach;
#endif
#if FEATURE_SPOREGRENADE
	float plrDmgSpore;
#endif
#if FEATURE_DISPLACER
	float plrDisplacerRadius;
	float plrDmgDisplacer;
#endif
#if FEATURE_MEDKIT
	float plrDmgMedkit;
	float plrMedkitTime;
#endif
#if FEATURE_UZI
	float plrDmgUzi;
#endif

	// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;
	float monDmg357;
	float monDmg556;
	float monDmg762;
	float monDmgBuckshot;

	// mortar
	float mortarDmg;

	// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;
	float scientistHealTime;
	float sodaHeal;
	float vortigauntArmorCharge;

	// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

	// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;

	float flashlightDrainTime;
	float flashlightChargeTime;

	float plrArmorStrength;
};

extern	DLL_GLOBAL	skilldata_t	gSkillData;
float GetSkillCvar( const char *pName, const char* fallback = 0 );
float GetSkillCvar( const char *pName, float fallback );
float GetSkillCvarZeroable( const char* pName );

extern DLL_GLOBAL int		g_iSkillLevel;

#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3
#endif // SKILL_H
