#pragma once
#ifndef TIME_BASED_DAMAGE_H
#define TIME_BASED_DAMAGE_H

#include "json_config.h"
#include "template_property_types.h"

struct TimeBasedDamageInfo
{
	int tickCount{0};
	float damagePerTick{0.0f};
	float interval{2.0f};
	float firstDelay{0.0f};

	AbsoluteValueOrFactor playedMaxSpeed;
};

class TimeBasedDamageDescription : public JSONConfig
{
public:
	TimeBasedDamageDescription();

	TimeBasedDamageInfo paralyze;
	TimeBasedDamageInfo nerveGas;
	TimeBasedDamageInfo poison;
	TimeBasedDamageInfo radiation;
	TimeBasedDamageInfo acid;
	TimeBasedDamageInfo slowBurn;
	TimeBasedDamageInfo slowFreeze;

protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
};

extern TimeBasedDamageDescription g_timeBasedDamageDescription;

#endif
