#include "mapconfig.h"
#include "util.h"
#include "weapons.h"
#include "parsetext.h"

PickupEnt::PickupEnt(): entName(0), count(0) {}

MapConfig::MapConfig() :
	pickupEntCount(0), ammoCount(0), cvarCount(0),
	starthealth(0), startarmor(0),
	maxhealth(0), maxarmor(0),
	nomedkit(false), nosuit(false),
	suitLogon(SuitNoLogon), suit_light(SUIT_LIGHT_DEFAULT), longjump(false),
	valid(false)
{
	memset(ammo, 0, sizeof(ammo));
	memset(overrideCvars, 0, sizeof(overrideCvars));
}

const char* FixedAmmoName(const char* ammoName)
{
	if (stricmp(ammoName, "Hand_Grenade") == 0)
		return "Hand Grenade";
	else if (stricmp(ammoName, "Satchel_Charge") == 0)
		return "Satchel Charge";
	else if (stricmp(ammoName, "Trip_Mine") == 0)
		return "Trip Mine";
	return ammoName;
}

bool ReadMapConfigFromText(MapConfig& mapConfig, byte* pMemFile, int fileSize)
{
	int filePos = 0;
	char buffer[512];
	memset( buffer, 0, sizeof(buffer) );
	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer) ) != NULL )
	{
		int i = 0;
		while( buffer[i] && buffer[i] == ' ' )
			i++;
		if( !buffer[i] || buffer[i] == '\n' )
			continue;
		if( buffer[i] == '/' || !isalpha( buffer[i] ) )
			continue;
		int j = i;
		while( buffer[j] && buffer[j] != ' ' && buffer[j] != '\n' )
			j++;
		char* key = buffer+i;
		char* value = buffer+j;
		if (buffer[j] && buffer[j] != '\n')
		{
			value = buffer+j+1;
			while(*value && *value == ' ')
				value++;
			int k = 0;
			while( value[k] && value[k] != ' ' && value[k] != '\n' )
				k++;
			value[k] = '\0';
		}

		key[j-i] = '\0';
		if (strncmp(key, "weapon_", 7) == 0 || strncmp(key, "ammo_", 5) == 0)
		{
			if (mapConfig.pickupEntCount < MAPCONFIG_MAX_PICKUP_ENTS)
			{
				mapConfig.pickupEnts[mapConfig.pickupEntCount].entName = ALLOC_STRING(key);
				int count = atoi(value);
				if (count <= 0)
					count = 1;
				mapConfig.pickupEnts[mapConfig.pickupEntCount].count = count;
				mapConfig.pickupEntCount++;
			}
		}
		else if (strncmp(key, "ammo!", 5) == 0)
		{
			const char* ammoName = key + 5;
			if (*ammoName)
			{
				int count = atoi(value);
				if (count > 0)
				{
					ammoName = FixedAmmoName(ammoName);
					strncpyEnsureTermination(mapConfig.ammo[mapConfig.ammoCount].name, ammoName);
					mapConfig.ammo[mapConfig.ammoCount].count = count;
					mapConfig.ammoCount++;
				}
			}
		}
		else if (strcmp(key, "nomedkit") == 0)
		{
			mapConfig.nomedkit = true;
		}
		else if (strcmp(key, "nosuit") == 0)
		{
			mapConfig.nosuit = true;
		}
		else if (strcmp(key, "suitlight") == 0)
		{
			if (strcmp(value, "flashlight") == 0)
			{
				mapConfig.suit_light = MapConfig::SUIT_LIGHT_FLASHLIGHT;
			}
			else if (strcmp(value, "nvg") == 0 || strcmp(value, "nightvision") == 0)
			{
				mapConfig.suit_light = MapConfig::SUIT_LIGHT_NVG;
			}
			else if (strcmp(value, "no") == 0 || strcmp(value, "nothing") == 0)
			{
				mapConfig.suit_light = MapConfig::SUIT_LIGHT_NOTHING;
			}
		}
		else if (strcmp(key, "suitlogon") == 0)
		{
			if (strcmp(value, "short"))
			{
				mapConfig.suitLogon = SuitShortLogon;
			}
			else if (strcmp(value, "long"))
			{
				mapConfig.suitLogon = SuitLongLogon;
			}
			else if (strcmp(value, "no"))
			{
				mapConfig.suitLogon = SuitNoLogon;
			}
		}
		else if (strcmp(key, "item_longjump") == 0)
		{
			mapConfig.longjump = true;
		}
		else if (strcmp(key, "startarmor") == 0)
		{
			mapConfig.startarmor = atoi(value);
		}
		else if (strcmp(key, "starthealth") == 0)
		{
			mapConfig.starthealth = atoi(value);
		}
		else if (strcmp(key, "maxhealth") == 0)
		{
			mapConfig.maxhealth = atoi(value);
		}
		else if (strcmp(key, "maxarmor") == 0)
		{
			mapConfig.maxarmor = atoi(value);
		}
		else if (strncmp(key, "sv_", 3) == 0 || strncmp(key, "mp_", 3) == 0 || strncmp(key, "npc_", 4) == 0)
		{
			if (mapConfig.cvarCount < 32)
			{
				strncpyEnsureTermination(mapConfig.overrideCvars[mapConfig.cvarCount].name, key);
				strncpyEnsureTermination(mapConfig.overrideCvars[mapConfig.cvarCount].value, value);
				mapConfig.cvarCount++;
			}
		}
	}

	mapConfig.valid = true;
	return true;
}

bool ReadMapConfigFromFile(MapConfig& mapConfig, const char* fileName)
{
	int fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if( !pMemFile )
		return false;
	bool result = ReadMapConfigFromText(mapConfig, pMemFile, fileSize);
	g_engfuncs.pfnFreeFile( pMemFile );
	return result;
}

bool ReadMapConfigByMapName(MapConfig& mapConfig, const char* mapName)
{
	char fileName[cchMapNameMost + 10];
	sprintf(fileName, "maps/%s.cfg", mapName);
	return ReadMapConfigFromFile(mapConfig, fileName);
}
