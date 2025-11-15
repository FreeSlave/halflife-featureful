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
// GameRules.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"ammunition.h"
#include	"gamerules.h"
#include	"teamplay_gamerules.h"
#include	"skill.h"
#include	"game.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

DLL_GLOBAL CGameRules *g_pGameRules = NULL;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgMOTD;

int g_teamplay = 0;

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer *pPlayer, const char *pszAmmoName )
{
	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(pszAmmoName);
	if( ammoType )
	{
		if( pPlayer->AmmoInventory( ammoType->id ) < ammoType->maxAmmo )
		{
			// player has room for more of this type of ammo
			return true;
		}
	}

	return false;
}

//=========================================================
//=========================================================
edict_t *CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );

	pPlayer->pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS( pentSpawnSpot )->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = 1;

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
	// only living players can have items
	if( pPlayer->pev->deadflag != DEAD_NO )
		return false;

	if( pWeapon->pszAmmo1() )
	{
		if( !CanHaveAmmo( pPlayer, pWeapon->pszAmmo1() ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if( pPlayer->HasPlayerItem( pWeapon ) )
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if( pPlayer->HasPlayerItem( pWeapon ) )
		{
			return false;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return true;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData ()
{
	int iSkill;

	iSkill = (int)CVAR_GET_FLOAT( "skill" );
	g_iSkillLevel = iSkill;

	if( iSkill < 1 )
	{
		iSkill = 1;
	}
	else if( iSkill > 3 )
	{
		iSkill = 3; 
	}

	gSkillData.iSkillLevel = iSkill;

	ALERT( at_console, "\nGAME SKILL LEVEL:%d\n",iSkill );

	//Agrunt		
	gSkillData.agruntHealth = GetSkillCvar( "sk_agrunt_health" );
	gSkillData.agruntDmgPunch = GetSkillCvar( "sk_agrunt_dmg_punch" );

	// Apache 
	gSkillData.apacheHealth = GetSkillCvar( "sk_apache_health" );
	
	//Barnacle
	gSkillData.barnacleHealth = GetSkillCvar( "sk_barnacle_health");

	// Baby Gargantua
	if (g_modFeatures.IsMonsterEnabled("babygarg"))
	{
		gSkillData.babygargantuaHealth = GetSkillCvar( "sk_babygargantua_health" );
		gSkillData.babygargantuaDmgSlash = GetSkillCvar( "sk_babygargantua_dmg_slash" );
		gSkillData.babygargantuaDmgFire = GetSkillCvar( "sk_babygargantua_dmg_fire" );
		gSkillData.babygargantuaDmgStomp = GetSkillCvar( "sk_babygargantua_dmg_stomp" );
	}

	// Barney
	gSkillData.barneyHealth = GetSkillCvar( "sk_barney_health" );

	// Big Momma
	gSkillData.bigmommaHealthFactor = GetSkillCvar( "sk_bigmomma_health_factor" );
	gSkillData.bigmommaDmgSlash = GetSkillCvar( "sk_bigmomma_dmg_slash" );
	gSkillData.bigmommaDmgBlast = GetSkillCvar( "sk_bigmomma_dmg_blast" );
	gSkillData.bigmommaRadiusBlast = GetSkillCvar( "sk_bigmomma_radius_blast" );

	// Bullsquid
	gSkillData.bullsquidHealth = GetSkillCvar( "sk_bullsquid_health" );
	gSkillData.bullsquidDmgBite = GetSkillCvar( "sk_bullsquid_dmg_bite" );
	gSkillData.bullsquidDmgWhip = GetSkillCvar( "sk_bullsquid_dmg_whip" );
	gSkillData.bullsquidDmgSpit = GetSkillCvar( "sk_bullsquid_dmg_spit" );
	gSkillData.bullsquidToxicity = GetSkillCvarZeroable( "sk_bullsquid_toxicity" );
	gSkillData.bullsquidDmgToxicPoison = GetSkillCvar( "sk_bullsquid_dmg_toxic_poison", gSkillData.bullsquidDmgSpit/4.0f );
	gSkillData.bullsquidDmgToxicImpact = GetSkillCvar( "sk_bullsquid_dmg_toxic_impact", gSkillData.bullsquidDmgSpit * 1.5f );

	// Cleansuit Scientist
	if (g_modFeatures.IsMonsterEnabled("cleansuit_scientist"))
		gSkillData.cleansuitScientistHealth = GetSkillCvar( "sk_cleansuit_scientist_health", "sk_scientist_health" );

	// Flybee
	if (g_modFeatures.IsMonsterEnabled("flybee"))
	{
		gSkillData.flybeeHealth = GetSkillCvar("sk_flybee_health", "sk_ichthyosaur_health");
		gSkillData.flybeeDmgKick = GetSkillCvar("sk_flybee_dmg_kick");
		gSkillData.flybeeDmgBeam = GetSkillCvar("sk_flybee_dmg_beam");
		gSkillData.flybeeDmgFlyball = GetSkillCvar("sk_flybee_dmg_flyball");
		gSkillData.flybeeMaxspeed = GetSkillCvar("sk_flybee_maxspeed");
	}

	// Gargantua
	gSkillData.gargantuaHealth = GetSkillCvar( "sk_gargantua_health" );
	gSkillData.gargantuaDmgSlash = GetSkillCvar( "sk_gargantua_dmg_slash" );
	gSkillData.gargantuaDmgFire = GetSkillCvar( "sk_gargantua_dmg_fire" );
	gSkillData.gargantuaDmgStomp = GetSkillCvar( "sk_gargantua_dmg_stomp" );

	// Hassassin
	gSkillData.hassassinHealth = GetSkillCvar( "sk_hassassin_health" );
	gSkillData.hassassinCloaking = GetSkillCvarZeroable( "sk_hassassin_cloaking" );

	// Headcrab
	gSkillData.headcrabHealth = GetSkillCvar( "sk_headcrab_health" );
	gSkillData.headcrabDmgBite = GetSkillCvar( "sk_headcrab_dmg_bite" );

	// Hgrunt
	if (g_modFeatures.IsMonsterEnabled("human_grunt_ally"))
	{
		gSkillData.fgruntHealth = GetSkillCvar( "sk_hgrunt_ally_health", "sk_hgrunt_health" );
		gSkillData.fgruntDmgKick = GetSkillCvar( "sk_hgrunt_ally_kick", "sk_hgrunt_kick" );
		gSkillData.fgruntShotgunPellets = GetSkillCvar( "sk_hgrunt_ally_pellets", "sk_hgrunt_pellets" );
		gSkillData.fgruntGrenadeSpeed = GetSkillCvar( "sk_hgrunt_ally_gspeed", "sk_hgrunt_gspeed" );
	}

	// Medic
	if (g_modFeatures.IsMonsterEnabled("human_grunt_medic"))
	{
		gSkillData.medicHealth = GetSkillCvar( "sk_medic_ally_health", "sk_hgrunt_health" );
		gSkillData.medicDmgKick = GetSkillCvar( "sk_medic_ally_kick", "sk_hgrunt_kick" );
		gSkillData.medicGrenadeSpeed = GetSkillCvar( "sk_medic_ally_gspeed", gSkillData.fgruntGrenadeSpeed );
		gSkillData.medicHeal = GetSkillCvar( "sk_medic_ally_heal" );
	}

	// Torch
	if (g_modFeatures.IsMonsterEnabled("human_grunt_torch"))
	{
		gSkillData.torchHealth = GetSkillCvar( "sk_torch_ally_health", "sk_hgrunt_health" );
		gSkillData.torchDmgKick = GetSkillCvar( "sk_torch_ally_kick", "sk_hgrunt_kick" );
		gSkillData.torchGrenadeSpeed = GetSkillCvar( "sk_torch_ally_gspeed", gSkillData.fgruntGrenadeSpeed );
	}

	// Hgrunt 
	gSkillData.hgruntHealth = GetSkillCvar( "sk_hgrunt_health" );
	gSkillData.hgruntDmgKick = GetSkillCvar( "sk_hgrunt_kick" );
	gSkillData.hgruntShotgunPellets = GetSkillCvar( "sk_hgrunt_pellets" );
	gSkillData.hgruntGrenadeSpeed = GetSkillCvar( "sk_hgrunt_gspeed" );

	// HWgrunt
	if (g_modFeatures.IsMonsterEnabled("hwgrunt"))
		gSkillData.hwgruntHealth = GetSkillCvar( "sk_hwgrunt_health" );

	// Houndeye
	gSkillData.houndeyeHealth = GetSkillCvar( "sk_houndeye_health" );
	gSkillData.houndeyeDmgBlast = GetSkillCvar( "sk_houndeye_dmg_blast" );

	// ISlave
	gSkillData.slaveHealth = GetSkillCvar( "sk_islave_health" );
	gSkillData.slaveDmgClaw = GetSkillCvar( "sk_islave_dmg_claw" );
	gSkillData.slaveDmgClawrake = GetSkillCvar( "sk_islave_dmg_clawrake" );
	gSkillData.slaveDmgZap = GetSkillCvar( "sk_islave_dmg_zap" );
	gSkillData.slaveZapRate = GetSkillCvar( "sk_islave_zap_rate" );
	gSkillData.slaveRevival = GetSkillCvarZeroable( "sk_islave_revival" );

	// Icthyosaur
	gSkillData.ichthyosaurHealth = GetSkillCvar( "sk_ichthyosaur_health" );
	gSkillData.ichthyosaurDmgShake = GetSkillCvar( "sk_ichthyosaur_shake" );

	// Leech
	gSkillData.leechHealth = GetSkillCvar( "sk_leech_health" );

	gSkillData.leechDmgBite = GetSkillCvar( "sk_leech_dmg_bite" );

	// Controller
	gSkillData.controllerHealth = GetSkillCvar( "sk_controller_health" );
	gSkillData.controllerDmgZap = GetSkillCvar( "sk_controller_dmgzap" );
	gSkillData.controllerSpeedBall = GetSkillCvar( "sk_controller_speedball" );
	gSkillData.controllerDmgBall = GetSkillCvar( "sk_controller_dmgball" );

	// Massn
	if (g_modFeatures.IsMonsterEnabled("male_assassin"))
	{
		gSkillData.massnHealth = GetSkillCvar( "sk_massassin_health", "sk_hgrunt_health" );
		gSkillData.massnDmgKick = GetSkillCvar( "sk_massassin_kick", "sk_hgrunt_kick" );
		gSkillData.massnGrenadeSpeed = GetSkillCvar( "sk_massassin_gspeed", "sk_hgrunt_gspeed" );
	}

	// Nihilanth
	gSkillData.nihilanthHealth = GetSkillCvar( "sk_nihilanth_health" );
	gSkillData.nihilanthZap = GetSkillCvar( "sk_nihilanth_zap" );

	// Panthereye
	if (g_modFeatures.IsMonsterEnabled("panthereye"))
	{
		gSkillData.panthereyeHealth = GetSkillCvar( "sk_panthereye_health" );
		gSkillData.panthereyeDmgClaw = GetSkillCvar( "sk_panthereye_dmg_claw" );
	}

	// Pitdrone
	if (g_modFeatures.IsMonsterEnabled("pitdrone"))
	{
		gSkillData.pitdroneHealth = GetSkillCvar( "sk_pitdrone_health" );
		gSkillData.pitdroneDmgBite = GetSkillCvar( "sk_pitdrone_dmg_bite" );
		gSkillData.pitdroneDmgWhip = GetSkillCvar( "sk_pitdrone_dmg_whip" );
		gSkillData.pitdroneDmgSpit = GetSkillCvar( "sk_pitdrone_dmg_spit" );
	}

	// Pitworm
	if (g_modFeatures.IsMonsterEnabled("pitworm"))
	{
		gSkillData.pwormHealth = GetSkillCvar( "sk_pitworm_health" );
		gSkillData.pwormDmgSwipe = GetSkillCvar( "sk_pitworm_dmg_swipe" );
		gSkillData.pwormDmgBeam = GetSkillCvar( "sk_pitworm_dmg_beam" );
	}

	// Geneworm
	if (g_modFeatures.IsMonsterEnabled("geneworm"))
	{
		gSkillData.gwormHealth = GetSkillCvar( "sk_geneworm_health" );
		gSkillData.gwormDmgSpit = GetSkillCvar( "sk_geneworm_dmg_spit" );
		gSkillData.gwormDmgHit = GetSkillCvar( "sk_geneworm_dmg_hit" );
	}

	gSkillData.ospreyHealth = GetSkillCvar( "sk_osprey" );

	if (g_modFeatures.IsMonsterEnabled("blkop_osprey"))
		gSkillData.blackopsOspreyHealth = GetSkillCvar( "sk_blkopsosprey", "sk_osprey" );

	// Otis
	if (g_modFeatures.IsMonsterEnabled("otis"))
		gSkillData.otisHealth = GetSkillCvar( "sk_otis_health", "sk_barney_health" );

	// Kate
	if (g_modFeatures.IsMonsterEnabled("kate"))
		gSkillData.kateHealth = GetSkillCvar( "sk_kate_health", "sk_barney_health" );

	// Robogrunt
	if (g_modFeatures.IsMonsterEnabled("robogrunt"))
		gSkillData.rgruntExplode = GetSkillCvar( "sk_rgrunt_explode", "sk_plr_hand_grenade" );
	// Scientist
	gSkillData.scientistHealth = GetSkillCvar( "sk_scientist_health" );

	// Shock Roach
	if (g_modFeatures.IsMonsterEnabled("shockroach"))
	{
		gSkillData.sroachHealth = GetSkillCvar( "sk_shockroach_health" );
		gSkillData.sroachDmgBite = GetSkillCvar( "sk_shockroach_dmg_bite" );
		gSkillData.sroachLifespan = GetSkillCvar( "sk_shockroach_lifespan" );
	}

	// ShockTrooper
	if (g_modFeatures.IsMonsterEnabled("shocktrooper"))
	{
		gSkillData.strooperHealth = GetSkillCvar( "sk_shocktrooper_health" );
		gSkillData.strooperDmgKick = GetSkillCvar( "sk_shocktrooper_kick" );
		gSkillData.strooperGrenadeSpeed = GetSkillCvar( "sk_shocktrooper_gspeed" );
		gSkillData.strooperMaxCharge = GetSkillCvar( "sk_shocktrooper_maxcharge" );
		gSkillData.strooperRchgSpeed = GetSkillCvar( "sk_shocktrooper_rchgspeed" );
	}

	// Snark
	gSkillData.snarkHealth = GetSkillCvar( "sk_snark_health" );
	gSkillData.snarkDmgBite = GetSkillCvar( "sk_snark_dmg_bite" );
	gSkillData.snarkDmgPop = GetSkillCvar( "sk_snark_dmg_pop" );

	// Voltigore
	if (g_modFeatures.IsMonsterEnabled("voltigore"))
	{
		gSkillData.voltigoreHealth = GetSkillCvar( "sk_voltigore_health" );
		gSkillData.voltigoreDmgPunch = GetSkillCvar( "sk_voltigore_dmg_punch" );
		gSkillData.voltigoreDmgBeam = GetSkillCvar( "sk_voltigore_dmg_beam" );
		gSkillData.voltigoreDmgExplode = GetSkillCvar( "sk_voltigore_dmg_explode", "sk_voltigore_dmg_beam" );
	}

	// Baby Voltigore
	if (g_modFeatures.IsMonsterEnabled("babyvoltigore"))
	{
		gSkillData.babyVoltigoreHealth = GetSkillCvar( "sk_babyvoltigore_health" );
		gSkillData.babyVoltigoreDmgPunch = GetSkillCvar( "sk_babyvoltigore_dmg_punch" );
	}

	// Zombie
	gSkillData.zombieHealth = GetSkillCvar( "sk_zombie_health" );
	gSkillData.zombieDmgOneSlash = GetSkillCvar( "sk_zombie_dmg_one_slash" );
	gSkillData.zombieDmgBothSlash = GetSkillCvar( "sk_zombie_dmg_both_slash" );
	// Zombie Barney
	if (g_modFeatures.IsMonsterEnabled("zombie_barney"))
	{
		gSkillData.zombieBarneyHealth = GetSkillCvar( "sk_zombie_barney_health", "sk_zombie_health" );
		gSkillData.zombieBarneyDmgOneSlash = GetSkillCvar( "sk_zombie_barney_dmg_one_slash", "sk_zombie_dmg_one_slash" );
		gSkillData.zombieBarneyDmgBothSlash = GetSkillCvar( "sk_zombie_barney_dmg_both_slash", "sk_zombie_dmg_both_slash" );
	}
	// Zombie Soldier
	if (g_modFeatures.IsMonsterEnabled("zombie_soldier"))
	{
		gSkillData.zombieSoldierHealth = GetSkillCvar( "sk_zombie_soldier_health");
		gSkillData.zombieSoldierDmgOneSlash = GetSkillCvar( "sk_zombie_soldier_dmg_one_slash");
		gSkillData.zombieSoldierDmgBothSlash = GetSkillCvar( "sk_zombie_soldier_dmg_both_slash");
	}
	// Gonome
	if (g_modFeatures.IsMonsterEnabled("gonome"))
	{
		gSkillData.gonomeHealth = GetSkillCvar( "sk_gonome_health" );
		gSkillData.gonomeDmgOneSlash = GetSkillCvar( "sk_gonome_dmg_one_slash" );
		gSkillData.gonomeDmgGuts = GetSkillCvar( "sk_gonome_dmg_guts" );
		gSkillData.gonomeDmgOneBite = GetSkillCvar( "sk_gonome_dmg_one_bite" );
	}
	// Floater
	if (g_modFeatures.IsMonsterEnabled("floater"))
	{
		gSkillData.floaterHealth = GetSkillCvar( "sk_floater_health" );
		gSkillData.floaterExplode = GetSkillCvar( "sk_floater_explode" );
	}

	//Turret
	gSkillData.turretHealth = GetSkillCvar( "sk_turret_health" );

	// MiniTurret
	gSkillData.miniturretHealth = GetSkillCvar( "sk_miniturret_health" );

	// Sentry Turret
	gSkillData.sentryHealth = GetSkillCvar( "sk_sentry_health" );

	// Robocop
	if (g_modFeatures.IsMonsterEnabled("robocop"))
	{
		gSkillData.robocopHealth = GetSkillCvar( "sk_robocop_health" );
		gSkillData.robocopDmgMortar = GetSkillCvar( "sk_robocop_dmg_mortar" );
		gSkillData.robocopDmgFist = GetSkillCvar( "sk_robocop_dmg_fist" );
		gSkillData.robocopSWRadius = GetSkillCvar( "sk_robocop_sw_radius" );
	}

	if (g_modFeatures.IsMonsterEnabled("tor"))
	{
		gSkillData.torHealth = GetSkillCvar( "sk_tor_health" );
		gSkillData.torDmgPunch = GetSkillCvar( "sk_tor_punch" );
		gSkillData.torDmgEnergyBeam = GetSkillCvar( "sk_tor_energybeam" );
		gSkillData.torDmgSonicBlast = GetSkillCvar( "sk_tor_sonicblast" );
	}

	// Zap ball trap
	gSkillData.zaptrapSenseRadius = GetSkillCvar( "sk_zaptrap_sense_radius" );
	gSkillData.zaptrapRespawnTime = GetSkillCvar( "sk_zaptrap_respawn_time" );

	// PLAYER WEAPONS

	// Crowbar whack
	gSkillData.plrDmgCrowbar = GetSkillCvar( "sk_plr_crowbar" );

	// Glock Round
	gSkillData.plrDmg9MM = GetSkillCvar( "sk_plr_9mm_bullet" );

	// 357 Round
	gSkillData.plrDmg357 = GetSkillCvar( "sk_plr_357_bullet" );

	// MP5 Round
	gSkillData.plrDmgMP5 = GetSkillCvar( "sk_plr_9mmAR_bullet" );

	// M203 grenade
	gSkillData.plrDmgM203Grenade = GetSkillCvar( "sk_plr_9mmAR_grenade" );

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = GetSkillCvar( "sk_plr_buckshot" );
	gSkillData.monDmgBuckshot = GetSkillCvar( "sk_buckshot", "sk_plr_buckshot" );

	// Crossbow
	gSkillData.plrDmgCrossbowClient = GetSkillCvar( "sk_plr_xbow_bolt_client" );
	gSkillData.plrDmgCrossbowMonster = GetSkillCvar( "sk_plr_xbow_bolt_monster" );

	// RPG
	gSkillData.plrDmgRPG = GetSkillCvar( "sk_plr_rpg" );

	// Gauss gun
	gSkillData.plrDmgGauss = GetSkillCvar( "sk_plr_gauss" );

	// Egon Gun
	gSkillData.plrDmgEgonNarrow = GetSkillCvar( "sk_plr_egon_narrow" );
	gSkillData.plrDmgEgonWide = GetSkillCvar( "sk_plr_egon_wide" );

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = GetSkillCvar( "sk_plr_hand_grenade" );

	// Satchel Charge
	gSkillData.plrDmgSatchel = GetSkillCvar( "sk_plr_satchel" );

	// Tripmine
	gSkillData.plrDmgTripmine = GetSkillCvar( "sk_plr_tripmine" );

	// Medkit 
	if (g_modFeatures.IsWeaponEnabled(WEAPON_MEDKIT))
	{
		gSkillData.plrDmgMedkit = GetSkillCvar( "sk_plr_medkitshot" );
		gSkillData.plrMedkitTime = GetSkillCvarZeroable( "sk_plr_medkittime" );
	}

	// Desert Eagle
	if (g_modFeatures.IsWeaponEnabled(WEAPON_EAGLE))
		gSkillData.plrDmgEagle = GetSkillCvar( "sk_plr_eagle" );

	// Pipe wrench
	if (g_modFeatures.IsWeaponEnabled(WEAPON_PIPEWRENCH))
		gSkillData.plrDmgPWrench = GetSkillCvar( "sk_plr_pipewrench" );

	// Knife
	if (g_modFeatures.IsWeaponEnabled(WEAPON_KNIFE))
		gSkillData.plrDmgKnife = GetSkillCvar( "sk_plr_knife" );

	// Grapple
	if (g_modFeatures.IsWeaponEnabled(WEAPON_GRAPPLE))
		gSkillData.plrDmgGrapple = GetSkillCvar( "sk_plr_grapple" );

	// M249
	if (g_modFeatures.IsWeaponEnabled(WEAPON_M249) || g_modFeatures.IsWeaponEnabled(WEAPON_MINIGUN))
		gSkillData.plrDmg556 = GetSkillCvar( "sk_plr_556_bullet" );

	// 762 Round
	if (g_modFeatures.IsWeaponEnabled(WEAPON_SNIPERRIFLE))
		gSkillData.plrDmg762 = GetSkillCvar( "sk_plr_762_bullet" );

	if (g_modFeatures.ShockBeamEnabled())
	{
		gSkillData.plrDmgShockroach = GetSkillCvar( "sk_plr_shockroachs" );
		gSkillData.plrDmgShockroachM = GetSkillCvar( "sk_plr_shockroachm" );
		gSkillData.monDmgShockroach = GetSkillCvar( "sk_shockroach", IsMultiplayer() ? gSkillData.plrDmgShockroachM : gSkillData.plrDmgShockroach );
	}

	if (g_modFeatures.SporesEnabled())
		gSkillData.plrDmgSpore = GetSkillCvar( "sk_plr_spore" );

	if (g_modFeatures.DisplacerBallEnabled())
	{
		gSkillData.plrDmgDisplacer = GetSkillCvar( "sk_plr_displacer_other" );
		gSkillData.plrDisplacerRadius = GetSkillCvar( "sk_plr_displacer_radius" );
	}

	if (g_modFeatures.IsWeaponEnabled(WEAPON_UZI))
		gSkillData.plrDmgUzi = GetSkillCvar( "sk_plr_uzi" );

	// MONSTER WEAPONS
	gSkillData.monDmg12MM = GetSkillCvar( "sk_12mm_bullet" );
	gSkillData.monDmgMP5 = GetSkillCvar ("sk_9mmAR_bullet" );
	gSkillData.monDmg9MM = GetSkillCvar( "sk_9mm_bullet" );
	gSkillData.monDmg357 = GetSkillCvar( "sk_357_bullet", "sk_plr_eagle" );
	gSkillData.monDmg556 = GetSkillCvar( "sk_556_bullet", "sk_plr_556_bullet" );
	gSkillData.monDmg762 = GetSkillCvar( "sk_762_bullet", "sk_plr_762_bullet" );

	// MONSTER HORNET
	gSkillData.monDmgHornet = GetSkillCvar( "sk_hornet_dmg" );

	gSkillData.plrDmgHornet = GetSkillCvar( "sk_plr_hornet_dmg" );

	// MORTAR
	gSkillData.mortarDmg = GetSkillCvar( "sk_mortar" );
	gSkillData.op4mortarDmg = GetSkillCvar( "sk_op4mortar", gSkillData.plrDmgRPG * 2 );

	// HEALTH/CHARGE
	gSkillData.suitchargerCapacity = GetSkillCvar( "sk_suitcharger" );
	gSkillData.batteryCapacity = GetSkillCvar( "sk_battery" );
	gSkillData.healthchargerCapacity = GetSkillCvar ( "sk_healthcharger" );
	gSkillData.healthkitCapacity = GetSkillCvar ( "sk_healthkit" );
	gSkillData.scientistHeal = GetSkillCvar ( "sk_scientist_heal" );
	gSkillData.scientistHealTime = GetSkillCvar ( "sk_scientist_heal_time" );
	gSkillData.sodaHeal = GetSkillCvar( "sk_soda" );
	gSkillData.vortigauntArmorCharge = GetSkillCvar( "sk_vortigaunt_armor_charge", "sk_battery" );

	// monster damage adj
	gSkillData.monHead = GetSkillCvar( "sk_monster_head" );
	gSkillData.monChest = GetSkillCvar( "sk_monster_chest" );
	gSkillData.monStomach = GetSkillCvar( "sk_monster_stomach" );
	gSkillData.monLeg = GetSkillCvar( "sk_monster_leg" );
	gSkillData.monArm = GetSkillCvar( "sk_monster_arm" );

	// player damage adj
	gSkillData.plrHead = GetSkillCvar( "sk_player_head" );
	gSkillData.plrChest = GetSkillCvar( "sk_player_chest" );
	gSkillData.plrStomach = GetSkillCvar( "sk_player_stomach" );
	gSkillData.plrLeg = GetSkillCvar( "sk_player_leg" );
	gSkillData.plrArm = GetSkillCvar( "sk_player_arm" );

	gSkillData.flashlightDrainTime = GetSkillCvar( "sk_flashlight_drain_time" );
	gSkillData.flashlightChargeTime = GetSkillCvar( "sk_flashlight_charge_time" );

	gSkillData.plrArmorStrength = GetSkillCvar( "sk_plr_armor_strength" );
}

void CGameRules::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	pPlayer->SetPrefsFromUserinfo( infobuffer );
}

CBasePlayer *CGameRules::EffectivePlayer(CBaseEntity *pActivator)
{
	if (pActivator && pActivator->IsPlayer()) {
		return (CBasePlayer*)pActivator;
	}
	return NULL;
}

CBasePlayer *CGameRules::EffectiveAlivePlayer(CBaseEntity *pActivator)
{
	CBasePlayer* pPlayer = EffectivePlayer(pActivator);
	if (pPlayer && pPlayer->IsAlive())
		return pPlayer;
	return NULL;
}

bool CGameRules::EquipPlayerFromMapConfig(CBasePlayer *pPlayer, const MapConfig &mapConfig)
{
	extern bool gEvilImpulse101;

	if (mapConfig.valid)
	{
		gEvilImpulse101 = true;

		bool giveSuit = !mapConfig.nosuit;
		if (giveSuit)
		{
			int suitSpawnFlags = 0;
			switch (mapConfig.suitLogon) {
			case SuitNoLogon:
				suitSpawnFlags |= SF_SUIT_NOLOGON;
				break;
			case SuitShortLogon:
				suitSpawnFlags |= SF_SUIT_SHORTLOGON;
				break;
			case SuitLongLogon:
				break;
			}
			pPlayer->GiveNamedItem("item_suit", suitSpawnFlags);
		}

		if (mapConfig.suit_light == MapConfig::SUIT_LIGHT_NOTHING)
		{
			pPlayer->RemoveSuitLight();
		}
		else if (mapConfig.suit_light == MapConfig::SUIT_LIGHT_FLASHLIGHT)
		{
			pPlayer->SetFlashlightOnly();
		}
		else if (mapConfig.suit_light == MapConfig::SUIT_LIGHT_NVG)
		{
			pPlayer->SetNVGOnly();
		}

		int i, j;
		for (i=0; i<mapConfig.pickupEntCount; ++i)
		{
			for (j=0; j<mapConfig.pickupEnts[i].count; ++j)
			{
				const char* entName = STRING(mapConfig.pickupEnts[i].entName);
				pPlayer->GiveNamedItem(entName);
			}
		}
		gEvilImpulse101 = false;

		for (i=0; i<mapConfig.ammoCount; ++i)
		{
			const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(mapConfig.ammo[i].name);
			if (ammoType && mapConfig.ammo[i].count > 0)
			{
				pPlayer->GiveAmmo(mapConfig.ammo[i].count, ammoType->name);
			}
		}

		if (IsCoOp() && g_modFeatures.IsWeaponEnabled(WEAPON_MEDKIT) && !mapConfig.nomedkit && !pPlayer->WeaponById(WEAPON_MEDKIT))
		{
			pPlayer->GiveNamedItem("weapon_medkit");
		}
		if (mapConfig.maxhealth > 0)
		{
			pPlayer->SetMaxHealth(mapConfig.maxhealth);
			pPlayer->pev->health = pPlayer->pev->max_health;
		}
		if (mapConfig.maxarmor.has_value())
		{
			pPlayer->SetMaxArmor(*mapConfig.maxarmor);
		}
		if (mapConfig.startarmor > 0)
			pPlayer->SetArmor(mapConfig.startarmor);
		if (mapConfig.starthealth > 0)
			pPlayer->SetHealth(mapConfig.starthealth);

		if (mapConfig.longjump)
		{
			pPlayer->SetLongjump(true);
		}

		pPlayer->SwitchToBestWeapon();

		return true;
	}
	return false;
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules *InstallGameRules()
{
	SERVER_COMMAND( "exec game.cfg\n" );
	SERVER_EXECUTE();

	if( !gpGlobals->deathmatch && !(gpGlobals->coop && gpGlobals->maxClients > 1) )
	{
		// generic half-life
		g_teamplay = 0;
		return new CHalfLifeRules;
	}
	else
	{
		if( teamplay.value > 0 )
		{
			// teamplay
			g_teamplay = 1;
			return new CHalfLifeTeamplay;
		}
		if( sv_busters.value > 0 )
		{
			g_teamplay = 0;
			return new CMultiplayBusters;
		}
		if( (int)gpGlobals->deathmatch == 1 )
		{
			// vanilla deathmatch
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
		else
		{
			// vanilla deathmatch??
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
	}
}

int TridepthValue()
{
	extern cvar_t npc_tridepth;
	return (int)npc_tridepth.value;
}

bool TridepthForAll()
{
	extern cvar_t npc_tridepth_all;
	return npc_tridepth_all.value > 0;
}

bool TridepthVertical()
{
	extern cvar_t npc_tridepth_vertical;
	return npc_tridepth_vertical.value > 0;
}

bool AllowUseThroughWalls()
{
#if FEATURE_USE_THROUGH_WALLS_CVAR
	extern cvar_t use_through_walls;
	return use_through_walls.value != 0;
#else
	return true;
#endif
}

bool NpcFollowNearest()
{
	extern cvar_t npc_follow_nearest;
	return npc_follow_nearest.value != 0;
}

bool NpcGetToEnemyNearest()
{
	extern cvar_t npc_get_to_enemy_nearest;
	return npc_get_to_enemy_nearest.value != 0;
}

float NpcForgetEnemyTime()
{
	extern cvar_t npc_forget_enemy_time;
	return npc_forget_enemy_time.value;
}

bool NpcActiveAfterCombat()
{
	extern cvar_t npc_active_after_combat;
	return npc_active_after_combat.value != 0;
}

bool NpcFollowOutOfPvs()
{
	extern cvar_t npc_follow_out_of_pvs;
	return npc_follow_out_of_pvs.value != 0;
}

bool NpcFixMeleeDistance()
{
#if FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR
	extern cvar_t npc_fix_melee_distance;
	return npc_fix_melee_distance.value != 0;
#else
	return false;
#endif
}

bool AllowGrenadeJump()
{
#if FEATURE_GRENADE_JUMP_CVAR
	extern cvar_t grenade_jump;
	return grenade_jump.value != 0;
#else
	return true;
#endif
}
