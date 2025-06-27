#include <cstring>

#include "ammoregistry.h"
#include "arraysize.h"
#include "logger.h"
#include "string_utils.h"
#if !CLIENT_DLL
#include "util.h"
#endif

void AmmoType::SetName(const char *ammoName)
{
	strncpyEnsureTermination(name, ammoName);
}

bool AmmoType::IsValid() const
{
	if (!name || *name == '\0')
		return false;
	return id > 0;
}

int AmmoRegistry::Register(const char *name, int maxAmmo, bool exhaustible)
{
	if (!name)
		return -1;
	// make sure it's not already in the registry
	const int index = IndexOf(name);
	if (index > 0)
	{
		const AmmoType* type = GetByIndex(index);
		if (type->maxAmmo != maxAmmo || type->exhaustible != exhaustible)
		{
			LOG_ERROR("Trying to re-register ammo '%s' with different parameters\n", name);
		}
		return index;
	}

	if (lastAmmoIndex >= MAX_AMMO_TYPES - 1)
	{
		LOG_ERROR("Too many ammo types. Max is %d\n", MAX_AMMO_TYPES-1);
		return -1;
	}

	if (maxAmmo <= 0)
	{
		LOG_ERROR("Invalid max ammo (%d) for '%s'\n", maxAmmo, name);
		return -1;
	}

	AmmoType& type = ammoTypes[lastAmmoIndex++];
	type.id = lastAmmoIndex;
	type.SetName(name);
	type.maxAmmo = maxAmmo;
	type.exhaustible = exhaustible;

	return type.id;
}

void AmmoRegistry::RegisterOnClient(const char *name, int maxAmmo, int index, bool exhaustible)
{
	if (!name)
		return;
	if (index <= 0 || index >= MAX_AMMO_TYPES)
	{
		LOG_ERROR("Invalid ammo index %d\n", index);
		return;
	}
	AmmoType& type = ammoTypes[index - 1];
	type.SetName(name);
	type.maxAmmo = maxAmmo;
	type.id = index;
	type.exhaustible = exhaustible;

	if (type.id > lastAmmoIndex)
		lastAmmoIndex = type.id;
}

const AmmoType* AmmoRegistry::GetByName(const char *name) const
{
	return GetByIndex(IndexOf(name));
}

const AmmoType* AmmoRegistry::GetByIndex(int id) const
{
	if (id > 0 && id <= MAX_AMMO_TYPES)
	{
		const AmmoType& ammoType = ammoTypes[id-1];
		if (ammoType.IsValid())
			return &ammoType;
	}
	return NULL;
}

int AmmoRegistry::IndexOf(const char *name) const
{
	if (!name)
		return 0;
	for (int i = 0; i < static_cast<int>(ARRAYSIZE(ammoTypes)); ++i)
	{
		if (!ammoTypes[i].IsValid())
			continue;
		if (stricmp(name, ammoTypes[i].name) == 0)
		{
			return i+1;
		}
	}
	return 0;
}

int AmmoRegistry::GetMaxAmmo(const char *name) const
{
	const AmmoType* ammoType = GetByName(name);
	if (ammoType)
		return ammoType->maxAmmo;
	return -1;
}

int AmmoRegistry::GetMaxAmmo(int index) const
{
	const AmmoType* ammoType = GetByIndex(index);
	if (ammoType)
		return ammoType->maxAmmo;
	return -1;
}

void AmmoRegistry::SetMaxAmmo(const char *name, int maxAmmo)
{
	int id = IndexOf(name);
	if (id > 0 && id <= MAX_AMMO_TYPES)
	{
		AmmoType& ammoType = ammoTypes[id-1];
		if (ammoType.IsValid())
		{
			ammoType.maxAmmo = maxAmmo;
		}
	}
}

void AmmoRegistry::ReportRegisteredTypes() const
{
	for (int i = 0; i<lastAmmoIndex; ++i)
	{
		ReportRegisteredType(ammoTypes[i]);
	}
}

void AmmoRegistry::ReportRegisteredType(const AmmoType& ammoType) const
{
	LOG_DEV("%s. Max ammo: %d. Index: %d. %s\n", ammoType.name, ammoType.maxAmmo, ammoType.id, ammoType.exhaustible ? "Exhaustible" : "");
}

AmmoRegistry g_AmmoRegistry;

void ReportRegisteredAmmoTypes()
{
	g_AmmoRegistry.ReportRegisteredTypes();
}
