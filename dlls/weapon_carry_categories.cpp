#include "weapon_carry_categories.h"

void WeaponCarryCategories::SetWeaponCategory(int weaponId, int category)
{
	if (!category)
	{
		weaponToCategory.erase(weaponId);
		return;
	}
	weaponToCategory[weaponId] = category;
}

int WeaponCarryCategories::Categorize(int weaponId)
{
	auto it = weaponToCategory.find(weaponId);
	if (it != weaponToCategory.end())
	{
		return it->second;
	}
	return 0;
}

WeaponCarryCategories g_WeaponCarryCategories;
