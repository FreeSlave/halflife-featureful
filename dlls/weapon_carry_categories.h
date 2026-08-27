#pragma once
#ifndef WEAPON_CARRY_CATEGORIES_H
#define WEAPON_CARRY_CATEGORIES_H

#include <map>

class WeaponCarryCategories
{
public:
	void SetWeaponCategory(int weaponId, int category);
	int Categorize(int weaponId);
	void Reset() {
		weaponToCategory.clear();
	}
	bool Empty() const {
		return weaponToCategory.empty();
	}
private:
	std::map<int, int> weaponToCategory;
};

extern WeaponCarryCategories g_WeaponCarryCategories;

#endif
