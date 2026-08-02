#pragma once
#ifndef MAPCONFIG_H
#define MAPCONFIG_H

#include "extdll.h"
#include "optional.h"
#include "fixed_string.h"
#include "fixed_vector.h"

enum SuitLogon
{
	SuitNoLogon = 0,
	SuitShortLogon,
	SuitLongLogon
};

#define MAPCONFIG_ENTRY_LENGTH 32
#define MAPCONFIG_MAX_OVERRIDE_CVARS 32
#define MAPCONFIG_MAX_PICKUP_ENTS 64

struct MapConfig
{
	struct PickupEnt
	{
		string_t entName;
		int count;
	};

	struct AmmoQuantity
	{
		fixed_string<MAPCONFIG_ENTRY_LENGTH> name;
		int count;
	};

	struct OverrideCvar
	{
		fixed_string<MAPCONFIG_ENTRY_LENGTH> name;
		fixed_string<MAPCONFIG_ENTRY_LENGTH> value;
	};

	fixed_vector<PickupEnt, MAPCONFIG_MAX_PICKUP_ENTS> pickupEnts;
	fixed_vector<AmmoQuantity, MAX_AMMO_TYPES> ammo;
	fixed_vector<PickupEnt, MAX_INVENTORY_ITEMS> inventory;

	fixed_vector<OverrideCvar, MAPCONFIG_MAX_OVERRIDE_CVARS> overrideCvars;

	string_t playerTemplate{0};
	int starthealth{0};
	int startarmor{0};
	int maxhealth{0};
	optional<int> maxarmor;

	bool nomedkit{false}; // for co-op

	bool nosuit{false};
	short suitLogon{SuitNoLogon};

	enum
	{
		SUIT_LIGHT_NOTHING = -1,
		SUIT_LIGHT_DEFAULT = 0,
		SUIT_LIGHT_FLASHLIGHT,
		SUIT_LIGHT_NVG,
	};
	int suit_light{SUIT_LIGHT_DEFAULT};

	fixed_string<64> deployWeapon;

	int antidotes{0};
	int radcans{0};
	int adrenalines{0};

	bool longjump{false};

	bool valid{false};
};

bool ReadMapConfigFromText(MapConfig& mapConfig, byte* pMemFile, int fileSize);
bool ReadMapConfigFromFile(MapConfig& mapConfig, const char* fileName);
bool ReadMapConfigByMapName(MapConfig& mapConfig, const char* mapName);

#endif
