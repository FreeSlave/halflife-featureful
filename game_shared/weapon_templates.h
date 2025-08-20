#pragma once
#ifndef WEAPON_TEMPLATES_H
#define WEAPON_TEMPLATES_H

#include "common_types.h"
#include "fixed_string.h"
#include "json_config.h"
#include "optional.h"
#include "weapon_parameters.h"

#include <string>
#include <map>
#include <set>

class WeaponTemplateSystem : public JSONConfig
{
public:
	void ParseWeaponTemplate(WeaponParameters& params, const rapidjson::Value& value, const char* fileName);
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
	void ParseWeaponSoundScript(WeaponSoundScript& soundScript, const rapidjson::Value& value);
private:
	const char* MakeConstantString(const char* str);

	std::set<std::string> _stringSet;
};

extern WeaponTemplateSystem g_WeaponTemplateSystem;

#endif
