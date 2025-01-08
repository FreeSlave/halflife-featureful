#pragma once
#ifndef TEX_MATERIALS_H
#define TEX_MATERIALS_H

#include "const_sound.h"
#include "min_and_max.h"
#include "fixed_string.h"
#include "fixed_vector.h"
#include "template_property_types.h"
#include "json_config.h"
#include <cstddef>
#include <array>
#include <map>

void GetStrippedTextureName(char* szbuffer, const char* pTextureName);

typedef fixed_string<64> MaterialSoundString;

struct MaterialStepSoundData
{
	float volume;
	int timeMsec;
};

struct MaterialStepData
{
	MaterialStepData();

	typedef fixed_vector<MaterialSoundString, 5> StepSoundArray;

	void SetRight(std::initializer_list<const char *> sounds) {
		right = sounds;
	}
	void SetLeft(std::initializer_list<const char *> sounds) {
		left = sounds;
	}
	inline bool IsDefined() const {
		return !left.empty() && !right.empty();
	}

	StepSoundArray right;
	StepSoundArray left;
	MaterialStepSoundData walking;
	MaterialStepSoundData running;
	bool skipSomeSteps = false;
};

struct MaterialHitData
{
	float volume = 0.9f;
	float volumebar = 0.5f;
	float attn = ATTN_NORM;

	bool allowWallpuff = true;
	bool skipMeleeSound = false;
	bool allowWeaponSparks = true;
	bool playSparks = false;

	Color3 wallpuffColor = Color3(40, 40, 40);

	fixed_vector<MaterialSoundString, 5> waves;

	void SetWaves(std::initializer_list<const char *> sounds) {
		waves = sounds;
	}
};

struct MaterialData
{
	MaterialStepData step;
	MaterialHitData hit;
};

class MaterialRegistry : public JSONConfig
{
public:
	void FillDefaults();
	void SetMaterialData(char c, const MaterialData& data);
	const MaterialData* GetMaterialData(char c) const;
	const MaterialData* GetMaterialDataWithFallback(char c) const;
	const MaterialStepData* GetMaterialStepData(char c) const;
	const MaterialStepData* GetSloshStepData() const;

	char DefaultMaterial() const {
		return _defaultMaterial;
	}
	char FleshMaterial() const {
		return _fleshMaterial;
	}

	inline const MaterialStepData* GetLadderStepData() const {
		return &_ladder;
	}
	inline void SetLadderStepData(const MaterialStepData& stepData) {
		_ladder = stepData;
	}
	inline const MaterialStepData* GetWadeStepData() const {
		return &_wade;
	}
	inline void SetWadeStepData(const MaterialStepData& stepData) {
		_wade = stepData;
	}

	void DumpMaterials() const;
	void DumpMaterial(char c) const;

protected:
	virtual const char* Schema() const;
	virtual bool ReadFromDocument(rapidjson::Document& document, const char* fileName);

private:
	void DumpMaterial(char c, const MaterialData& data) const;

	std::map<char, MaterialData> _materials;
	MaterialStepData _ladder;
	MaterialStepData _wade;
	char _defaultMaterial;
	char _defaultStepMaterial;
	char _sloshMaterial;
	char _fleshMaterial;
};

extern MaterialRegistry g_MaterialRegistry;

#endif
