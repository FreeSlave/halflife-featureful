#include <algorithm>
#include <cstring>
#include "string_utils.h"

#include "ammo_amounts.h"

struct AmmoComparator
{
	bool operator()(const AmmoAmountInfo& lhs, const char* rhs)
	{
		return strcmp(lhs.entityName, rhs) < 0;
	}
	bool operator()(const char* lhs, const AmmoAmountInfo& rhs)
	{
		return strcmp(lhs, rhs.entityName) < 0;
	}
	bool operator()(const AmmoAmountInfo& lhs, const AmmoAmountInfo& rhs)
	{
		return strcmp(lhs.entityName, rhs.entityName) < 0;
	}
};

bool AmmoAmounts::RegisterAmountForAmmoEnt(const char* entityName, int amount)
{
	if (AmountForAmmoEnt(entityName) > 0) {
		return false; // already registered
	}
	AmmoAmountInfo info;
	strncpyEnsureTermination(info.entityName, entityName);
	info.amount = amount;
	amounts.push_back(info);
	std::sort(amounts.begin(), amounts.end(), AmmoComparator());
	return true;
}

int AmmoAmounts::AmountForAmmoEnt(const char* entityName)
{
	auto result = std::equal_range(amounts.begin(), amounts.end(), entityName, AmmoComparator());
	if (result.first != result.second)
	{
		return result.first->amount;
	}
	return -1;
}

AmmoAmounts g_AmmoAmounts;
