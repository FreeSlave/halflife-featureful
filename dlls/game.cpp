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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "game.h"
#include "mod_features.h"
#include "parsetext.h"
#include "weapon_ids.h"
#include "saverestore.h"
#include "locus.h"
#include "ammo_amounts.h"
#include "inventory.h"
#include "soundscripts.h"
#include "visuals.h"
#include "warpball.h"
#include "ent_templates.h"
#include "followers.h"
#include "savetitles.h"
#include "objecthint_spec.h"
#include "vcs_info.h"
#include "tex_materials.h"
#include "error_collector.h"
#include "weapons.h"
#include "weapon_templates.h"
#include "ai_debug.h"

#include <chrono>

ModFeatures g_modFeatures;

struct WeaponNameAndId
{
	WeaponNameAndId(const char* n, int i): name(n), id(i) {}
	const char* name;
	int id;
};

cvar_t corpse_player_collision_fix = {"corpse_player_collision_fix", "0"};
cvar_t doors_open_in_move_direction = {"doors_open_in_move_direction", "0"};
cvar_t doors_blocked_recheck = {"doors_blocked_recheck", "0"};
cvar_t doors_blocked_fade_corpses = {"doors_blocked_fade_corpses", "0"};

ModFeatures::ModFeatures()
{
	memset(monsters, 0, sizeof(monsters));
	memset(weapons, 0, sizeof(weapons));
	memset(maxAmmos, 0, sizeof(maxAmmos));
	monstersCount = 0;
	maxAmmoCount = 0;

	EnableDefaultWeapons();

	player_maxhealth = MAX_NORMAL_HEALTH;
	player_maxarmor = MAX_NORMAL_BATTERY;

	suit_light = SUIT_LIGHT_FLASHLIGHT;
	suit_light_allow_both = false;
	suit_sentences = true;
	hev_dead_requires_suit = false;
	nosuit_allow_healthcharger = false;
	items_instant_drop = true;
	tripmines_solid = FEATURE_OPFOR_SPECIFIC ? false : true;
	satchels_pickable = true;
	weapon_p_models = true;

	alien_teleport_sound = false;
	warpball_at_monster_center = true;

	monsters_stop_attacking_dying_monsters = false;
	monsters_delegate_squad_leadership = true;
	monsters_eat_for_health = true;
	monsters_spawned_named_wait_trigger = true;
	monsters_open_named_doors = true;
	dying_monsters_block_player = true;
	corpse_player_collision_fix = false;

	blackops_classify = false;
	opfor_grunts_dislike_civilians = FEATURE_OPFOR_SPECIFIC ? true : false;
	medic_drop_healthkit = false;

	racex_dislike_alien_military = true;
	racex_dislike_gargs = true;
	racex_dislike_alien_monsters = false;

	scientist_random_heads =  4;

	vortigaunt_coil_attack = true;
	vortigaunt_idle_effects = false;
	vortigaunt_arm_boost = true;
	vortigaunt_selfheal = true;
	vortigaunt_heal = true;
	vortigaunt_revive = true;
	vortigaunt_armor_charge = false;

	sentry_retract = true;

	bigmomma_wait_fix = false;
	bigmomma_lastnode_fix = false;

	gonome_lock_player = false;

	doors_open_in_move_direction = false;
	doors_blocked_recheck = false;
	doors_blocked_fade_corpses = false;
	door_rotating_starts_open_fix = false;

	env_spark_transit = false;

	skill_opfor = FEATURE_OPFOR_SPECIFIC ? true : false;
	opfor_decals = FEATURE_OPFOR_SPECIFIC ? true : false;
	opfor_deadhaz = FEATURE_OPFOR_SPECIFIC ? true : false;
	tentacle_opfor_height = FEATURE_OPFOR_SPECIFIC ? true : false;
}

template <typename T>
struct KeyValueDefinition
{
	const char* name;
	T& value;
};

#define KEY_VALUE_DEF(name) { #name, name }

bool ModFeatures::SetValue(const char *key, const char *value)
{
	KeyValueDefinition<bool> booleans[] = {
		KEY_VALUE_DEF(suit_light_allow_both),
		KEY_VALUE_DEF(suit_sentences),
		KEY_VALUE_DEF(hev_dead_requires_suit),
		KEY_VALUE_DEF(nosuit_allow_healthcharger),
		KEY_VALUE_DEF(items_instant_drop),
		KEY_VALUE_DEF(tripmines_solid),
		KEY_VALUE_DEF(satchels_pickable),
		KEY_VALUE_DEF(weapon_p_models),
		KEY_VALUE_DEF(alien_teleport_sound),
		KEY_VALUE_DEF(warpball_at_monster_center),
		KEY_VALUE_DEF(monsters_stop_attacking_dying_monsters),
		KEY_VALUE_DEF(monsters_delegate_squad_leadership),
		KEY_VALUE_DEF(monsters_eat_for_health),
		KEY_VALUE_DEF(monsters_spawned_named_wait_trigger),
		KEY_VALUE_DEF(monsters_open_named_doors),
		KEY_VALUE_DEF(dying_monsters_block_player),
		KEY_VALUE_DEF(corpse_player_collision_fix),
		KEY_VALUE_DEF(blackops_classify),
		KEY_VALUE_DEF(opfor_grunts_dislike_civilians),
		KEY_VALUE_DEF(medic_drop_healthkit),
		KEY_VALUE_DEF(racex_dislike_alien_military),
		KEY_VALUE_DEF(racex_dislike_gargs),
		KEY_VALUE_DEF(racex_dislike_alien_monsters),
		KEY_VALUE_DEF(vortigaunt_coil_attack),
		KEY_VALUE_DEF(vortigaunt_idle_effects),
		KEY_VALUE_DEF(vortigaunt_arm_boost),
		KEY_VALUE_DEF(vortigaunt_selfheal),
		KEY_VALUE_DEF(vortigaunt_heal),
		KEY_VALUE_DEF(vortigaunt_revive),
		KEY_VALUE_DEF(vortigaunt_armor_charge),
		KEY_VALUE_DEF(sentry_retract),
		KEY_VALUE_DEF(bigmomma_wait_fix),
		KEY_VALUE_DEF(bigmomma_lastnode_fix),
		KEY_VALUE_DEF(gonome_lock_player),
		KEY_VALUE_DEF(doors_open_in_move_direction),
		KEY_VALUE_DEF(doors_blocked_recheck),
		KEY_VALUE_DEF(doors_blocked_fade_corpses),
		KEY_VALUE_DEF(door_rotating_starts_open_fix),
		KEY_VALUE_DEF(env_spark_transit),
		KEY_VALUE_DEF(skill_opfor),
		KEY_VALUE_DEF(opfor_decals),
		KEY_VALUE_DEF(opfor_deadhaz),
		KEY_VALUE_DEF(tentacle_opfor_height),
	};

	unsigned int i = 0;
	for (i = 0; i<ARRAYSIZE(booleans); ++i)
	{
		if (strcmp(key, booleans[i].name) == 0)
		{
			return UpdateBoolean(value, booleans[i].value, key);
		}
	}

	KeyValueDefinition<int> integers[] = {
		KEY_VALUE_DEF(player_maxhealth),
		KEY_VALUE_DEF(player_maxarmor),
		KEY_VALUE_DEF(scientist_random_heads),
	};

	for (i = 0; i<ARRAYSIZE(integers); ++i)
	{
		if (strcmp(key, integers[i].name) == 0)
		{
			return UpdateInteger(value, integers[i].value, key);
		}
	}

	if (strcmp(key, "suit_light") == 0)
	{
		if (strcmp(value, "nothing") == 0 || strcmp(value, "no") == 0)
			suit_light = SUIT_LIGHT_NOTHING;
		else if (strcmp(value, "flashlight") == 0)
			suit_light = SUIT_LIGHT_FLASHLIGHT;
		else if (strcmp(value, "nvg") == 0 || strcmp(value, "nightvision") == 0)
			suit_light = SUIT_LIGHT_NVG;
		else
		{
			ALERT(at_console, "Parameter '%s' should be one of the following: nothing, flashlight, nvg", key);
			return false;
		}
		return true;
	}

	ALERT(at_console, "Unknown mod feature key '%s'\n", key);
	return false;
}

void ModFeatures::SetMaxAmmo(const char *name, int maxAmmo)
{
	if (maxAmmo <= 0)
	{
		ALERT(at_console, "Invalid max ammo value for ammo '%s'\n", name);
		return;
	}
	for (int i = 0; i<MAX_AMMO_TYPES; ++i)
	{
		if (stricmp(name, maxAmmos[i].name) == 0)
		{
			maxAmmos[i].maxAmmo = maxAmmo;
			return;
		}
	}
	if (maxAmmoCount >= MAX_AMMO_TYPES)
	{
		ALERT(at_console, "Can't add a new ammo type '%s', max count is reached\n", name);
		return;
	}
	strncpyEnsureTermination(maxAmmos[maxAmmoCount].name, name);
	maxAmmos[maxAmmoCount].maxAmmo = maxAmmo;
	maxAmmoCount++;
}

bool ModFeatures::UpdateBoolean(const char *value, bool &result, const char *key)
{
	bool success = ParseBoolean(value, result);
	if (!success)
		ALERT(at_console, "Parameter '%s' expected a boolean value, got '%s' instead\n", key, value);
	return success;
}

bool ModFeatures::UpdateInteger(const char *value, int &result, const char *key)
{
	bool success = ParseInteger(value, result);
	if (!success)
		ALERT(at_console, "Parameter '%s' expected an integer value, got '%s' instead\n", key, value);
	return success;
}

bool ModFeatures::UpdateColor(const char *value, int &result, const char *key)
{
	bool success = ParseColor(value, result);
	if (!success)
		ALERT(at_console, "Parameter '%s' expected a color value, got '%s' instead\n", key, value);
	return success;
}

bool ModFeatures::UpdateFloat(const char *value, float &result, const char *key)
{
	bool success = ParseFloat(value, result);
	if (!success)
		ALERT(at_console, "Parameter '%s' expected an floating-point value, got '%s' instead\n", key, value);
	return success;
}

bool ModFeatures::EnableWeapon(const char *name, bool enable)
{
	static const WeaponNameAndId knownWeapons[] = {
		WeaponNameAndId("crowbar", WEAPON_CROWBAR),
		WeaponNameAndId("9mmhandgun", WEAPON_GLOCK),
		WeaponNameAndId("glock", WEAPON_GLOCK),
		WeaponNameAndId("357", WEAPON_PYTHON),
		WeaponNameAndId("python", WEAPON_PYTHON),
		WeaponNameAndId("9mmAR", WEAPON_MP5),
		WeaponNameAndId("mp5", WEAPON_MP5),
		WeaponNameAndId("shotgun", WEAPON_SHOTGUN),
		WeaponNameAndId("crossbow", WEAPON_CROSSBOW),
		WeaponNameAndId("rpg", WEAPON_RPG),
		WeaponNameAndId("gauss", WEAPON_GAUSS),
		WeaponNameAndId("egon", WEAPON_EGON),
		WeaponNameAndId("hornetgun", WEAPON_HORNETGUN),
		WeaponNameAndId("handgrenade", WEAPON_HANDGRENADE),
		WeaponNameAndId("satchel", WEAPON_SATCHEL),
		WeaponNameAndId("tripmine", WEAPON_TRIPMINE),
		WeaponNameAndId("snark", WEAPON_SNARK),
		WeaponNameAndId("pipewrench", WEAPON_PIPEWRENCH),
		WeaponNameAndId("knife", WEAPON_KNIFE),
		WeaponNameAndId("medkit", WEAPON_MEDKIT),
		WeaponNameAndId("grapple", WEAPON_GRAPPLE),
		WeaponNameAndId("eagle", WEAPON_EAGLE),
		WeaponNameAndId("m249", WEAPON_M249),
		WeaponNameAndId("sniperrifle", WEAPON_SNIPERRIFLE),
		WeaponNameAndId("displacer", WEAPON_DISPLACER),
		WeaponNameAndId("sporelauncher", WEAPON_SPORELAUNCHER),
		WeaponNameAndId("shockrifle", WEAPON_SHOCKRIFLE),
		WeaponNameAndId("penguin", WEAPON_PENGUIN),
		WeaponNameAndId("uzi", WEAPON_UZI),
		WeaponNameAndId("pistol", WEAPON_PISTOL),
		WeaponNameAndId("pistol2", WEAPON_PISTOL2),
		WeaponNameAndId("smg", WEAPON_SMG),
		WeaponNameAndId("smg2", WEAPON_SMG2),
		WeaponNameAndId("rifle", WEAPON_RIFLE),
		WeaponNameAndId("rifle2", WEAPON_RIFLE2),
		WeaponNameAndId("shotgun2", WEAPON_SHOTGUN2),
		WeaponNameAndId("sniperrifle2", WEAPON_SNIPERRIFLE2),
		WeaponNameAndId("camera", WEAPON_CAMERA),
		WeaponNameAndId("radio", WEAPON_RADIO)
	};

	for (unsigned int i=0; i<ARRAYSIZE(knownWeapons); ++i)
	{
		if (stricmp(name, knownWeapons[i].name) == 0)
		{
			weapons[knownWeapons[i].id] = enable;
			return true;
		}
	}
	return false;
}

bool ModFeatures::DisableWeapon(const char *name)
{
	return EnableWeapon(name, false);
}

void ModFeatures::EnableDefaultWeapons()
{
	weapons[WEAPON_CROWBAR] = true;
	weapons[WEAPON_GLOCK] = true;
	weapons[WEAPON_PYTHON] = true;
	weapons[WEAPON_MP5] = true;
	weapons[WEAPON_SHOTGUN] = true;
	weapons[WEAPON_CROSSBOW] = true;
	weapons[WEAPON_RPG] = true;
	weapons[WEAPON_GAUSS] = true;
	weapons[WEAPON_EGON] = true;
	weapons[WEAPON_HORNETGUN] = true;
	weapons[WEAPON_HANDGRENADE] = true;
	weapons[WEAPON_SATCHEL] = true;
	weapons[WEAPON_TRIPMINE] = true;
	weapons[WEAPON_SNARK] = true;
}

void ModFeatures::EnableAllWeapons()
{
	memset(weapons, 1, sizeof(weapons));
}

bool ModFeatures::IsWeaponEnabled(int weaponId) const
{
	return weapons[weaponId];
}

const char* ModFeatures::DesertEagleDropName() const
{
#if FEATURE_DESERT_EAGLE
	if (IsWeaponEnabled(WEAPON_EAGLE))
		return "weapon_eagle";
#endif
	return "ammo_357";
}

const char* ModFeatures::M249DropName() const
{
#if FEATURE_M249
	if (IsWeaponEnabled(WEAPON_M249))
		return "weapon_m249";
#endif
	return "ammo_9mmAR";
}

const char* ModFeatures::DeadHazModel() const
{
	if (opfor_deadhaz)
		return "models/deadhaz.mdl";
	else
		return "models/player.mdl";
}

bool ModFeatures::DisplacerBallEnabled() const
{
#if FEATURE_DISPLACER
	return IsWeaponEnabled(WEAPON_DISPLACER);
#else
	return false;
#endif
}

bool ModFeatures::ShockBeamEnabled() const
{
	if (IsMonsterEnabled("shocktrooper"))
		return true;
#if FEATURE_SHOCKRIFLE
	return IsWeaponEnabled(WEAPON_SHOCKRIFLE);
#else
	return false;
#endif
}

bool ModFeatures::SporesEnabled() const
{
	if (IsMonsterEnabled("shocktrooper"))
		return true;
#if FEATURE_SPORELAUNCHER
	return IsWeaponEnabled(WEAPON_SPORELAUNCHER);
#else
	return false;
#endif
}

void ModFeatures::EnableMonster(const char *name)
{
	for (unsigned int i=0; i<monstersCount; ++i)
	{
		if (strcmp(monsters[i], name) == 0)
		{
			ALERT(at_warning, "Monster '%s' is already enabled\n", name);
			return;
		}
	}

	if (monstersCount >= ARRAYSIZE(monsters))
	{
		ALERT(at_error, "Can't enable monster '%s' due to monster count limit\n", name);
		return;
	}

	strncpyEnsureTermination(monsters[monstersCount], name);
	monstersCount++;
}

bool ModFeatures::IsMonsterEnabled(const char *name) const
{
	// TODO: optimize
	for (unsigned int i=0; i<monstersCount; ++i)
	{
		if (strcmp(monsters[i], name) == 0)
		{
			return true;
		}
	}
	return false;
}

bool ModFeatures::DoorsOpenInMoveDirection() const
{
	return ::doors_open_in_move_direction.value != 0;
}

bool ModFeatures::DoorsRecheckWhenBlocked() const
{
	return ::doors_blocked_recheck.value != 0;
}

bool ModFeatures::DoorsFadeCorpsesWhenBlocked() const
{
	return ::doors_blocked_fade_corpses.value != 0;
}

bool ModFeatures::FixPlayerAndCorpseCollisionBug() const
{
	return ::corpse_player_collision_fix.value != 0;
}

byte* LoadFileForMeWithBackup(const char* fileName, const char* fileNameBackup, int* pFileSize, const char** chosenFileName)
{
	*chosenFileName = fileName;

	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, pFileSize );
	if (!pMemFile)
	{
		pMemFile = g_engfuncs.pfnLoadFileForMe( fileNameBackup, pFileSize );
		if (pMemFile)
		{
			*chosenFileName = fileNameBackup;
		}
	}
	return pMemFile;
}

bool IsNonSignificantLine(const char* line, bool allowMinus = false)
{
	if (!*line || *line == '/')
		return true;
	if (allowMinus)
		return !(IsValidIdentifierCharacter(*line) || *line == '-');
	return !IsValidIdentifierCharacter(*line);
}

char* TryConsumeToken(char* buffer, const int length)
{
	int i = 0;
	SkipSpaces(buffer, i, length);

	if (IsNonSignificantLine(buffer + i, true))
		return NULL;

	int tokenStart = i;
	ConsumeNonSpaceCharacters(buffer, i, length);
	int tokenLength = i - tokenStart;

	if (tokenLength > 0)
	{
		char* token = buffer + tokenStart;
		token[tokenLength] = '\0';
		return token;
	}
	return NULL;
}

enum
{
	CONSUME_VALUE_ONLY_FIRST_TOKEN,
	CONSUME_VALUE_THE_WHOLE,
};

void TryConsumeKeyAndValue(char* buffer, const int length, char*& key, char*& value, int consumeValuePolicy = CONSUME_VALUE_ONLY_FIRST_TOKEN)
{
	int i = 0;
	SkipSpaces(buffer, i, length);

	if (IsNonSignificantLine(buffer + i))
		return;

	const int keyStart = i;
	ConsumeNonSpaceCharacters(buffer, i, length);
	const int keyLength = i - keyStart;
	SkipSpaces(buffer, i, length);
	const int valueStart = i;
	if (consumeValuePolicy == CONSUME_VALUE_ONLY_FIRST_TOKEN)
		ConsumeNonSpaceCharacters(buffer, i, length);
	else
		ConsumeLineSignificantOnly(buffer, i, length);
	const int valueLength = i - valueStart;

	if (keyLength > 0)
	{
		key = buffer + keyStart;
		key[keyLength] = '\0';

		if (valueLength > 0)
		{
			value = buffer + valueStart;
			value[valueLength] = '\0';
		}
	}
}

#define FEATUREFUL_WEAPONS_CONFIG "featureful_weapons.cfg"
#define FEATUREFUL_MONSTERS_CONFIG "featureful_monsters.cfg"
#define FEATUREFUL_SERVER_CONFIG "featureful_server.cfg"

void ReadEnabledWeapons()
{
	const char* fileName;
	int filePos = 0, fileSize;
	byte *pMemFile = LoadFileForMeWithBackup("features/" FEATUREFUL_WEAPONS_CONFIG, FEATUREFUL_WEAPONS_CONFIG, &fileSize, &fileName);
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing enabled weapons from %s\n", fileName);

	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) )
	{
		char* weaponName = TryConsumeToken(buffer, sizeof(buffer));
		if (weaponName)
		{
			bool enable = *weaponName == '-' ? false : true;
			if (g_modFeatures.EnableWeapon(enable ? weaponName : weaponName+1, enable))
				ALERT(at_console, "%s weapon '%s'\n", enable ? "Enabled" : "Disabled", weaponName);
			else
				ALERT(at_warning, "Unknown weapon '%s' in %s\n", weaponName, fileName);
		}
	}
	g_engfuncs.pfnFreeFile( pMemFile );
}

void ReadEnabledMonsters()
{
	const char* fileName;
	int filePos = 0, fileSize;
	byte *pMemFile = LoadFileForMeWithBackup("features/" FEATUREFUL_MONSTERS_CONFIG, FEATUREFUL_MONSTERS_CONFIG, &fileSize, &fileName);
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing enabled monsters from %s\n", fileName);

	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) )
	{
		char* monsterName = TryConsumeToken(buffer, sizeof(buffer));
		if (monsterName)
		{
			ALERT(at_console, "Enabling monster '%s'\n", monsterName);
			g_modFeatures.EnableMonster(monsterName);
		}
	}
	g_engfuncs.pfnFreeFile( pMemFile );
}

void ReadServerFeatures()
{
	const char* fileName;
	int filePos = 0, fileSize;
	byte *pMemFile = LoadFileForMeWithBackup("features/" FEATUREFUL_SERVER_CONFIG, FEATUREFUL_SERVER_CONFIG, &fileSize, &fileName);
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing server features from %s\n", fileName);

	char buffer[512];
	memset(buffer, 0, sizeof(buffer));

	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) )
	{
		char* key = NULL;
		char* value = NULL;
		TryConsumeKeyAndValue(buffer, sizeof(buffer), key, value, CONSUME_VALUE_THE_WHOLE);

		if (key)
		{
			if (value)
			{
				// ALERT(at_console, "Key: '%s'. Value: '%s'\n", key, value);
				g_modFeatures.SetValue(key, value);
			}
			else
			{
				ALERT(at_warning, "Key '%s' without value!\n", key);
			}
		}
	}
	g_engfuncs.pfnFreeFile( pMemFile );
}

void ReadMaxAmmos()
{
	const char* fileName = "features/maxammo.cfg";
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing max ammo values from %s\n", fileName);

	char buffer[512];
	memset(buffer, 0, sizeof(buffer));

	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) )
	{
		char* key = NULL;
		char* value = NULL;
		TryConsumeKeyAndValue(buffer, sizeof(buffer), key, value, CONSUME_VALUE_ONLY_FIRST_TOKEN);

		if (key)
		{
			if (value)
			{
				// ALERT(at_console, "Ammo name: %s, maxAmmo value: %s\n", key, value);
				g_modFeatures.SetMaxAmmo(FixedAmmoName(key), atoi(value));
			}
			else
			{
				ALERT(at_warning, "Key '%s' without value!\n", key);
			}
		}
	}

	g_engfuncs.pfnFreeFile( pMemFile );
}

void ReadAmmoAmounts()
{
	const char* fileName = "features/ammo_amounts.cfg";
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing default ammo amounts for ammo and weapon entities from %s\n", fileName);

	char buffer[512];
	memset(buffer, 0, sizeof(buffer));

	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) )
	{
		char* key = NULL;
		char* value = NULL;
		TryConsumeKeyAndValue(buffer, sizeof(buffer), key, value, CONSUME_VALUE_ONLY_FIRST_TOKEN);

		if (key)
		{
			if (value)
			{
				const int amount = atoi(value);
				if (amount < 0) {
					ALERT(at_warning, "%s has a negative value for ammo amount in %s\n", fileName);
					continue;
				}
				if (g_AmmoAmounts.RegisterAmountForAmmoEnt(key, amount)) {
					ALERT(at_console, "Set default ammo amount for %s to %d\n", key, amount);
				} else {
					ALERT(at_warning, "Repeated definition of ammo amount for %s\n", key);
				}
			}
			else
			{
				ALERT(at_warning, "Key '%s' without value!\n", key);
			}
		}
	}
}

static cvar_t build_commit = { "sv_game_build_commit", g_VCSInfo_Commit };
static cvar_t build_branch = { "sv_game_build_branch", g_VCSInfo_Branch };

cvar_t displaysoundlist = {"displaysoundlist","0"};

// multiplayer server rules
cvar_t fragsleft	= { "mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	  // Don't spam console/log files/users with this changing
cvar_t timeleft		= { "mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	  // "      "

// multiplayer server rules
cvar_t teamplay		= { "mp_teamplay","0", FCVAR_SERVER };
cvar_t fraglimit	= { "mp_fraglimit","0", FCVAR_SERVER };
cvar_t timelimit	= { "mp_timelimit","0", FCVAR_SERVER };
cvar_t friendlyfire	= { "mp_friendlyfire","0", FCVAR_SERVER };
cvar_t falldamage	= { "mp_falldamage","0", FCVAR_SERVER };
cvar_t weaponstay	= { "mp_weaponstay","0", FCVAR_SERVER };
cvar_t dropweapons	= { "mp_dropweapons","1", FCVAR_SERVER };

cvar_t weapon_respawndelay = { "mp_weapon_respawndelay","-2",FCVAR_SERVER };
cvar_t ammo_respawndelay = { "mp_ammo_respawndelay","-2",FCVAR_SERVER };
cvar_t item_respawndelay = { "mp_item_respawndelay","-2",FCVAR_SERVER };
cvar_t healthcharger_rechargetime = { "mp_healthcharger_rechargetime","-2",FCVAR_SERVER };
cvar_t hevcharger_rechargetime = { "mp_hevcharger_rechargetime","-2",FCVAR_SERVER };

cvar_t selfgauss	= { "selfgauss", "0", FCVAR_SERVER };
cvar_t satchelfix	= { "satchelfix", "1", FCVAR_SERVER };
cvar_t explosionfix	= { "explosionfix", "1", FCVAR_SERVER };
cvar_t monsteryawspeedfix	= { "monsteryawspeedfix", "1", FCVAR_SERVER };
cvar_t animeventfix = {"animeventfix", "0", FCVAR_SERVER };
cvar_t anim_attack_reset_fix = {"anim_attack_reset_fix", "1", FCVAR_SERVER };
cvar_t anim_dispatch_fix = {"anim_dispatch_fix", "0", FCVAR_SERVER};
cvar_t npc_run_task_instant = {"npc_run_task_instant", "1", FCVAR_SERVER};
cvar_t corpsephysics = { "corpsephysics", "0", FCVAR_SERVER };
cvar_t pushablemode = { "pushablemode", "0", FCVAR_SERVER };
cvar_t forcerespawn	= { "mp_forcerespawn","1", FCVAR_SERVER };
cvar_t respawndelay	= { "mp_respawndelay","0", FCVAR_SERVER };
cvar_t flashlight	= { "mp_flashlight","0", FCVAR_SERVER };
cvar_t aimcrosshair	= { "mp_autocrosshair","1", FCVAR_SERVER };
cvar_t decalfrequency	= { "decalfrequency","30", FCVAR_SERVER };
cvar_t teamlist		= { "mp_teamlist","hgrunt;scientist", FCVAR_SERVER };
cvar_t teamoverride	= { "mp_teamoverride","1" };
cvar_t defaultteam	= { "mp_defaultteam","0" };

cvar_t allowmonsters	= { "mp_allowmonsters","0", FCVAR_SERVER };
cvar_t mp_allowmonsterinfo = { "mp_allowmonsterinfo","0", FCVAR_SERVER };
cvar_t sp_allowmonsterinfo = { "sp_allowmonsterinfo","0", FCVAR_SERVER };
cvar_t npc_dropweapons = { "npc_dropweapons", "1", FCVAR_SERVER };
cvar_t dmgperscore = { "mp_dmgperscore", "0", FCVAR_SERVER };
cvar_t allydmgpenalty = { "mp_allydmgpenalty", "2", FCVAR_SERVER };
cvar_t npckill = { "mp_npckill", "1", FCVAR_SERVER };

cvar_t sv_bunnyhop		= { "sv_bunnyhop", "0", FCVAR_SERVER };

cvar_t allow_spectators = { "allow_spectators", "0", FCVAR_SERVER };	// 0 prevents players from being spectators

#if FEATURE_USE_THROUGH_WALLS_CVAR
cvar_t use_through_walls = { "use_through_walls", "1", FCVAR_SERVER };
#endif
cvar_t items_physics_fix = { "items_physics_fix", "0", FCVAR_SERVER };
cvar_t npc_tridepth = { "npc_tridepth", "1", FCVAR_SERVER };
cvar_t npc_tridepth_all = { "npc_tridepth_all", "0", FCVAR_SERVER };
cvar_t npc_tridepth_vertical = { "npc_tridepth_vertical", "0", FCVAR_SERVER };
cvar_t npc_follow_nearest = { "npc_follow_nearest", "0", FCVAR_SERVER };
cvar_t npc_get_to_enemy_nearest = { "npc_get_to_enemy_nearest", "0", FCVAR_SERVER };
cvar_t npc_forget_enemy_time = { "npc_forget_enemy_time", "0", FCVAR_SERVER };
cvar_t npc_trace_hull_attack_retry = { "npc_trace_hull_attack_retry", "0", FCVAR_SERVER };
#if FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR
cvar_t npc_fix_melee_distance = { "npc_fix_melee_distance", "0", FCVAR_SERVER };
#endif
cvar_t npc_active_after_combat = { "npc_active_after_combat", "0", FCVAR_SERVER };
cvar_t npc_combat_fail_schedule = { "npc_combat_fail_schedule", "0", FCVAR_SERVER };
cvar_t npc_lateral_retreat = { "npc_lateral_retreat", "0", FCVAR_SERVER };
cvar_t npc_follow_out_of_pvs = { "npc_follow_out_of_pvs", "1", FCVAR_SERVER };
cvar_t npc_patrol = { "npc_patrol", "1", FCVAR_SERVER };
cvar_t npc_vanilla_kick_behavior = { "npc_vanilla_kick_behavior", "0", FCVAR_SERVER };

cvar_t mp_chattime	= { "mp_chattime","10", FCVAR_SERVER };

cvar_t pickup_policy = { "pickup_policy","0", FCVAR_SERVER };

#if FEATURE_GRENADE_JUMP_CVAR
cvar_t grenade_jump = { "grenade_jump","1", FCVAR_SERVER };
#endif

cvar_t findnearestnodefix = { "findnearestnodefix", "1", FCVAR_SERVER };
cvar_t nodegraph_distinfo_sort_fix = {"nodegraph_distinfo_sort_fix", "0", FCVAR_SERVER};

cvar_t keepinventory	= { "mp_keepinventory","0", FCVAR_SERVER }; // keep inventory across level transitions in multiplayer coop

// Engine Cvars
cvar_t *g_psv_gravity = NULL;
cvar_t *g_psv_maxspeed = NULL;
cvar_t *g_psv_aim = NULL;
cvar_t *g_psv_allow_autoaim = NULL;
cvar_t *g_footsteps = NULL;
cvar_t *g_enable_cheats = NULL;

cvar_t *g_psv_developer = NULL;

#define DECLARE_SKILL_VALUE(name, defaultValue) \
cvar_t name ## 1 = { #name "1", defaultValue, 0, 0, 0 }; \
cvar_t name ## 2 = { #name "2", defaultValue, 0, 0, 0 }; \
cvar_t name ## 3 = { #name "3", defaultValue, 0, 0, 0 };

#define DECLARE_SKILL_VALUE3(name, defaultValue1, defaultValue2, defaultValue3) \
cvar_t name ## 1 = { #name "1", defaultValue1, 0, 0, 0 }; \
cvar_t name ## 2 = { #name "2", defaultValue2, 0, 0, 0 }; \
cvar_t name ## 3 = { #name "3", defaultValue3, 0, 0, 0 };

#define REGISTER_SKILL_CVARS(name) \
CVAR_REGISTER( &name ## 1 ); \
CVAR_REGISTER( &name ## 2 ); \
CVAR_REGISTER( &name ## 3 );

//CVARS FOR SKILL LEVEL SETTINGS
// Agrunt
DECLARE_SKILL_VALUE(sk_agrunt_health, "0")
DECLARE_SKILL_VALUE(sk_agrunt_dmg_punch, "0")

// Apache
DECLARE_SKILL_VALUE(sk_apache_health, "0")

#if FEATURE_BABYGARG
// Baby Gargantua
DECLARE_SKILL_VALUE(sk_babygargantua_health, "0")
DECLARE_SKILL_VALUE(sk_babygargantua_dmg_slash, "0")
DECLARE_SKILL_VALUE(sk_babygargantua_dmg_fire, "0")
DECLARE_SKILL_VALUE(sk_babygargantua_dmg_stomp, "0")
#endif

// Barnacle
DECLARE_SKILL_VALUE(sk_barnacle_health, "25")

// Barney
DECLARE_SKILL_VALUE(sk_barney_health, "0")

// Bullsquid
DECLARE_SKILL_VALUE(sk_bullsquid_health, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_whip, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_spit, "0")
DECLARE_SKILL_VALUE3(sk_bullsquid_toxicity, "0", "1", "1")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_toxic_poison, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_toxic_impact, "0")

// Big Momma
DECLARE_SKILL_VALUE(sk_bigmomma_health_factor, "1.0")
DECLARE_SKILL_VALUE(sk_bigmomma_dmg_slash, "50")
DECLARE_SKILL_VALUE(sk_bigmomma_dmg_blast, "100")
DECLARE_SKILL_VALUE(sk_bigmomma_radius_blast, "250")

#if FEATURE_CLEANSUIT_SCIENTIST
// Cleansuit Scientist
DECLARE_SKILL_VALUE(sk_cleansuit_scientist_health, "0")
#endif

#if FEATURE_FLYBEE
// Flybee
DECLARE_SKILL_VALUE(sk_flybee_health, "0")
DECLARE_SKILL_VALUE(sk_flybee_dmg_kick, "20")
DECLARE_SKILL_VALUE(sk_flybee_dmg_beam, "50")
DECLARE_SKILL_VALUE(sk_flybee_dmg_flyball, "20")
#endif

// Gargantua
DECLARE_SKILL_VALUE(sk_gargantua_health, "0")
DECLARE_SKILL_VALUE(sk_gargantua_dmg_slash, "0")
DECLARE_SKILL_VALUE(sk_gargantua_dmg_fire, "0")
DECLARE_SKILL_VALUE(sk_gargantua_dmg_stomp, "0")

// Hassassin
DECLARE_SKILL_VALUE(sk_hassassin_health, "0")
DECLARE_SKILL_VALUE3(sk_hassassin_cloaking, "0", "0", "1")

// Headcrab
DECLARE_SKILL_VALUE(sk_headcrab_health, "0")
DECLARE_SKILL_VALUE(sk_headcrab_dmg_bite, "0")

#if FEATURE_OPFOR_GRUNT
// Opposing Force Hgrunt
DECLARE_SKILL_VALUE(sk_hgrunt_ally_health, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_ally_kick, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_ally_pellets, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_ally_gspeed, "0")

// Medic
DECLARE_SKILL_VALUE(sk_medic_ally_health, "0")
DECLARE_SKILL_VALUE(sk_medic_ally_kick, "0")
DECLARE_SKILL_VALUE(sk_medic_ally_gspeed, "0")
DECLARE_SKILL_VALUE(sk_medic_ally_heal, "0")

// Torch
DECLARE_SKILL_VALUE(sk_torch_ally_health, "0")
DECLARE_SKILL_VALUE(sk_torch_ally_kick, "0")
DECLARE_SKILL_VALUE(sk_torch_ally_gspeed, "0")
#endif

// Hgrunt 
DECLARE_SKILL_VALUE(sk_hgrunt_health, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_kick, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_pellets, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_gspeed, "0")

#if FEATURE_HWGRUNT
DECLARE_SKILL_VALUE(sk_hwgrunt_health, "0")
#endif

// Houndeye
DECLARE_SKILL_VALUE(sk_houndeye_health, "0")
DECLARE_SKILL_VALUE(sk_houndeye_dmg_blast, "0")

// ISlave
DECLARE_SKILL_VALUE(sk_islave_health, "0")
DECLARE_SKILL_VALUE(sk_islave_dmg_claw, "0")
DECLARE_SKILL_VALUE(sk_islave_dmg_clawrake, "0")
DECLARE_SKILL_VALUE(sk_islave_dmg_zap, "0")

DECLARE_SKILL_VALUE3(sk_islave_zap_rate, "1", "1", "1.5")
DECLARE_SKILL_VALUE3(sk_islave_revival, "0", "0", "1")

// Icthyosaur
DECLARE_SKILL_VALUE(sk_ichthyosaur_health, "0")
DECLARE_SKILL_VALUE(sk_ichthyosaur_shake, "0")

// Leech
DECLARE_SKILL_VALUE(sk_leech_health, "0")
DECLARE_SKILL_VALUE(sk_leech_dmg_bite, "0")

// Controller
DECLARE_SKILL_VALUE(sk_controller_health, "0")
DECLARE_SKILL_VALUE(sk_controller_dmgzap, "0")
DECLARE_SKILL_VALUE(sk_controller_speedball, "0")
DECLARE_SKILL_VALUE(sk_controller_dmgball, "0")

#if FEATURE_MASSN
// Massassin
DECLARE_SKILL_VALUE(sk_massassin_health, "0")
DECLARE_SKILL_VALUE(sk_massassin_kick, "0")
DECLARE_SKILL_VALUE(sk_massassin_gspeed, "0")
#endif

// Nihilanth
DECLARE_SKILL_VALUE(sk_nihilanth_health, "0")
DECLARE_SKILL_VALUE(sk_nihilanth_zap, "0")

// Osprey
DECLARE_SKILL_VALUE(sk_osprey, "400")

#if FEATURE_BLACK_OSPREY
// Blackops Osprey
DECLARE_SKILL_VALUE(sk_blkopsosprey, "0")
#endif

#if FEATURE_OTIS
// Otis
DECLARE_SKILL_VALUE(sk_otis_health, "0")
#endif

#if FEATURE_KATE
// Kate
DECLARE_SKILL_VALUE(sk_kate_health, "0")
#endif

#if FEATURE_PANTHEREYE
// Panthereye
DECLARE_SKILL_VALUE(sk_panthereye_health, "150")
DECLARE_SKILL_VALUE(sk_panthereye_dmg_claw, "20")
#endif

#if FEATURE_PITDRONE
// Pitdrone
DECLARE_SKILL_VALUE(sk_pitdrone_health, "0")
DECLARE_SKILL_VALUE(sk_pitdrone_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_pitdrone_dmg_whip, "0")
DECLARE_SKILL_VALUE(sk_pitdrone_dmg_spit, "0")
#endif

#if FEATURE_PITWORM
// Pitworm
DECLARE_SKILL_VALUE(sk_pitworm_health, "0")
DECLARE_SKILL_VALUE(sk_pitworm_dmg_swipe, "0")
DECLARE_SKILL_VALUE(sk_pitworm_dmg_beam, "0")
#endif

#if FEATURE_GENEWORM
// Geneworm
DECLARE_SKILL_VALUE(sk_geneworm_health, "0")
DECLARE_SKILL_VALUE(sk_geneworm_dmg_spit, "0")
DECLARE_SKILL_VALUE(sk_geneworm_dmg_hit, "0")
#endif

// Scientist
DECLARE_SKILL_VALUE(sk_scientist_health, "0")

#if FEATURE_ROBOGRUNT
DECLARE_SKILL_VALUE(sk_rgrunt_explode, "0")
#endif

#if FEATURE_SHOCKTROOPER
// Shock Roach
DECLARE_SKILL_VALUE(sk_shockroach_health, "0")
DECLARE_SKILL_VALUE(sk_shockroach_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_shockroach_lifespan, "0")

// ShockTrooper
DECLARE_SKILL_VALUE(sk_shocktrooper_health, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_kick, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_gspeed, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_maxcharge, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_rchgspeed, "0")
#endif

// Snark
DECLARE_SKILL_VALUE(sk_snark_health, "0")
DECLARE_SKILL_VALUE(sk_snark_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_snark_dmg_pop, "0")

#if FEATURE_VOLTIFORE
// Voltigore
DECLARE_SKILL_VALUE(sk_voltigore_health, "0")
DECLARE_SKILL_VALUE(sk_voltigore_dmg_punch, "0")
DECLARE_SKILL_VALUE(sk_voltigore_dmg_beam, "0")
DECLARE_SKILL_VALUE(sk_voltigore_dmg_explode, "0")

// Baby Voltigore
DECLARE_SKILL_VALUE(sk_babyvoltigore_health, "0")
DECLARE_SKILL_VALUE(sk_babyvoltigore_dmg_punch, "0")
#endif

// Zombie
DECLARE_SKILL_VALUE(sk_zombie_health, "0")
DECLARE_SKILL_VALUE(sk_zombie_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_zombie_dmg_both_slash, "0")

#if FEATURE_ZOMBIE_BARNEY
// Zombie Barney
DECLARE_SKILL_VALUE(sk_zombie_barney_health, "0")
DECLARE_SKILL_VALUE(sk_zombie_barney_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_zombie_barney_dmg_both_slash, "0")
#endif

#if FEATURE_ZOMBIE_SOLDIER
// Zombie Soldier
DECLARE_SKILL_VALUE(sk_zombie_soldier_health, "0")
DECLARE_SKILL_VALUE(sk_zombie_soldier_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_zombie_soldier_dmg_both_slash, "0")
#endif

#if FEATURE_GONOME
// Gonome
DECLARE_SKILL_VALUE(sk_gonome_health, "0")
DECLARE_SKILL_VALUE(sk_gonome_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_gonome_dmg_guts, "0")
DECLARE_SKILL_VALUE(sk_gonome_dmg_one_bite, "0")
#endif

#if FEATURE_FLOATER
// Floater
DECLARE_SKILL_VALUE(sk_floater_health, "0")
DECLARE_SKILL_VALUE(sk_floater_explode, "0")
#endif

//Turret
DECLARE_SKILL_VALUE(sk_turret_health, "0")

// MiniTurret
DECLARE_SKILL_VALUE(sk_miniturret_health, "0")

// Sentry Turret
DECLARE_SKILL_VALUE(sk_sentry_health, "0")

#if FEATURE_ROBOCOP
// Robocop
DECLARE_SKILL_VALUE(sk_robocop_health, "0")
DECLARE_SKILL_VALUE(sk_robocop_dmg_mortar, "0")
DECLARE_SKILL_VALUE(sk_robocop_dmg_fist, "0")
DECLARE_SKILL_VALUE(sk_robocop_sw_radius, "0")
#endif

DECLARE_SKILL_VALUE(sk_tor_health, "0")
DECLARE_SKILL_VALUE(sk_tor_punch, "0")
DECLARE_SKILL_VALUE(sk_tor_energybeam, "0")
DECLARE_SKILL_VALUE(sk_tor_sonicblast, "0")

DECLARE_SKILL_VALUE(sk_zaptrap_sense_radius, "244")
DECLARE_SKILL_VALUE(sk_zaptrap_respawn_time, "18")

// PLAYER WEAPONS

// Crowbar whack
DECLARE_SKILL_VALUE(sk_plr_crowbar, "0")

// Glock Round
DECLARE_SKILL_VALUE(sk_plr_9mm_bullet, "0")

// 357 Round
DECLARE_SKILL_VALUE(sk_plr_357_bullet, "0")

// MP5 Round
DECLARE_SKILL_VALUE(sk_plr_9mmAR_bullet, "0")

// M203 grenade
DECLARE_SKILL_VALUE(sk_plr_9mmAR_grenade, "0")

// Shotgun buckshot
DECLARE_SKILL_VALUE(sk_buckshot, "0")
DECLARE_SKILL_VALUE(sk_plr_buckshot, "0")

// Crossbow
DECLARE_SKILL_VALUE(sk_plr_xbow_bolt_client, "0")
DECLARE_SKILL_VALUE(sk_plr_xbow_bolt_monster, "0")

// RPG
DECLARE_SKILL_VALUE(sk_plr_rpg, "0")

// Tau Cannon
DECLARE_SKILL_VALUE(sk_plr_gauss, "0")

// Gluon Gun
DECLARE_SKILL_VALUE(sk_plr_egon_narrow, "0")
DECLARE_SKILL_VALUE(sk_plr_egon_wide, "0")

// Hand Grendade
DECLARE_SKILL_VALUE(sk_plr_hand_grenade, "0")

// Satchel Charge
DECLARE_SKILL_VALUE(sk_plr_satchel, "0")

// Tripmine
DECLARE_SKILL_VALUE(sk_plr_tripmine, "0")

#if FEATURE_DESERT_EAGLE
// Desert Eagle
DECLARE_SKILL_VALUE(sk_plr_eagle, "0")
#endif

#if FEATURE_PIPEWRENCH
// Pipe wrench
DECLARE_SKILL_VALUE(sk_plr_pipewrench, "0")
#endif

#if FEATURE_KNIFE
// Knife
DECLARE_SKILL_VALUE(sk_plr_knife, "0")
#endif

#if FEATURE_GRAPPLE
// Grapple
DECLARE_SKILL_VALUE(sk_plr_grapple, "0")
#endif

#if FEATURE_M249
// M249
DECLARE_SKILL_VALUE(sk_plr_556_bullet, "0")
#endif

#if FEATURE_SNIPERRIFLE
// Sniper rifle
DECLARE_SKILL_VALUE(sk_plr_762_bullet, "0")
#endif

#if FEATURE_MEDKIT
// Medkit
DECLARE_SKILL_VALUE(sk_plr_medkitshot, "10")

DECLARE_SKILL_VALUE3(sk_plr_medkittime, "3", "5", "0")
#endif

// WORLD WEAPONS
DECLARE_SKILL_VALUE(sk_12mm_bullet, "0")
DECLARE_SKILL_VALUE(sk_9mmAR_bullet, "0")
DECLARE_SKILL_VALUE(sk_9mm_bullet, "0")
DECLARE_SKILL_VALUE(sk_357_bullet, "34")
DECLARE_SKILL_VALUE(sk_556_bullet, "15")
DECLARE_SKILL_VALUE3(sk_762_bullet, "35", "40", "40")

#if FEATURE_SHOCKBEAM
DECLARE_SKILL_VALUE(sk_plr_shockroachs, "0")
DECLARE_SKILL_VALUE(sk_plr_shockroachm, "0")
DECLARE_SKILL_VALUE(sk_shockroach, "0")
#endif

#if FEATURE_SPOREGRENADE
DECLARE_SKILL_VALUE(sk_plr_spore, "0")
#endif

#if FEATURE_DISPLACER
DECLARE_SKILL_VALUE(sk_plr_displacer_other, "0")
DECLARE_SKILL_VALUE(sk_plr_displacer_radius, "0")
#endif

#if FEATURE_UZI
DECLARE_SKILL_VALUE(sk_plr_uzi, "6")
#endif

// HORNET
DECLARE_SKILL_VALUE(sk_hornet_dmg, "0")
DECLARE_SKILL_VALUE(sk_plr_hornet_dmg, "7")

// MORTAR
DECLARE_SKILL_VALUE(sk_mortar, "200")

// HEALTH/CHARGE
DECLARE_SKILL_VALUE(sk_suitcharger, "0")
DECLARE_SKILL_VALUE(sk_battery, "0")
DECLARE_SKILL_VALUE(sk_healthcharger, "0")
DECLARE_SKILL_VALUE(sk_healthkit, "0")
DECLARE_SKILL_VALUE(sk_scientist_heal, "0")
DECLARE_SKILL_VALUE(sk_scientist_heal_time, "60")
DECLARE_SKILL_VALUE(sk_soda, "1")
DECLARE_SKILL_VALUE(sk_vortigaunt_armor_charge, "0")

// monster damage adjusters
DECLARE_SKILL_VALUE(sk_monster_head, "2")
DECLARE_SKILL_VALUE(sk_monster_chest, "1")
DECLARE_SKILL_VALUE(sk_monster_stomach, "1")
DECLARE_SKILL_VALUE(sk_monster_arm, "1")
DECLARE_SKILL_VALUE(sk_monster_leg, "1")

// player damage adjusters
DECLARE_SKILL_VALUE(sk_player_head, "2")
DECLARE_SKILL_VALUE(sk_player_chest, "1")
DECLARE_SKILL_VALUE(sk_player_stomach, "1")
DECLARE_SKILL_VALUE(sk_player_arm, "1")
DECLARE_SKILL_VALUE(sk_player_leg, "1")

// flashlight settings
DECLARE_SKILL_VALUE(sk_flashlight_drain_time, "120")
DECLARE_SKILL_VALUE(sk_flashlight_charge_time, "20")

DECLARE_SKILL_VALUE(sk_plr_armor_strength, "2")

// END Cvars for Skill Level settings

void Cmd_ReportAIState()
{
	ReportAIStateByClassname(CMD_ARGV( 1 ));
}

void Cmd_AddScheduleWatcher()
{
	const char* classnameOrEntIndex = CMD_ARGV(1);
	if (!classnameOrEntIndex || !*classnameOrEntIndex)
	{
		ALERT(at_console, "Must provide an argument!\n");
		return;
	}
	int entindex = atoi(classnameOrEntIndex);
	if (entindex != 0)
	{
		if (entindex > 0)
		{
			CBaseMonster* pMonster = nullptr;
			edict_t* edict = INDEXENT(entindex);
			if (edict)
			{
				CBaseEntity* pEntity = CBaseEntity::Instance(edict);
				if (pEntity)
				{
					pMonster = pEntity->MyMonsterPointer();
				}
			}
			if (pMonster)
			{
				ALERT(at_aiconsole, "Adding monster \"%s\" with entindex %d to the schedule watcher\n", STRING(pMonster->pev->classname), entindex);
				AddScheduleWatcher(entindex);
			}
			else
			{
				ALERT(at_aiconsole, "Entity with entindex %d is not a monster!\n", entindex);
			}
		}
	}
	else
	{
		CBaseEntity* pEntity = 0;
		ALERT(at_console, "Adding all monsters of \"%s\" classname to the schedule watcher\n", classnameOrEntIndex);
		while((pEntity = UTIL_FindEntityByClassname(pEntity, classnameOrEntIndex)) != 0) {
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster) {
				AddScheduleWatcher(pMonster->entindex());
			}
		}
	}
}

void Cmd_NumberOfEntities()
{
	if (CMD_ARGC() > 1)
	{
		const char* className = CMD_ARGV(1);
		if (className && *className)
		{
			int count = 0;
			CBaseEntity* pEntity = nullptr;
			while ((pEntity = UTIL_FindEntityByClassname(pEntity, className)) != nullptr)
			{
				count++;
			}
			ALERT(at_console, "%d\n", count);
		}
	}
	else
	{
		ALERT(at_console, "%d / %d\n", NUMBER_OF_ENTITIES(), gpGlobals->maxEntities);
	}
}

static bool CanRunCheatCommand()
{
	if (CheatsEnabled())
		return true;
	ALERT(at_console, "%s is available only when cheats enabled\n", CMD_ARGV(0));
	return false;
}

void Cmd_SetGlobalState()
{
	if (!CanRunCheatCommand())
		return;
	if (CMD_ARGC() < 3)
	{
		ALERT(at_console, "Usage: %s <globalname> <off|on|dead>\n", CMD_ARGV(0));
		return;
	}
	const char* globalName = CMD_ARGV(1);
	if (!globalName || !*globalName)
	{
		ALERT(at_console, "globalname must be non-empty string\n");
		return;
	}
	const char* stateStr = CMD_ARGV(2);

	GLOBALESTATE state;
	if (stricmp(stateStr, "off") == 0)
	{
		state = GLOBAL_OFF;
	}
	else if (stricmp(stateStr, "on") == 0)
	{
		state = GLOBAL_ON;
	}
	else if (stricmp(stateStr, "dead") == 0)
	{
		state = GLOBAL_DEAD;
	}
	else
	{
		ALERT(at_console, "Unknown state '%s'. Available states: off, on, dead\n", stateStr);
		return;
	}

	const globalentity_t *pGlobal = gGlobalState.EntityFromTable(globalName);
	if (pGlobal)
	{
		gGlobalState.EntitySetState(globalName, state);
	}
	else
	{
		ALERT(at_console, "Global '%s' was not found. Creating a new one with '%s' state\n", globalName, stateStr);
		gGlobalState.EntityAdd(globalName, gpGlobals->mapname, state);
	}
}

void Cmd_SetGlobalValue()
{
	if (!CanRunCheatCommand())
		return;
	if (CMD_ARGC() < 3)
	{
		ALERT(at_console, "Usage: %s <globalname> <number>\n", CMD_ARGV(0));
		return;
	}
	const char* globalName = CMD_ARGV(1);
	if (!globalName || !*globalName)
	{
		ALERT(at_console, "globalname must be non-empty string\n");
		return;
	}
	const int num = atoi(CMD_ARGV(2));

	const globalentity_t *pGlobal = gGlobalState.EntityFromTable(globalName);
	if (pGlobal)
	{
		gGlobalState.SetValue(globalName, num);
	}
	else
	{
		ALERT(at_console, "Global '%s' was not found. Creating a new one with 'off' state and value %d\n", globalName, num);
		gGlobalState.EntityAdd(globalName, gpGlobals->mapname, GLOBAL_OFF, num);
	}
}

void Cmd_CalcRatio()
{
	if (CMD_ARGC() < 2)
	{
		ALERT(at_console, "Usage: %s <targetname>\n", CMD_ARGV(0));
		return;
	}
	const char* target = CMD_ARGV(1);
	float r;
	if (TryCalcLocus_Ratio(NULL, target, r))
		ALERT(at_console, "%s calc_ratio is %g\n", target, r);
}

void Cmd_CalcPosition()
{
	if (CMD_ARGC() < 2)
	{
		ALERT(at_console, "Usage: %s <targetname>\n", CMD_ARGV(0));
		return;
	}
	const char* target = CMD_ARGV(1);
	Vector r;
	if (TryCalcLocus_Position(NULL, NULL, target, r))
		ALERT(at_console, "%s calc_position is (%g %g %g)\n", target, r.x, r.y, r.z);
}

void Cmd_CalcVelocity()
{
	if (CMD_ARGC() < 2)
	{
		ALERT(at_console, "Usage: %s <targetname>\n", CMD_ARGV(0));
		return;
	}
	const char* target = CMD_ARGV(1);
	Vector r;
	if (TryCalcLocus_Velocity(NULL, NULL, target, r))
		ALERT(at_console, "%s calc_velocity is (%g %g %g)\n", target, r.x, r.y, r.z);
}

void Cmd_CalcState()
{
	if (CMD_ARGC() < 2)
	{
		ALERT(at_console, "Usage: %s <targetname>\n", CMD_ARGV(0));
		return;
	}
	const char* target = CMD_ARGV(1);
	CBaseEntity* pEntity = UTIL_FindEntityByTargetname(NULL, target);
	if (pEntity)
	{
		const bool state = pEntity->IsTriggered(NULL);
		ALERT(at_console, "%s state is %s\n", target, state ? "On" : "Off");
	}
	else
	{
		ALERT(at_console, "Couldn't find %s\n", target);
	}
}

void ReportSoundScripts()
{
	int argc = CMD_ARGC();
	if (argc > 1)
	{
		for (int i=1; i<argc; ++i)
			g_SoundScriptSystem.DumpSoundScript(CMD_ARGV(i));
	}
	else
		g_SoundScriptSystem.DumpSoundScripts();
}

void ReportVisuals()
{
	int argc = CMD_ARGC();
	if (argc > 1)
	{
		for (int i=1; i<argc; ++i)
			g_VisualSystem.DumpVisual(CMD_ARGV(i));
	}
	else
		g_VisualSystem.DumpVisuals();
}

void ReportWarpballTemplates()
{
	g_WarpballCatalog.DumpWarpballTemplates();
}

void ReportMaterials()
{
	int argc = CMD_ARGC();
	if (argc > 1)
	{
		for (int i=1; i<argc; ++i)
		{
			const char* str = CMD_ARGV(i);
			if (*str)
				g_MaterialRegistry.DumpMaterial(*str);
		}
	}
	else
		g_MaterialRegistry.DumpMaterials();
}

void ForceScheduleFail()
{
	if (!CanRunCheatCommand())
		return;

	const int argc = CMD_ARGC();
	if (argc > 1)
	{
		auto forceFail = [](CBaseMonster* pMonster) {
			pMonster->m_failSchedule = SCHED_NONE;
			pMonster->TaskFail("forced fail by command");
		};

		const char* name = CMD_ARGV(1);
		CBaseEntity *pEntity = nullptr;
		while((pEntity = UTIL_FindEntityByTargetname(pEntity, name)) != nullptr)
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
				forceFail(pMonster);
		}
		while((pEntity = UTIL_FindEntityByClassname(pEntity, name)) != nullptr)
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
				forceFail(pMonster);
		}
	}
	else
	{
		ALERT(at_console, "Usage: %s <targetname or classname>\n", CMD_ARGV(0));
	}
}

static void CVAR_REGISTER_INTEGER( cvar_t* cvar, int value )
{
	char valueStr[12];
	sprintf(valueStr, "%d", value);
	cvar->string = valueStr;
	CVAR_REGISTER(cvar);
}

static void CVAR_REGISTER_BOOLEAN( cvar_t* cvar, bool value )
{
	const char* valueStr = value ? "1" : "0";
	cvar->string = valueStr;
	CVAR_REGISTER(cvar);
}

static void CVAR_REGISTER_FLOAT( cvar_t* cvar, float value )
{
	char valueStr[64];
	sprintf(valueStr, "%g", value);
	cvar->string = valueStr;
	CVAR_REGISTER(cvar);
}

cvar_t* violence_hblood = NULL;
cvar_t* violence_ablood = NULL;
cvar_t* violence_hgibs = NULL;
cvar_t* violence_agibs = NULL;

cvar_t sv_pushable_fixed_tick_fudge = { "sv_pushable_fixed_tick_fudge", "15" };
cvar_t sv_busters = { "sv_busters", "0" };

extern void RegisterAmmoTypes();
extern void ReportRegisteredAmmoTypes();

void ParseModConfigs()
{
	g_errorCollector.Clear();

	auto start = std::chrono::steady_clock::now();

	MaterialRegistry materialRegistry;
	materialRegistry.FillDefaults();
	materialRegistry.ReadFromFile("features/materials.json");
	g_MaterialRegistry = std::move(materialRegistry);

	WarpballTemplateCatalog warpballCatalog;
	warpballCatalog.ReadFromFile("templates/warpball.json");
	g_WarpballCatalog = std::move(warpballCatalog);

	SoundScriptSystem soundScriptSystem;
	soundScriptSystem.ReadFromFile("sound/soundscripts.json");
	g_SoundScriptSystem = std::move(soundScriptSystem);

	VisualSystem visualSystem;
	visualSystem.ReadFromFile("templates/visuals.json");
	g_VisualSystem = std::move(visualSystem);

	auto startEntities = std::chrono::steady_clock::now();
	EntTemplateSystem entTemplateSystem;
	entTemplateSystem.SetSoundScriptSystem(&g_SoundScriptSystem);
	entTemplateSystem.SetVisualSystem(&g_VisualSystem);
	const bool entitiesRead = entTemplateSystem.ReadFromFile("templates/entities.json");
	g_EntTemplateSystem = std::move(entTemplateSystem);
	auto finishEntities = std::chrono::steady_clock::now();

	InventorySpec inventorySpec;
	inventorySpec.SetEntTemplateSystem(&g_EntTemplateSystem);
	inventorySpec.ReadFromFile("templates/inventory.json");
	g_InventorySpec = std::move(inventorySpec);

	ObjectHintCatalog objectHintCatalog;
	objectHintCatalog.ReadFromFile("templates/objecthint.json");
	g_objectHintCatalog = std::move(objectHintCatalog);

	FollowersDescription followersDescription;
	followersDescription.ReadFromFile("features/followers.json");
	g_FollowersDescription = std::move(followersDescription);

	auto finish = std::chrono::steady_clock::now();
	unsigned int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count();

	ALERT(at_aiconsole, "Parsed mod configuration files in %u milliseconds\n", milliseconds);
	if (entitiesRead)
	{
		unsigned int millisecondsEntities = std::chrono::duration_cast<std::chrono::milliseconds>(finishEntities-startEntities).count();
		ALERT(at_aiconsole, "%u of them are spent on templates/entities.json\n", millisecondsEntities);
	}
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit( void )
{
	ReadServerFeatures();
	ReadEnabledMonsters();
	ReadEnabledWeapons();
	ReadMaxAmmos();
	ReadAmmoAmounts();

	RegisterAmmoTypes();

	ParseModConfigs();

	ReadSaveTitles();

	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );
	g_psv_maxspeed = CVAR_GET_POINTER( "sv_maxspeed" );
	g_psv_aim = CVAR_GET_POINTER( "sv_aim" );
	g_psv_allow_autoaim = CVAR_GET_POINTER( "sv_allow_autoaim" );
	g_footsteps = CVAR_GET_POINTER( "mp_footsteps" );

	g_psv_developer = CVAR_GET_POINTER( "developer" );

	g_enable_cheats = CVAR_GET_POINTER( "sv_cheats" );

	violence_hblood = CVAR_GET_POINTER( "violence_hblood" );
	violence_ablood = CVAR_GET_POINTER( "violence_ablood" );
	violence_hgibs = CVAR_GET_POINTER( "violence_hgibs" );
	violence_agibs = CVAR_GET_POINTER( "violence_agibs" );

	CVAR_REGISTER( &build_commit );
	CVAR_REGISTER( &build_branch );

	CVAR_REGISTER( &displaysoundlist );
	CVAR_REGISTER( &allow_spectators );
#if FEATURE_USE_THROUGH_WALLS_CVAR
	CVAR_REGISTER( &use_through_walls );
#endif
	CVAR_REGISTER( &items_physics_fix );
	CVAR_REGISTER( &npc_tridepth );
	CVAR_REGISTER( &npc_tridepth_all );
	CVAR_REGISTER( &npc_tridepth_vertical );
	CVAR_REGISTER( &npc_follow_nearest );
	CVAR_REGISTER( &npc_get_to_enemy_nearest );
	CVAR_REGISTER( &npc_forget_enemy_time );
	CVAR_REGISTER( &npc_trace_hull_attack_retry );
#if FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR
	CVAR_REGISTER( &npc_fix_melee_distance );
#endif
	CVAR_REGISTER( &npc_active_after_combat );
	CVAR_REGISTER( &npc_combat_fail_schedule );
	CVAR_REGISTER( &npc_lateral_retreat );
	CVAR_REGISTER( &npc_follow_out_of_pvs );
	CVAR_REGISTER( &npc_patrol );
	CVAR_REGISTER( &npc_vanilla_kick_behavior );

	CVAR_REGISTER( &teamplay );
	CVAR_REGISTER( &fraglimit );
	CVAR_REGISTER( &timelimit );

	CVAR_REGISTER( &fragsleft );
	CVAR_REGISTER( &timeleft );

	CVAR_REGISTER( &friendlyfire );
	CVAR_REGISTER( &falldamage );
	CVAR_REGISTER( &weaponstay );
	CVAR_REGISTER( &dropweapons );

	CVAR_REGISTER( &weapon_respawndelay );
	CVAR_REGISTER( &ammo_respawndelay );
	CVAR_REGISTER( &item_respawndelay );
	CVAR_REGISTER( &healthcharger_rechargetime );
	CVAR_REGISTER( &hevcharger_rechargetime );

	CVAR_REGISTER( &selfgauss );
	CVAR_REGISTER( &satchelfix );
	CVAR_REGISTER( &explosionfix );
	CVAR_REGISTER( &monsteryawspeedfix );
	CVAR_REGISTER( &animeventfix );
	CVAR_REGISTER( &anim_attack_reset_fix );
	CVAR_REGISTER( &anim_dispatch_fix );
	CVAR_REGISTER( &npc_run_task_instant );
	CVAR_REGISTER( &corpsephysics );
	CVAR_REGISTER( &pushablemode );
	CVAR_REGISTER( &forcerespawn );
	CVAR_REGISTER( &respawndelay );
	CVAR_REGISTER( &flashlight );
	CVAR_REGISTER( &aimcrosshair );
	CVAR_REGISTER( &decalfrequency );
	CVAR_REGISTER( &teamlist );
	CVAR_REGISTER( &teamoverride );
	CVAR_REGISTER( &defaultteam );

	CVAR_REGISTER( &allowmonsters );
	CVAR_REGISTER( &mp_allowmonsterinfo );
	CVAR_REGISTER( &sp_allowmonsterinfo );
	CVAR_REGISTER( &npc_dropweapons );
	CVAR_REGISTER( &dmgperscore );
	CVAR_REGISTER( &allydmgpenalty );
	CVAR_REGISTER( &npckill );

	CVAR_REGISTER( &sv_bunnyhop );

	CVAR_REGISTER( &mp_chattime );

	CVAR_REGISTER( &pickup_policy );

	CVAR_REGISTER( &sv_busters );

#if FEATURE_GRENADE_JUMP_CVAR
	CVAR_REGISTER( &grenade_jump );
#endif

	CVAR_REGISTER( &findnearestnodefix );
	CVAR_REGISTER( &nodegraph_distinfo_sort_fix );

	CVAR_REGISTER_BOOLEAN(&corpse_player_collision_fix, g_modFeatures.corpse_player_collision_fix);
	CVAR_REGISTER_BOOLEAN(&doors_open_in_move_direction, g_modFeatures.doors_open_in_move_direction);
	CVAR_REGISTER_BOOLEAN(&doors_blocked_recheck, g_modFeatures.doors_blocked_recheck);
	CVAR_REGISTER_BOOLEAN(&doors_blocked_fade_corpses, g_modFeatures.doors_blocked_fade_corpses);

	CVAR_REGISTER( &keepinventory );

// REGISTER CVARS FOR SKILL LEVEL STUFF
	// Agrunt
	REGISTER_SKILL_CVARS(sk_agrunt_health);
	REGISTER_SKILL_CVARS(sk_agrunt_dmg_punch);

	// Apache
	REGISTER_SKILL_CVARS( sk_apache_health );
	
	// Barnacle
	REGISTER_SKILL_CVARS(sk_barnacle_health);

#if FEATURE_BABYGARG
	// Baby Gargantua
	if (g_modFeatures.IsMonsterEnabled("babygarg"))
	{
		REGISTER_SKILL_CVARS(sk_babygargantua_health);
		REGISTER_SKILL_CVARS(sk_babygargantua_dmg_slash);
		REGISTER_SKILL_CVARS(sk_babygargantua_dmg_fire);
		REGISTER_SKILL_CVARS(sk_babygargantua_dmg_stomp);
	}
#endif

	// Barney
	REGISTER_SKILL_CVARS(sk_barney_health);

	// Bullsquid
	REGISTER_SKILL_CVARS(sk_bullsquid_health);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_bite);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_whip);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_spit);
	REGISTER_SKILL_CVARS(sk_bullsquid_toxicity);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_toxic_poison);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_toxic_impact);

	// Big Momma
	REGISTER_SKILL_CVARS(sk_bigmomma_health_factor);
	REGISTER_SKILL_CVARS(sk_bigmomma_dmg_slash);
	REGISTER_SKILL_CVARS(sk_bigmomma_dmg_blast);
	REGISTER_SKILL_CVARS(sk_bigmomma_radius_blast);

#if FEATURE_CLEANSUIT_SCIENTIST
	// Cleansuit Scientist
	if (g_modFeatures.IsMonsterEnabled("cleansuit_scientist"))
		REGISTER_SKILL_CVARS(sk_cleansuit_scientist_health);
#endif

#if FEATURE_FLYBEE
	// Flybee
	if (g_modFeatures.IsMonsterEnabled("flybee"))
	{
		REGISTER_SKILL_CVARS(sk_flybee_health);
		REGISTER_SKILL_CVARS(sk_flybee_dmg_kick);
		REGISTER_SKILL_CVARS(sk_flybee_dmg_beam);
		REGISTER_SKILL_CVARS(sk_flybee_dmg_flyball);
	}
#endif

	// Gargantua
	REGISTER_SKILL_CVARS(sk_gargantua_health);
	REGISTER_SKILL_CVARS(sk_gargantua_dmg_slash);
	REGISTER_SKILL_CVARS(sk_gargantua_dmg_fire);
	REGISTER_SKILL_CVARS(sk_gargantua_dmg_stomp);

	// Hassassin
	REGISTER_SKILL_CVARS(sk_hassassin_health);
	REGISTER_SKILL_CVARS(sk_hassassin_cloaking);

	// Headcrab
	REGISTER_SKILL_CVARS(sk_headcrab_health);
	REGISTER_SKILL_CVARS(sk_headcrab_dmg_bite);

#if FEATURE_OPFOR_GRUNT
	// Opposing Hgrunt
	if (g_modFeatures.IsMonsterEnabled("human_grunt_ally"))
	{
		REGISTER_SKILL_CVARS(sk_hgrunt_ally_health);
		REGISTER_SKILL_CVARS(sk_hgrunt_ally_kick);
		REGISTER_SKILL_CVARS(sk_hgrunt_ally_pellets);
		REGISTER_SKILL_CVARS(sk_hgrunt_ally_gspeed);
	}

	// Medic
	if (g_modFeatures.IsMonsterEnabled("human_grunt_medic"))
	{
		REGISTER_SKILL_CVARS(sk_medic_ally_health);
		REGISTER_SKILL_CVARS(sk_medic_ally_kick);
		REGISTER_SKILL_CVARS(sk_medic_ally_gspeed);
		REGISTER_SKILL_CVARS(sk_medic_ally_heal);
	}

	// Torch
	if (g_modFeatures.IsMonsterEnabled("human_grunt_torch"))
	{
		REGISTER_SKILL_CVARS(sk_torch_ally_health);
		REGISTER_SKILL_CVARS(sk_torch_ally_kick);
		REGISTER_SKILL_CVARS(sk_torch_ally_gspeed);
	}
#endif

	// Hgrunt
	REGISTER_SKILL_CVARS(sk_hgrunt_health);
	REGISTER_SKILL_CVARS(sk_hgrunt_kick);
	REGISTER_SKILL_CVARS(sk_hgrunt_pellets);
	REGISTER_SKILL_CVARS(sk_hgrunt_gspeed);

#if FEATURE_HWGRUNT
	// HWgrunt
	if (g_modFeatures.IsMonsterEnabled("hwgrunt"))
		REGISTER_SKILL_CVARS(sk_hwgrunt_health);
#endif

	// Houndeye
	REGISTER_SKILL_CVARS(sk_houndeye_health);
	REGISTER_SKILL_CVARS(sk_houndeye_dmg_blast);

	// ISlave
	REGISTER_SKILL_CVARS(sk_islave_health);
	REGISTER_SKILL_CVARS(sk_islave_dmg_claw);
	REGISTER_SKILL_CVARS(sk_islave_dmg_clawrake);
	REGISTER_SKILL_CVARS(sk_islave_dmg_zap);
	REGISTER_SKILL_CVARS(sk_islave_zap_rate);
	REGISTER_SKILL_CVARS(sk_islave_revival);

	// Icthyosaur
	REGISTER_SKILL_CVARS(sk_ichthyosaur_health);
	REGISTER_SKILL_CVARS(sk_ichthyosaur_shake);

	// Leech
	REGISTER_SKILL_CVARS(sk_leech_health);
	REGISTER_SKILL_CVARS(sk_leech_dmg_bite);

	// Controller
	REGISTER_SKILL_CVARS(sk_controller_health);
	REGISTER_SKILL_CVARS(sk_controller_dmgzap);
	REGISTER_SKILL_CVARS(sk_controller_speedball);
	REGISTER_SKILL_CVARS(sk_controller_dmgball);

#if FEATURE_MASSN
	// Massassin
	if (g_modFeatures.IsMonsterEnabled("male_assassin"))
	{
		REGISTER_SKILL_CVARS(sk_massassin_health);
		REGISTER_SKILL_CVARS(sk_massassin_kick);
		REGISTER_SKILL_CVARS(sk_massassin_gspeed);
	}
#endif

	// Nihilanth
	REGISTER_SKILL_CVARS(sk_nihilanth_health);
	REGISTER_SKILL_CVARS(sk_nihilanth_zap);

	// Osprey
	REGISTER_SKILL_CVARS(sk_osprey);

#if FEATURE_BLACK_OSPREY
	// Blackops Osprey
	if (g_modFeatures.IsMonsterEnabled("blkop_osprey"))
		REGISTER_SKILL_CVARS(sk_blkopsosprey);
#endif

#if FEATURE_OTIS
	// Otis
	if (g_modFeatures.IsMonsterEnabled("otis"))
		REGISTER_SKILL_CVARS(sk_otis_health);
#endif

#if FEATURE_KATE
	// Kate
	if (g_modFeatures.IsMonsterEnabled("kate"))
		REGISTER_SKILL_CVARS(sk_kate_health);
#endif

#if FEATURE_PANTHEREYE
	// Panthereye
	if (g_modFeatures.IsMonsterEnabled("panthereye"))
	{
		REGISTER_SKILL_CVARS(sk_panthereye_health);
		REGISTER_SKILL_CVARS(sk_panthereye_dmg_claw);
	}
#endif

#if FEATURE_PITDRONE
	// Pitdrone
	if (g_modFeatures.IsMonsterEnabled("pitdrone"))
	{
		REGISTER_SKILL_CVARS(sk_pitdrone_health);
		REGISTER_SKILL_CVARS(sk_pitdrone_dmg_bite);
		REGISTER_SKILL_CVARS(sk_pitdrone_dmg_whip);
		REGISTER_SKILL_CVARS(sk_pitdrone_dmg_spit);
	}
#endif

#if FEATURE_PITWORM
	// Pitworm
	if (g_modFeatures.IsMonsterEnabled("pitworm"))
	{
		REGISTER_SKILL_CVARS(sk_pitworm_health);
		REGISTER_SKILL_CVARS(sk_pitworm_dmg_swipe);
		REGISTER_SKILL_CVARS(sk_pitworm_dmg_beam);
	}
#endif

#if FEATURE_GENEWORM
	// Geneworm
	if (g_modFeatures.IsMonsterEnabled("geneworm"))
	{
		REGISTER_SKILL_CVARS(sk_geneworm_health);
		REGISTER_SKILL_CVARS(sk_geneworm_dmg_spit);
		REGISTER_SKILL_CVARS(sk_geneworm_dmg_hit);
	}
#endif

#if FEATURE_ROBOGRUNT
	if (g_modFeatures.IsMonsterEnabled("robogrunt"))
		REGISTER_SKILL_CVARS(sk_rgrunt_explode);
#endif


#if FEATURE_SHOCKTROOPER
	// ShockTrooper
	if (g_modFeatures.IsMonsterEnabled("shocktrooper"))
	{
		REGISTER_SKILL_CVARS(sk_shocktrooper_health);
		REGISTER_SKILL_CVARS(sk_shocktrooper_kick);
		REGISTER_SKILL_CVARS(sk_shocktrooper_gspeed);
		REGISTER_SKILL_CVARS(sk_shocktrooper_maxcharge);
		REGISTER_SKILL_CVARS(sk_shocktrooper_rchgspeed);
	}

	// Shock Roach
	if (g_modFeatures.IsMonsterEnabled("shockroach"))
	{
		REGISTER_SKILL_CVARS(sk_shockroach_health);
		REGISTER_SKILL_CVARS(sk_shockroach_dmg_bite);
		REGISTER_SKILL_CVARS(sk_shockroach_lifespan);
	}
#endif

	// Scientist
	REGISTER_SKILL_CVARS(sk_scientist_health);

	// Snark
	REGISTER_SKILL_CVARS(sk_snark_health);
	REGISTER_SKILL_CVARS(sk_snark_dmg_bite);
	REGISTER_SKILL_CVARS(sk_snark_dmg_pop);

#if FEATURE_VOLTIFORE
	// Voltigore
	if (g_modFeatures.IsMonsterEnabled("voltigore"))
	{
		REGISTER_SKILL_CVARS(sk_voltigore_health);
		REGISTER_SKILL_CVARS(sk_voltigore_dmg_punch);
		REGISTER_SKILL_CVARS(sk_voltigore_dmg_beam);
		REGISTER_SKILL_CVARS(sk_voltigore_dmg_explode);
	}

	// Baby Voltigore
	if (g_modFeatures.IsMonsterEnabled("babyvoltigore"))
	{
		REGISTER_SKILL_CVARS(sk_babyvoltigore_health);
		REGISTER_SKILL_CVARS(sk_babyvoltigore_dmg_punch);
	}
#endif

	// Zombie
	REGISTER_SKILL_CVARS(sk_zombie_health);
	REGISTER_SKILL_CVARS(sk_zombie_dmg_one_slash);
	REGISTER_SKILL_CVARS(sk_zombie_dmg_both_slash);

#if FEATURE_ZOMBIE_BARNEY
	// Zombie Barney
	if (g_modFeatures.IsMonsterEnabled("zombie_barney"))
	{
		REGISTER_SKILL_CVARS(sk_zombie_barney_health);
		REGISTER_SKILL_CVARS(sk_zombie_barney_dmg_one_slash);
		REGISTER_SKILL_CVARS(sk_zombie_barney_dmg_both_slash);
	}
#endif

#if FEATURE_ZOMBIE_SOLDIER
	// Zombie Soldier
	if (g_modFeatures.IsMonsterEnabled("zombie_soldier"))
	{
		REGISTER_SKILL_CVARS(sk_zombie_soldier_health);
		REGISTER_SKILL_CVARS(sk_zombie_soldier_dmg_one_slash);
		REGISTER_SKILL_CVARS(sk_zombie_soldier_dmg_both_slash);
	}
#endif

#if FEATURE_GONOME
	// Gonome
	if (g_modFeatures.IsMonsterEnabled("gonome"))
	{
		REGISTER_SKILL_CVARS(sk_gonome_health);
		REGISTER_SKILL_CVARS(sk_gonome_dmg_one_slash);
		REGISTER_SKILL_CVARS(sk_gonome_dmg_guts);
		REGISTER_SKILL_CVARS(sk_gonome_dmg_one_bite);
	}
#endif

#if FEATURE_FLOATER
	if (g_modFeatures.IsMonsterEnabled("floater"))
	{
		REGISTER_SKILL_CVARS(sk_floater_health);
		REGISTER_SKILL_CVARS(sk_floater_explode);
	}
#endif

	//Turret
	REGISTER_SKILL_CVARS(sk_turret_health);

	// MiniTurret
	REGISTER_SKILL_CVARS(sk_miniturret_health);

	// Sentry Turret
	REGISTER_SKILL_CVARS(sk_sentry_health);

#if FEATURE_ROBOCOP
	// Robocop
	if (g_modFeatures.IsMonsterEnabled("robocop"))
	{
		REGISTER_SKILL_CVARS(sk_robocop_health);
		REGISTER_SKILL_CVARS(sk_robocop_dmg_mortar);
		REGISTER_SKILL_CVARS(sk_robocop_dmg_fist);
		REGISTER_SKILL_CVARS(sk_robocop_sw_radius);
	}
#endif

	// Tor
	if (g_modFeatures.IsMonsterEnabled("tor"))
	{
		REGISTER_SKILL_CVARS(sk_tor_health);
		REGISTER_SKILL_CVARS(sk_tor_punch);
		REGISTER_SKILL_CVARS(sk_tor_energybeam);
		REGISTER_SKILL_CVARS(sk_tor_sonicblast);
	}

	REGISTER_SKILL_CVARS(sk_zaptrap_sense_radius);
	REGISTER_SKILL_CVARS(sk_zaptrap_respawn_time);

	// PLAYER WEAPONS

	// Crowbar whack
	REGISTER_SKILL_CVARS(sk_plr_crowbar);

	// Glock Round
	REGISTER_SKILL_CVARS(sk_plr_9mm_bullet);

	// 357 Round
	REGISTER_SKILL_CVARS(sk_plr_357_bullet);

	// MP5 Round
	REGISTER_SKILL_CVARS(sk_plr_9mmAR_bullet);

	// M203 grenade
	REGISTER_SKILL_CVARS(sk_plr_9mmAR_grenade);

	// Shotgun buckshot
	REGISTER_SKILL_CVARS(sk_buckshot);
	REGISTER_SKILL_CVARS(sk_plr_buckshot);

	// Crossbow
	REGISTER_SKILL_CVARS(sk_plr_xbow_bolt_monster);
	REGISTER_SKILL_CVARS(sk_plr_xbow_bolt_client);

	// RPG
	REGISTER_SKILL_CVARS(sk_plr_rpg);

	// Tau Cannon
	REGISTER_SKILL_CVARS(sk_plr_gauss);

	// Gluon Gun
	REGISTER_SKILL_CVARS(sk_plr_egon_narrow);
	REGISTER_SKILL_CVARS(sk_plr_egon_wide);

	// Hand Grendade
	REGISTER_SKILL_CVARS(sk_plr_hand_grenade);

	// Satchel Charge
	REGISTER_SKILL_CVARS(sk_plr_satchel);

	// Tripmine
	REGISTER_SKILL_CVARS(sk_plr_tripmine);

#if FEATURE_MEDKIT
	// Medkit
	if (g_modFeatures.IsWeaponEnabled(WEAPON_MEDKIT))
	{
		REGISTER_SKILL_CVARS(sk_plr_medkitshot);
		REGISTER_SKILL_CVARS(sk_plr_medkittime);
	}
#endif

#if FEATURE_DESERT_EAGLE
	// Desert Eagle
	if (g_modFeatures.IsWeaponEnabled(WEAPON_EAGLE))
		REGISTER_SKILL_CVARS(sk_plr_eagle);
#endif

#if FEATURE_PIPEWRENCH
	// Pipe wrench
	if (g_modFeatures.IsWeaponEnabled(WEAPON_PIPEWRENCH))
		REGISTER_SKILL_CVARS(sk_plr_pipewrench);
#endif

#if FEATURE_KNIFE
	// Knife
	if (g_modFeatures.IsWeaponEnabled(WEAPON_KNIFE))
		REGISTER_SKILL_CVARS(sk_plr_knife);
#endif

#if FEATURE_GRAPPLE
	// Grapple
	if (g_modFeatures.IsWeaponEnabled(WEAPON_GRAPPLE))
		REGISTER_SKILL_CVARS(sk_plr_grapple);
#endif

#if FEATURE_M249
	// M249
	if (g_modFeatures.IsWeaponEnabled(WEAPON_M249))
		REGISTER_SKILL_CVARS(sk_plr_556_bullet);
#endif

#if FEATURE_SNIPERRIFLE
	// Sniper rifle
	if (g_modFeatures.IsWeaponEnabled(WEAPON_SNIPERRIFLE))
		REGISTER_SKILL_CVARS(sk_plr_762_bullet);
#endif

	// Shock rifle
#if FEATURE_SHOCKBEAM
	if (g_modFeatures.ShockBeamEnabled())
	{
		REGISTER_SKILL_CVARS(sk_plr_shockroachs);
		REGISTER_SKILL_CVARS(sk_plr_shockroachm);
		REGISTER_SKILL_CVARS(sk_shockroach);
	}
#endif

#if FEATURE_SPOREGRENADE
	if (g_modFeatures.SporesEnabled())
		REGISTER_SKILL_CVARS(sk_plr_spore);
#endif

#if FEATURE_DISPLACER
	if (g_modFeatures.DisplacerBallEnabled())
	{
		REGISTER_SKILL_CVARS(sk_plr_displacer_other);
		REGISTER_SKILL_CVARS(sk_plr_displacer_radius);
	}
#endif

#if FEATURE_UZI
	if (g_modFeatures.IsWeaponEnabled(WEAPON_UZI))
		REGISTER_SKILL_CVARS(sk_plr_uzi);
#endif

	// WORLD WEAPONS
	REGISTER_SKILL_CVARS(sk_12mm_bullet);
	REGISTER_SKILL_CVARS(sk_9mmAR_bullet);
	REGISTER_SKILL_CVARS(sk_9mm_bullet);
	REGISTER_SKILL_CVARS(sk_357_bullet);
	REGISTER_SKILL_CVARS(sk_556_bullet);
	REGISTER_SKILL_CVARS(sk_762_bullet);

	// HORNET
	REGISTER_SKILL_CVARS(sk_hornet_dmg);
	REGISTER_SKILL_CVARS(sk_plr_hornet_dmg);

	// MORTAR
	REGISTER_SKILL_CVARS(sk_mortar);

	// HEALTH/SUIT CHARGE DISTRIBUTION
	REGISTER_SKILL_CVARS(sk_suitcharger);
	REGISTER_SKILL_CVARS(sk_battery);
	REGISTER_SKILL_CVARS(sk_healthcharger);
	REGISTER_SKILL_CVARS(sk_healthkit);
	REGISTER_SKILL_CVARS(sk_scientist_heal);
	REGISTER_SKILL_CVARS(sk_scientist_heal_time);
	REGISTER_SKILL_CVARS(sk_soda);
	REGISTER_SKILL_CVARS(sk_vortigaunt_armor_charge);

	// monster damage adjusters
	REGISTER_SKILL_CVARS(sk_monster_head);
	REGISTER_SKILL_CVARS(sk_monster_chest);
	REGISTER_SKILL_CVARS(sk_monster_stomach);
	REGISTER_SKILL_CVARS(sk_monster_arm);
	REGISTER_SKILL_CVARS(sk_monster_leg);

	// player damage adjusters
	REGISTER_SKILL_CVARS(sk_player_head);
	REGISTER_SKILL_CVARS(sk_player_chest);
	REGISTER_SKILL_CVARS(sk_player_stomach);
	REGISTER_SKILL_CVARS(sk_player_arm);
	REGISTER_SKILL_CVARS(sk_player_leg);

	// Flashlight
	REGISTER_SKILL_CVARS(sk_flashlight_drain_time);
	REGISTER_SKILL_CVARS(sk_flashlight_charge_time);

	// Player armor
	REGISTER_SKILL_CVARS(sk_plr_armor_strength);
// END REGISTER CVARS FOR SKILL LEVEL STUFF

	CVAR_REGISTER( &sv_pushable_fixed_tick_fudge );

	SERVER_COMMAND( "exec skill.cfg\n" );

	if (g_modFeatures.skill_opfor)
		SERVER_COMMAND( "exec skillopfor.cfg\n" );

	const char* fileName = "features/featureful_exec.cfg";
	int fileSize;
	byte *pExecFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pExecFile)
	{
		pExecFile = g_engfuncs.pfnLoadFileForMe( "featureful_exec.cfg", &fileSize );
	}

	if (pExecFile)
	{
		SERVER_COMMAND((const char*)pExecFile);
		g_engfuncs.pfnFreeFile( pExecFile );
	}

	// Register server commands
	g_engfuncs.pfnAddServerCommand("report_ai_state", Cmd_ReportAIState);
	g_engfuncs.pfnAddServerCommand("watch_ai_schedules", Cmd_AddScheduleWatcher);
	g_engfuncs.pfnAddServerCommand("entities_count", Cmd_NumberOfEntities);
	g_engfuncs.pfnAddServerCommand("set_global_state", Cmd_SetGlobalState);
	g_engfuncs.pfnAddServerCommand("set_global_value", Cmd_SetGlobalValue);
	g_engfuncs.pfnAddServerCommand("calc_ratio", Cmd_CalcRatio);
	g_engfuncs.pfnAddServerCommand("calc_position", Cmd_CalcPosition);
	g_engfuncs.pfnAddServerCommand("calc_velocity", Cmd_CalcVelocity);
	g_engfuncs.pfnAddServerCommand("calc_state", Cmd_CalcState);
	g_engfuncs.pfnAddServerCommand("dump_ammo_types", ReportRegisteredAmmoTypes);
	g_engfuncs.pfnAddServerCommand("dump_warpballs", ReportWarpballTemplates);
	g_engfuncs.pfnAddServerCommand("dump_precached_models", ReportPrecachedModels);
	g_engfuncs.pfnAddServerCommand("dump_precached_sounds", ReportPrecachedSounds);
	g_engfuncs.pfnAddServerCommand("dump_soundscripts", ReportSoundScripts);
	g_engfuncs.pfnAddServerCommand("dump_visuals", ReportVisuals);
	g_engfuncs.pfnAddServerCommand("dump_materials", ReportMaterials);
	g_engfuncs.pfnAddServerCommand("force_schedule_fail", ForceScheduleFail);
}

bool ItemsPickableByTouch()
{
	return pickup_policy.value != 1;
}

bool ItemsPickableByUse()
{
	return pickup_policy.value >= 1;
}

int ItemsPhysicsFix()
{
	return static_cast<int>(items_physics_fix.value);
}

bool IsDeveloperModeOn()
{
	return g_psv_developer && g_psv_developer->value > 0;
}

int DeveloperModeLevel()
{
	if (g_psv_developer)
		return (int)g_psv_developer->value;
	return 0;
}

bool CheatsEnabled()
{
	return g_enable_cheats && g_enable_cheats->value != 0;
}
