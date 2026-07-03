#pragma once
#ifndef DAMAGEINFO_H
#define DAMAGEINFO_H

#include "optional.h"
#include "tribool.h"

#include "gib.h"
#include "dmg_types.h"
#include "template_property_types.h"
#include "skillbasedvalue.h"

struct DamageInfo
{
	DamageInfo() {}
	DamageInfo(float dmg, int dmgType): damage(dmg), type(dmgType) {}
	float damage = 0.0f;
	int type = DMG_GENERIC;
	int gibPolicy = GIB_NORMAL;
	bool nonLethal = false; // this damage shouldn't kill player or monster
	bool timedNonLethal = false;
	bool ignoreArmor = false; // ignore player's armor, deal damage to health only
	bool timedIgnoreArmor = false;
	bool noPlayerPush = false; // don't push player
	bool noPunch = false; // don't make a smalle punch on player's camera
	bool noBlood = false; // used in TraceAttack. Force not to bleed.
	bool ignoreTransform = false;
	bool enforceLightDamage = false;
	bool ignorePowerShield = false;

	bool mustSkip = false;

	DamageInfo& SetGibPolicy(int gib) {
		gibPolicy = gib;
		return *this;
	}
	DamageInfo& SetNonLethal(bool enable = true) {
		nonLethal = enable;
		return *this;
	}
	DamageInfo& SetTimedNonLethal(bool enable = true) {
		timedNonLethal = enable;
		return *this;
	}
	DamageInfo& SetIgnoreArmor(bool enable = true) {
		ignoreArmor = enable;
		return *this;
	}
	DamageInfo& SetTimedIgnoreArmor(bool enable = true) {
		timedIgnoreArmor = enable;
		return *this;
	}
	DamageInfo& SetNoPlayerPush(bool enable = true) {
		noPlayerPush = enable;
		return *this;
	}
	DamageInfo& SetNoPunch(bool enable = true) {
		noPunch = enable;
		return *this;
	}
	DamageInfo& SetNoBlood(bool enable = true) {
		noBlood = enable;
		return *this;
	}
	DamageInfo& SetIgnoreTransform(bool enable = true) {
		ignoreTransform = enable;
		return *this;
	}
	DamageInfo& SetIgnorePowerShield(bool enable = true) {
		ignorePowerShield = enable;
		return *this;
	}
	DamageInfo& SetMakePureDamageToHealth() {
		SetIgnoreArmor();
		SetIgnoreTransform();
		SetIgnorePowerShield();
		return *this;
	}
};

struct DamageInfoPatch
{
	enum
	{
		REPLACE_DAMAGE_TYPE,
		ADD_DAMAGE_TYPE,
	};

	optional<SkillBasedValue> damage;
	optional<int> type;
	int typePolicy = ADD_DAMAGE_TYPE;
	optional<int> gibPolicy;
	tribool nonLethal;
	tribool ignoreArmor;
	tribool noBlood;
	tribool timedNonLethal;
	tribool timedIgnoreArmor;
};

int ParseDamageType(const char *type);

void ApplyDamageInfoPatch(DamageInfo& curDamageInfo, const DamageInfoPatch& damageInfo);

#endif
