#pragma once
#ifndef AMMO_AMOUNTS
#define AMMO_AMOUNTS

#include <vector>

struct AmmoAmountInfo
{
	char entityName[64];
	int amount;
};

class AmmoAmounts
{
public:
	bool RegisterAmountForAmmoEnt(const char* entityName, int amount);
	int AmountForAmmoEnt(const char* entityName);
private:
	std::vector<AmmoAmountInfo> amounts;
};

extern AmmoAmounts g_AmmoAmounts;

#endif
