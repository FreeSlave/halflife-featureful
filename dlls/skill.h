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

#include "util.h"

struct skilldata_t
{
	int iSkillLevel; // game skill level

	// Monster Health & Damage
	float agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;

	float babygargantuaHealth;
	float babygargantuaDmgSlash;
	float babygargantuaDmgFire;
	float babygargantuaDmgStomp;

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

	float cleansuitScientistHealth;

	float flybeeHealth;
	float flybeeDmgKick;
	float flybeeDmgBeam;
	float flybeeDmgFlyball;
	float flybeeMaxspeed;

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

	float hwgruntHealth;

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

	float massnHealth;
	float massnDmgKick;
	float massnGrenadeSpeed;

	float nihilanthHealth;
	float nihilanthZap;

	float panthereyeHealth;
	float panthereyeDmgClaw;

	float pitdroneHealth;
	float pitdroneDmgBite;
	float pitdroneDmgWhip;
	float pitdroneDmgSpit;

	float pwormHealth;
	float pwormDmgSwipe;
	float pwormDmgBeam;

	float gwormHealth;
	float gwormDmgSpit;
	float gwormDmgHit;

	float ospreyHealth;

	float blackopsOspreyHealth;

	float otisHealth;

	float kateHealth;

	float rgruntExplode;

	float scientistHealth;

	float sroachHealth;
	float sroachDmgBite;
	float sroachLifespan;

	float strooperHealth;
	float strooperDmgKick;
	float strooperGrenadeSpeed;
	float strooperMaxCharge;
	float strooperRchgSpeed;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float voltigoreHealth;
	float voltigoreDmgPunch;
	float voltigoreDmgBeam;
	float voltigoreDmgExplode;

	float babyVoltigoreHealth;
	float babyVoltigoreDmgPunch;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float zombieBarneyHealth;
	float zombieBarneyDmgOneSlash;
	float zombieBarneyDmgBothSlash;

	float zombieSoldierHealth;
	float zombieSoldierDmgOneSlash;
	float zombieSoldierDmgBothSlash;

	float gonomeHealth;
	float gonomeDmgOneSlash;
	float gonomeDmgGuts;
	float gonomeDmgOneBite;

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
	float plrDmgEagle;
	float plrDmgPWrench;
	float plrDmgKnife;
	float plrDmgGrapple;
	float plrDmg556;
	float plrDmg762;
	float plrDmgShockroach;
	float plrDmgShockroachM;
	float monDmgShockroach;
	float plrDmgSpore;
	float plrDisplacerRadius;
	float plrDmgDisplacer;
	float plrDmgMedkit;
	float plrMedkitTime;
	float plrDmgUzi;

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
	float op4mortarDmg;

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
