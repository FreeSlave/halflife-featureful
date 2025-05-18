#pragma once
#ifndef WEAPON_TEMPLATES_H
#define WEAPON_TEMPLATES_H

#include "json_config.h"
#include "optional.h"

#include <string>
#include <map>

struct WeaponTemplate
{
	bool IsMaxClipDefined() const {
		return _maxClip.has_value();
	}
	int MaxClip() const {
		return _maxClip.value_or(-1);
	}
	void SetMaxClip(int maxClip) {
		_maxClip = maxClip;
	}
private:
	optional<int> _maxClip;
};

class WeaponTemplateSystem : public JSONConfig
{
public:
	const WeaponTemplate* GetTemplate(const char* name);
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
private:
	std::map<std::string, WeaponTemplate> _templates;
	std::string _temp;
};

extern WeaponTemplateSystem g_WeaponTemplateSystem;

#endif
