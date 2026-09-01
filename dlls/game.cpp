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
#include "skill.h"
#include "skilldata.h"
#include "ammo_amounts.h"
#include "inventory.h"
#include "soundscripts.h"
#include "visuals.h"
#include "warpball.h"
#include "ent_templates.h"
#include "player_templates.h"
#include "followers.h"
#include "time_based_damage.h"
#include "savetitles.h"
#include "objecthint_spec.h"
#include "vcs_info.h"
#include "tex_materials.h"
#include "error_collector.h"
#include "weapons.h"
#include "weapon_templates.h"
#include "weapon_carry_categories.h"
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
cvar_t handle_tiny_creatures = {"handle_tiny_creatures", "0"};

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

	bigmomma_wait_fix = false;
	bigmomma_lastnode_fix = false;

	doors_open_in_move_direction = false;
	doors_blocked_recheck = false;
	doors_blocked_fade_corpses = false;
	door_rotating_starts_open_fix = false;

	env_spark_transit = false;

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
		KEY_VALUE_DEF(bigmomma_wait_fix),
		KEY_VALUE_DEF(bigmomma_lastnode_fix),
		KEY_VALUE_DEF(doors_open_in_move_direction),
		KEY_VALUE_DEF(doors_blocked_recheck),
		KEY_VALUE_DEF(doors_blocked_fade_corpses),
		KEY_VALUE_DEF(door_rotating_starts_open_fix),
		KEY_VALUE_DEF(env_spark_transit),
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
		WeaponNameAndId("minigun", WEAPON_MINIGUN),
		WeaponNameAndId("penguin", WEAPON_PENGUIN),
		WeaponNameAndId("uzi", WEAPON_UZI),
		WeaponNameAndId("nailgun", WEAPON_NAILGUN),
		WeaponNameAndId("grenadelauncher", WEAPON_GRENADE_LAUNCHER),
		WeaponNameAndId("melee", WEAPON_MELEE),
		WeaponNameAndId("pistol", WEAPON_PISTOL),
		WeaponNameAndId("pistol2", WEAPON_PISTOL2),
		WeaponNameAndId("smg", WEAPON_SMG),
		WeaponNameAndId("smg2", WEAPON_SMG2),
		WeaponNameAndId("rifle", WEAPON_RIFLE),
		WeaponNameAndId("rifle2", WEAPON_RIFLE2),
		WeaponNameAndId("shotgun2", WEAPON_SHOTGUN2),
		WeaponNameAndId("sniperrifle2", WEAPON_SNIPERRIFLE2),
		WeaponNameAndId("throwable", WEAPON_THROWABLE),
		WeaponNameAndId("camera", WEAPON_CAMERA),
		WeaponNameAndId("radio", WEAPON_RADIO),
		WeaponNameAndId("blowtorch", WEAPON_BTORCH),
		WeaponNameAndId("tool", WEAPON_TOOL),
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
	if (IsWeaponEnabled(WEAPON_EAGLE))
		return "weapon_eagle";
	return "ammo_357";
}

const char* ModFeatures::M249DropName() const
{
	if (IsWeaponEnabled(WEAPON_M249))
		return "weapon_m249";
	return "ammo_9mmAR";
}

const char* ModFeatures::DeadHazModel() const
{
	if (opfor_deadhaz)
		return "models/deadhaz.mdl";
	else
		return "models/player.mdl";
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

bool ModFeatures::ShouldIgnoreTinyCreatures(int policy) const
{
	if (policy == HANDLE_TINY_CREATURES_DEFAULT)
		return static_cast<int>(::handle_tiny_creatures.value) == HANDLE_TINY_CREATURES_DONTCOLLIDE;
	return policy == HANDLE_TINY_CREATURES_DONTCOLLIDE;
}

bool ModFeatures::ShouldCrushTinyCreatures(int policy) const
{
	if (policy == HANDLE_TINY_CREATURES_DEFAULT)
		return static_cast<int>(::handle_tiny_creatures.value) == HANDLE_TINY_CREATURES_CRUSH;
	return policy == HANDLE_TINY_CREATURES_CRUSH;
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
	SkipSpacesAndTabs(buffer, i, length);

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
	SkipSpacesAndTabs(buffer, i, length);

	if (IsNonSignificantLine(buffer + i))
		return;

	const int keyStart = i;
	ConsumeNonSpaceCharacters(buffer, i, length);
	const int keyLength = i - keyStart;
	SkipSpacesAndTabs(buffer, i, length);
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

void ReadEnabledWeapons()
{
	const char* fileName = "features/featureful_weapons.cfg";
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe(fileName, &fileSize);
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
	const char* fileName = "features/featureful_monsters.cfg";
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe(fileName, &fileSize);
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
	const char* fileName = "features/featureful_server.cfg";
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe(fileName, &fileSize);
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
				ALERT(at_warning, "%s: key '%s' without a value!\n", fileName, key);
			}
		}
	}
}

void ReadWeaponCarryCategories()
{
	g_WeaponCarryCategories.Reset();

	const char* fileName = "features/weapon_carry_categories.cfg";
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing weapon carry categories from %s\n", fileName);

	char buffer[512];
	memset(buffer, 0, sizeof(buffer));

	while (memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ))
	{
		char* key = nullptr;
		char* value = nullptr;
		TryConsumeKeyAndValue(buffer, sizeof(buffer), key, value);

		if (key)
		{
			if (value)
			{
				const int category = atoi(value);
				const int weaponId = GetWeaponIdByName(key);
				if (weaponId < 0)
					continue;

				g_WeaponCarryCategories.SetWeaponCategory(weaponId, category);
			}
			else
			{
				ALERT(at_warning, "%s: key '%s' without a value!\n", fileName, key);
			}
		}
	}
}

static cvar_t build_commit = { "sv_game_build_commit", g_VCSInfo_Commit };
static cvar_t build_commit_date = { "sv_game_build_commit_date", g_VCSInfo_CommitDate };
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
cvar_t tripminefix	= { "tripminefix", "1", FCVAR_SERVER };
cvar_t explosionfix	= { "explosionfix", "1", FCVAR_SERVER };
cvar_t monsteryawspeedfix	= { "monsteryawspeedfix", "1", FCVAR_SERVER };
cvar_t animeventfix = {"animeventfix", "1", FCVAR_SERVER };
cvar_t animevent_floorframe = {"animevent_floorframe", "1", FCVAR_SERVER };
cvar_t anim_attack_reset_fix = {"anim_attack_reset_fix", "1", FCVAR_SERVER };
cvar_t anim_dispatch_fix = {"anim_dispatch_fix", "0", FCVAR_SERVER};
cvar_t npc_run_task_instant = {"npc_run_task_instant", "1", FCVAR_SERVER};
cvar_t npc_range_attack_unlooped = {"npc_range_attack_unlooped", "1", FCVAR_SERVER};
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
cvar_t mp_allowdropammo = { "mp_allowdropammo","1", FCVAR_SERVER };
cvar_t sp_allowdropammo = { "sp_allowdropammo","0", FCVAR_SERVER };
cvar_t npc_dropweapons = { "npc_dropweapons", "1", FCVAR_SERVER };
cvar_t dmgperscore = { "mp_dmgperscore", "0", FCVAR_SERVER };
cvar_t allydmgpenalty = { "mp_allydmgpenalty", "2", FCVAR_SERVER };
cvar_t npckill = { "mp_npckill", "1", FCVAR_SERVER };
cvar_t mp_playtexturesounds = { "mp_playtexturesounds", "1", FCVAR_SERVER };

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
cvar_t npc_lateral_retreat = { "npc_lateral_retreat", "1", FCVAR_SERVER };
cvar_t npc_follow_out_of_pvs = { "npc_follow_out_of_pvs", "1", FCVAR_SERVER };
cvar_t npc_patrol = { "npc_patrol", "1", FCVAR_SERVER };
cvar_t npc_vanilla_kick_behavior = { "npc_vanilla_kick_behavior", "0", FCVAR_SERVER };
cvar_t npc_report_fire_animevents = { "npc_report_fire_animevents", "0", FCVAR_SERVER };
cvar_t npc_idlesound_requires_pvs = { "npc_idlesound_requires_pvs", "0", FCVAR_SERVER };

cvar_t mp_chattime	= { "mp_chattime","10", FCVAR_SERVER };

cvar_t pickup_policy = { "pickup_policy","0", FCVAR_SERVER };

cvar_t grenade_jump = { "grenade_jump","1", FCVAR_SERVER };

cvar_t findnearestnodefix = { "findnearestnodefix", "1", FCVAR_SERVER };
cvar_t nodegraph_distinfo_sort_fix = {"nodegraph_distinfo_sort_fix", "1", FCVAR_SERVER};

cvar_t keepinventory	= { "mp_keepinventory","0", FCVAR_SERVER }; // keep inventory across level transitions in multiplayer coop

// Engine Cvars
cvar_t *g_psv_gravity = NULL;
cvar_t *g_psv_maxspeed = NULL;
cvar_t *g_psv_aim = NULL;
cvar_t *g_psv_allow_autoaim = NULL;
cvar_t *g_footsteps = NULL;
cvar_t *g_enable_cheats = NULL;

cvar_t *g_psv_developer = NULL;

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
				ALERT(at_console, "Adding the monster \"%s\" (%d)\n", FStringNull(pMonster->pev->targetname) ? "" : STRING(pMonster->pev->targetname), pMonster->entindex());
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

void ListEntityTemplates()
{
	for (auto it = g_EntTemplateSystem.EntityTemplatesBegin(); it != g_EntTemplateSystem.EntityTemplatesEnd(); ++it)
	{
		ALERT(at_console, "%s\n", it->first.c_str());
	}
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

static void PrintFloatRange(const char* prefix, const FloatRange& range)
{
	if (range.max > range.min)
	{
		ALERT(at_console, "%s: [%g, %g]\n", prefix, range.min, range.max);
	}
	else
	{
		ALERT(at_console, "%s: %g\n", prefix, range.min);
	}
}

void PrintSkillVariable()
{
	const int argc = CMD_ARGC();
	if (argc <= 1)
	{
		ALERT(at_console, "Usage: %s <skill variable name>\n", CMD_ARGV(0));
		return;
	}

	for (int i=1; i<argc; ++i)
	{
		const char* name = CMD_ARGV(i);
		const SkillVariable* variable = g_SkillData.GetSkillVariable(name);
		if (variable)
		{
			if (argc > 2)
			{
				ALERT(at_console, "%s:\n", name);
			}
			if (variable->HasValue(SkillVariable::COMMON))
			{
				PrintFloatRange("Common", variable->GetValue(SkillVariable::COMMON));
			}
			if (variable->HasValue(SkillVariable::EASY))
			{
				PrintFloatRange("Easy", variable->GetValue(SkillVariable::EASY));
			}
			if (variable->HasValue(SkillVariable::MEDIUM))
			{
				PrintFloatRange("Normal", variable->GetValue(SkillVariable::MEDIUM));
			}
			if (variable->HasValue(SkillVariable::HARD))
			{
				PrintFloatRange("Hard", variable->GetValue(SkillVariable::HARD));
			}
			const char* fallback = variable->Fallback();
			if (fallback)
			{
				ALERT(at_console, "Fallback: %s\n", fallback);
			}
			optional<float> multiplier = variable->FallbackMultiplier();
			if (multiplier.has_value())
			{
				ALERT(at_console, "Fallback multiplier: %g\n", *multiplier);
			}
			PrintFloatRange("Current value", ::GetSkillValueRange(name));
		}
		else
		{
			ALERT(at_console, "No skill variable '%s' found\n", name);
		}
	}
}

void PrintSkillReplacementForEntTemplate()
{
	const int argc = CMD_ARGC();
	if (argc <= 2)
	{
		ALERT(at_console, "Usage: %s <entity template name> <skill variable name>\n", CMD_ARGV(0));
		return;
	}

	const char* entTemplateName = CMD_ARGV(1);
	const EntTemplate* entTemplate = g_EntTemplateSystem.GetTemplate(entTemplateName);

	if (!entTemplate)
	{
		ALERT(at_console, "No entity template with such name: %s\n", entTemplateName);
		return;
	}

	for (int i=2; i<argc; ++i)
	{
		const char* name = CMD_ARGV(i);

		const SkillReplacement* replacement = entTemplate->GetSkillReplacement(name);
		if (replacement)
		{
			switch(replacement->type)
			{
			case SkillReplacement::STRING:
				ALERT(at_console, "Replacement variable is %s\n", replacement->replacement.c_str());
				break;
			case SkillReplacement::COMMON:
				PrintFloatRange("Replacement value", replacement->medium);
				break;
			case SkillReplacement::DIFFICULTIES:
				ALERT(at_console, "Replacement values are:\n");
				PrintFloatRange("On Easy:", replacement->easy);
				PrintFloatRange("On Medium:", replacement->medium);
				PrintFloatRange("On Hard:", replacement->hard);
				break;
			case SkillReplacement::MULTIPLIER:
				ALERT(at_console, "Replacement multiplier is %g\n", replacement->medium.min);
				break;
			default:
				break;
			}
		}

		PrintFloatRange("Evaluated value", ::GetSkillValueRange(name, entTemplate, entTemplateName));
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

void ProvideSkillFallbacks()
{
	g_SkillData.ProvideFallback("agrunt_head", 1.5f);

	g_SkillData.ProvideFallback("apache_dmg_blast", 300.0f);
	g_SkillData.ProvideFallback("apache_rockets_and_gun", 0.0f, 1.0f, 1.0f);
	g_SkillData.ProvideFallback("apache_rocket_reload_time", 10.0f);
	g_SkillData.ProvideFallback("apache_rocket_delay", 0.5f);

	g_SkillData.ProvideFallback("archer_delay_zap", FloatRange(2.5f, 7.0f));

	g_SkillData.ProvideFallbackWithFactor("babycrab_health", "headcrab_health", 0.25f);
	g_SkillData.ProvideFallbackWithFactor("babycrab_dmg_bite", "headcrab_dmg_bite", 0.3f);

	g_SkillData.ProvideFallback("barnacle_health", 25.0f);

	g_SkillData.ProvideFallback("bigmomma_health_factor", 1.0f);
	g_SkillData.ProvideFallback("bigmomma_dmg_slash", 50.0f);
	g_SkillData.ProvideFallback("bigmomma_dmg_blast", 100.0f);
	g_SkillData.ProvideFallback("bigmomma_radius_blast", 250.0f);

	g_SkillData.ProvideFallback("bullsquid_toxicity", 0.0f, 1.0f, 1.0f);
	g_SkillData.ProvideFallbackWithFactor("bullsquid_dmg_toxic_poison", "bullsquid_dmg_spit", 0.25f);
	g_SkillData.ProvideFallbackWithFactor("bullsquid_dmg_toxic_impact", "bullsquid_dmg_spit", 1.5f);
	g_SkillData.ProvideFallback("bullsquid_spit_inaccuracy", 5.0f, 3.0f, 1.0f);

	g_SkillData.ProvideFallback("civilian_health", "scientist_health");
	g_SkillData.ProvideFallback("cleansuit_scientist_health", "scientist_health");

	g_SkillData.ProvideFallback("floater_basespeed", 100.0f);
	g_SkillData.ProvideFallback("floater_extraspeed", 400.0f);
	g_SkillData.ProvideFallback("floater_bloat_time", 2.1f);
	g_SkillData.ProvideFallback("floater_bloat_distance", 128.0f);

	g_SkillData.ProvideFallback("flybee_health", "ichthyosaur_health");
	g_SkillData.ProvideFallback("flybee_dmg_kick", 20.0f);
	g_SkillData.ProvideFallback("flybee_dmg_beam", 50.0f);
	g_SkillData.ProvideFallback("flybee_dmg_flyball", 20.0f);
	g_SkillData.ProvideFallback("flybee_maxspeed", 400.0f);

	g_SkillData.ProvideFallback("gargantua_stomp_initial_speed", 0.0f);

	g_SkillData.ProvideFallback("gonome_lock_player", 0.0f);

	g_SkillData.ProvideFallback("hassassin_cloaking", 0.0f, 0.0f, 1.0f);

	g_SkillData.ProvideFallback("hgrunt_gren_launch_delay", 6.0f, 6.0f, FloatRange(2.0f, 5.0f));
	g_SkillData.ProvideFallback("hgrunt_gren_throw_delay", 6.0f);
	g_SkillData.ProvideFallback("hgrunt_gren_before_cover", 0.0f, 0.0f, 1.0f);

	g_SkillData.ProvideFallback("hgrunt_ally_health", "hgrunt_health");
	g_SkillData.ProvideFallback("hgrunt_ally_kick", "hgrunt_kick");
	g_SkillData.ProvideFallback("hgrunt_ally_pellets", "hgrunt_pellets");
	g_SkillData.ProvideFallback("hgrunt_ally_gspeed", "hgrunt_gspeed");

	g_SkillData.ProvideFallback("hgrunt_ally_gren_launch_delay", 6.0f);
	g_SkillData.ProvideFallback("hgrunt_ally_gren_throw_delay", 6.0f);

	g_SkillData.ProvideFallback("medic_ally_health", "hgrunt_ally_health");
	g_SkillData.ProvideFallback("medic_ally_kick", "hgrunt_ally_kick");
	g_SkillData.ProvideFallback("medic_ally_gspeed", "hgrunt_ally_gspeed");

	g_SkillData.ProvideFallback("torch_ally_health", "hgrunt_ally_health");
	g_SkillData.ProvideFallback("torch_ally_kick", "hgrunt_ally_kick");
	g_SkillData.ProvideFallback("torch_ally_gspeed", "hgrunt_ally_gspeed");

	g_SkillData.ProvideFallback("houndeye_squad_bonus_factor", 1.1f);

	g_SkillData.ProvideFallback("ichthyosaur_maxspeed", 400.0f);

	g_SkillData.ProvideFallback("islave_zap_rate", 1.0f, 1.0f, 1.5f);
	g_SkillData.ProvideFallback("islave_revival", 0.0f, 0.0f, 1.0f);
	g_SkillData.ProvideFallback("islave_coil_attack", 1.0f);
	g_SkillData.ProvideFallbackWithFactor("islave_dmg_coil", "islave_dmg_zap", 2.5f);
	g_SkillData.ProvideFallback("islave_selfheal", "islave_dmg_zap");
	g_SkillData.ProvideFallback("islave_heal", "islave_dmg_zap");
	g_SkillData.ProvideFallback("islave_arm_boost", 1.0f);
	g_SkillData.ProvideFallbackWithFactor("islave_boosted_dmg_claw", "islave_dmg_claw", 1.5f);
	g_SkillData.ProvideFallback("islave_idle_effects", 0.0f);
	g_SkillData.ProvideFallback("islave_initial_energy", 0.0f);
	g_SkillData.ProvideFallback("islave_max_energy", "islave_health");
	g_SkillData.ProvideFallback("islave_delay_zap", FloatRange(0.5f, 4.0f));
	g_SkillData.ProvideFallback("islave_delay_coil", FloatRange(0.9f, 4.0f));
	g_SkillData.ProvideFallback("islave_fear", 1.0f);

	g_SkillData.ProvideFallback("snark_add_dmg_pop", "snark_dmg_pop");
	g_SkillData.ProvideFallback("snark_max_dmg_pop", 0.0f);
	g_SkillData.ProvideFallback("snark_lifespan", 15.0f);
	g_SkillData.ProvideFallback("snark_jump_delay", 2.0f);
	g_SkillData.ProvideFallback("snark_jump_speed", 300.0f);

	g_SkillData.ProvideFallback("penguin_health", "snark_health");
	g_SkillData.ProvideFallback("penguin_dmg_bite", "snark_dmg_bite");
	g_SkillData.ProvideFallback("penguin_dmg_pop", "plr_hand_grenade");
	g_SkillData.ProvideFallback("penguin_add_dmg_pop", "penguin_dmg_pop");
	g_SkillData.ProvideFallbackWithFactor("penguin_max_dmg_pop", "plr_hand_grenade", 5.0f);
	g_SkillData.ProvideFallback("penguin_lifespan", "snark_lifespan");
	g_SkillData.ProvideFallback("penguin_jump_delay", "snark_jump_delay");
	g_SkillData.ProvideFallback("penguin_jump_speed", "snark_jump_speed");

	g_SkillData.ProvideFallback("kate_health", "barney_health");

	g_SkillData.ProvideFallback("kingpin_health", "ichthyosaur_health");
	g_SkillData.ProvideFallback("kingpin_melee", "zombie_dmg_one_slash");
	g_SkillData.ProvideFallback("kingpin_plasma_blast", "nihilanth_zap");
	g_SkillData.ProvideFallback("kingpin_lightning", "islave_dmg_zap");
	g_SkillData.ProvideFallback("kingpin_head", 1.25f);
	g_SkillData.ProvideFallback("kingpin_shield_reserve", "kingpin_shield");

	g_SkillData.ProvideFallback("massassin_health", "hgrunt_health");
	g_SkillData.ProvideFallback("massassin_kick", "hgrunt_kick");
	g_SkillData.ProvideFallback("massassin_gspeed", "hgrunt_gspeed");

	g_SkillData.ProvideFallback("osprey", 400.0f);
	g_SkillData.ProvideFallback("blkopsosprey", "osprey");
	g_SkillData.ProvideFallback("osprey_dmg_blast", 300.0f);

	g_SkillData.ProvideFallback("otis_health", "barney_health");

	g_SkillData.ProvideFallback("panthereye_health", 150.0f);
	g_SkillData.ProvideFallback("panthereye_dmg_claw", 20.0f);

	g_SkillData.ProvideFallback("rgrunt_explode", "plr_hand_grenade");

	g_SkillData.ProvideFallback("tentacle_health", 75.0f);
	g_SkillData.ProvideFallback("tentacle_dmg_hit", 200.0f);
	g_SkillData.ProvideFallback("tentacle_dmg_tap", 20.0f);

	g_SkillData.ProvideFallback("turret_searchtime", 15.0f);
	g_SkillData.ProvideFallback("miniturret_searchtime", 15.0f);
	g_SkillData.ProvideFallback("sentry_searchtime", 15.0f);

	g_SkillData.ProvideFallback("shocktrooper_health_factor", 2.5f);
	g_SkillData.ProvideFallback("voltigore_dmg_explode", "voltigore_dmg_beam");
	g_SkillData.ProvideFallbackWithFactor("voltigore_dmg_beam_aura", "voltigore_dmg_beam", 0.2f);

	g_SkillData.ProvideFallback("tor_lift_speed_ground", 200.0f);
	g_SkillData.ProvideFallbackWithFactor("tor_lift_speed", "tor_lift_speed_ground", 0.6f);

	g_SkillData.ProvideFallback("zaptrap_sense_radius", 244.0f);
	g_SkillData.ProvideFallback("zaptrap_respawn_time", 18);

	g_SkillData.ProvideFallback("zombie_barney_health", "zombie_health");
	g_SkillData.ProvideFallback("zombie_barney_dmg_one_slash", "zombie_dmg_one_slash");
	g_SkillData.ProvideFallback("zombie_barney_dmg_both_slash", "zombie_dmg_both_slash");

	g_SkillData.ProvideFallback("tripmine_health", 1.0f);

	g_SkillData.ProvideFallback("plr_medkitshot", 10.0f);
	g_SkillData.ProvideFallback("plr_medkittime", 3.0f, 5.0f, 0.0f);
	g_SkillData.ProvideFallback("plr_uzi", 6.0f);

	g_SkillData.ProvideFallback("mortar", 200.0f);
	g_SkillData.ProvideFallbackWithFactor("op4mortar", "plr_rpg", 2.0f);

	g_SkillData.ProvideFallback("scientist_heal_time", 60.0f);
	g_SkillData.ProvideFallback("soda", 1.0f);
	g_SkillData.ProvideFallback("vortigaunt_armor_charge", "battery");

	g_SkillData.ProvideFallback("healthcharger_rate", 1.0f);
	g_SkillData.ProvideFallback("suitcharger_rate", 1.0f);

	g_SkillData.ProvideFallback("monster_head", 2.0f);
	g_SkillData.ProvideFallback("monster_chest", 1.0f);
	g_SkillData.ProvideFallback("monster_stomach", 1.0f);
	g_SkillData.ProvideFallback("monster_leg", 1.0f);
	g_SkillData.ProvideFallback("monster_arm", 1.0f);

	g_SkillData.ProvideFallback("player_head", 2.0f);
	g_SkillData.ProvideFallback("player_chest", 1.0f);
	g_SkillData.ProvideFallback("player_stomach", 1.0f);
	g_SkillData.ProvideFallback("player_arm", 1.0f);
	g_SkillData.ProvideFallback("player_leg", 1.0f);

	g_SkillData.ProvideFallback("357_bullet", 34.0f);
	g_SkillData.ProvideFallback("556_bullet", 15.0f);
	g_SkillData.ProvideFallback("762_bullet", 35.0f, 35.0f, 40.0f);
	g_SkillData.ProvideFallback("buckshot", "plr_buckshot");
	g_SkillData.ProvideFallback("plr_xbow_bolt_hitscan", 120.0f);
	g_SkillData.ProvideFallback("plr_xbow_bolt_explo", 40.0f);
	g_SkillData.ProvideFallback("plr_gauss_maxspin", 200.0f);
	g_SkillData.ProvideFallback("plr_gauss_radius_factor", 2.5f);
	g_SkillData.ProvideFallback("plr_gauss_overcharge", 50.0f);
	g_SkillData.ProvideFallback("plr_hornet_dmg", 7.0f);
	g_SkillData.ProvideFallback("plr_hand_grenade_hit", 1.0f);
	g_SkillData.ProvideFallback("shockroach", "plr_shockroachs");
	g_SkillData.ProvideFallback("plr_shockroachm", "plr_shockroachs");
	g_SkillData.ProvideFallback("plr_shockroach_discharge_factor", 100.0f);
	g_SkillData.ProvideFallback("nail", 8.0f);
	g_SkillData.ProvideFallback("plr_nail", 8.0f);
	g_SkillData.ProvideFallback("displacer_beam_dmg", 25.0f);
	g_SkillData.ProvideFallback("displacer_beam_radius", 15.0f);
	g_SkillData.ProvideFallback("plr_spore_direct", "plr_spore");
	g_SkillData.ProvideFallback("plr_grenade", "plr_hand_grenade");

	g_SkillData.ProvideFallback("plr_knife_stab_base", 20.0f);
	g_SkillData.ProvideFallback("plr_knife_stab_factor", "plr_knife");
	g_SkillData.ProvideFallback("plr_knife_stab_max", 100.0f);
	g_SkillData.ProvideFallback("plr_pipewrench_wind_base", 25.0f);
	g_SkillData.ProvideFallback("plr_pipewrench_wind_factor", "plr_pipewrench");
	g_SkillData.ProvideFallback("plr_pipewrench_wind_max", 150.0f);

	g_SkillData.ProvideFallback("flashlight_drain_time", 120.0f);
	g_SkillData.ProvideFallback("flashlight_charge_time", 20.0f);

	g_SkillData.ProvideFallback("plr_armor_strength", 2.0f);

	g_SkillData.ProvideFallback("antidote_time", 10.0f);
	g_SkillData.ProvideFallback("antirad_time", 10.0f);
	g_SkillData.ProvideFallback("adrenaline_health", 25.0f);

	g_SkillData.ProvideFallback("eyescanner_sentence_delay", 0.0f);
}

void ParseSkillCfg(const char* fileName)
{
	int fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe(fileName, &fileSize);
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing %s\n", fileName);
	ParseSkillCfg(pMemFile, fileSize, fileName);

	g_engfuncs.pfnFreeFile(pMemFile);
}

void ParseModConfigs()
{
	g_errorCollector.Clear();

	g_SkillData.Clear();
	ParseSkillCfg("skill.cfg");
	ParseSkillCfg("skillopfor.cfg");
	ProvideSkillFallbacks();

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

	PlayerTemplateSystem playerTemplateSystem;
	playerTemplateSystem.SetEntTemplateSystem(&g_EntTemplateSystem);
	playerTemplateSystem.ReadFromFile("templates/player.json");
	g_PlayerTemplateSystem = std::move(playerTemplateSystem);

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

	TimeBasedDamageDescription timeDamageDescription;
	timeDamageDescription.ReadFromFile("features/time_based_damage.json");
	g_timeBasedDamageDescription = std::move(timeDamageDescription);

	auto finish = std::chrono::steady_clock::now();
	unsigned int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count();

	ALERT(at_aiconsole, "Parsed mod configuration files in %u milliseconds\n", milliseconds);
	if (entitiesRead)
	{
		unsigned int millisecondsEntities = std::chrono::duration_cast<std::chrono::milliseconds>(finishEntities-startEntities).count();
		ALERT(at_aiconsole, "%u of them are spent on templates/entities.json\n", millisecondsEntities);
	}
}

static void ExecuteServerCommand(const char* pfile, int size)
{
	if (size <= 1)
		return;

	if (pfile[size-1] != '\n')
	{
		std::vector<char> vec(size + 2);
		memcpy(vec.data(), pfile, size);
		vec[size] = '\n';
		vec[size+1] = '\0';

		SERVER_COMMAND(vec.data());
	}
	else
	{
		SERVER_COMMAND(pfile);
	}
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit()
{
	ReadServerFeatures();
	ReadEnabledMonsters();
	ReadEnabledWeapons();
	ReadMaxAmmos();
	ReadAmmoAmounts();

	RegisterAmmoTypes();

	ParseModConfigs();

	{
		const char* fileName = "save_titles.txt";
		int fileSize;
		unsigned char *pMemFile = g_engfuncs.pfnLoadFileForMe(fileName, &fileSize);
		if (pMemFile)
		{
			ReadSaveTitles(pMemFile, fileSize, fileName);
			g_engfuncs.pfnFreeFile(pMemFile);
		}
	}

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
	CVAR_REGISTER( &build_commit_date );
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
	CVAR_REGISTER( &npc_report_fire_animevents );
	CVAR_REGISTER( &npc_idlesound_requires_pvs );

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
	CVAR_REGISTER( &tripminefix );
	CVAR_REGISTER( &explosionfix );
	CVAR_REGISTER( &monsteryawspeedfix );
	CVAR_REGISTER( &animeventfix );
	CVAR_REGISTER( &animevent_floorframe );
	CVAR_REGISTER( &anim_attack_reset_fix );
	CVAR_REGISTER( &anim_dispatch_fix );
	CVAR_REGISTER( &npc_run_task_instant );
	CVAR_REGISTER( &npc_range_attack_unlooped );
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
	CVAR_REGISTER( &mp_allowdropammo );
	CVAR_REGISTER( &sp_allowdropammo );
	CVAR_REGISTER( &npc_dropweapons );
	CVAR_REGISTER( &dmgperscore );
	CVAR_REGISTER( &allydmgpenalty );
	CVAR_REGISTER( &npckill );
	CVAR_REGISTER( &mp_playtexturesounds );

	CVAR_REGISTER( &sv_bunnyhop );

	CVAR_REGISTER( &mp_chattime );

	CVAR_REGISTER( &pickup_policy );

	CVAR_REGISTER( &sv_busters );

	CVAR_REGISTER( &grenade_jump );

	CVAR_REGISTER( &findnearestnodefix );
	CVAR_REGISTER( &nodegraph_distinfo_sort_fix );

	CVAR_REGISTER_BOOLEAN(&corpse_player_collision_fix, g_modFeatures.corpse_player_collision_fix);
	CVAR_REGISTER_BOOLEAN(&doors_open_in_move_direction, g_modFeatures.doors_open_in_move_direction);
	CVAR_REGISTER_BOOLEAN(&doors_blocked_recheck, g_modFeatures.doors_blocked_recheck);
	CVAR_REGISTER_BOOLEAN(&doors_blocked_fade_corpses, g_modFeatures.doors_blocked_fade_corpses);
	CVAR_REGISTER_INTEGER(&handle_tiny_creatures, g_modFeatures.handle_tiny_creatures);

	CVAR_REGISTER( &keepinventory );

	CVAR_REGISTER( &sv_pushable_fixed_tick_fudge );

	const char* fileName = "features/featureful_exec.cfg";
	int fileSize;
	byte *pExecFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (pExecFile)
	{
		ExecuteServerCommand((const char*)pExecFile, fileSize);
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
	g_engfuncs.pfnAddServerCommand("dump_entity_templates", ListEntityTemplates);
	g_engfuncs.pfnAddServerCommand("dump_materials", ReportMaterials);
	g_engfuncs.pfnAddServerCommand("force_schedule_fail", ForceScheduleFail);
	g_engfuncs.pfnAddServerCommand("get_skill_for_entity_template", PrintSkillReplacementForEntTemplate);
	g_engfuncs.pfnAddServerCommand("get_skill", PrintSkillVariable);
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
