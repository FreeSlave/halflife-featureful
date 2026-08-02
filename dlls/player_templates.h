#pragma once
#ifndef PLAYER_TEMPLATES
#define PLAYER_TEMPLATES

#include <map>
#include <set>
#include <string>

#include "common_limits.h"
#include "fixed_string.h"
#include "icase_compare.h"
#include "json_config.h"
#include "ent_templates.h"

class PlayerTemplateSystem;

struct PlayerTemplate
{
	enum
	{
		HEV_DEAD_DEFAULT = 0,
		HEV_DEAD_ALWAYS,
		HEV_DEAD_SUIT,
		HEV_DEAD_NEVER
	};

	struct WeaponReplacement
	{
		std::string viewModel;
		std::string viewModelDetonator;
	};

	bool HasAnyWeaponReplacaments() const {
		return !weapons.empty();
	}
	const WeaponReplacement* GetWeaponReplacement(const char* name) const;
	bool IsItemAllowed(const char* name) const;

	Color3 hudColor{};
	Color3 hudColorNoSuit{};
	Color3 hudColorCritical{};
	float maxSpeed{0.0f};
	tribool suitSentences;
	tribool hudDrawNoSuit;
	int playHevDead;
	tribool nosuitAllowHealthCharger;
	std::string entTemplateName;

	friend class PlayerTemplateSystem;
private:
	mutable std::string _temp;
	std::set<std::string> allowedItems;
	std::set<std::string> prohibitedItems;
	std::map<std::string, WeaponReplacement> weapons;
};

class PlayerTemplateSystem : public JSONConfig
{
public:
	typedef std::map<std::string, PlayerTemplate, CaseInsensitiveCompare> PlayerTemplatesMap;

	void SetEntTemplateSystem(EntTemplateSystem* entTemplateSystem) {
		_entTemplateSystem = entTemplateSystem;
	}
	const PlayerTemplate* GetTemplate(const char* name) const;
	const PlayerTemplate* GetDefaultTemplate() const {
		return _defaultPlayerTemplate;
	}
	PlayerTemplatesMap::const_iterator PlayerTemplatesBegin() const {
		return _playerTemplates.cbegin();
	}
	PlayerTemplatesMap::const_iterator PlayerTemplatesEnd() const {
		return _playerTemplates.cend();
	}
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
private:
	PlayerTemplatesMap _playerTemplates;
	const PlayerTemplate* _defaultPlayerTemplate = nullptr;
	EntTemplateSystem* _entTemplateSystem = nullptr;
};

extern PlayerTemplateSystem g_PlayerTemplateSystem;

#endif
