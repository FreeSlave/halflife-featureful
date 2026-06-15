#pragma once
#ifndef VISUAL_OBJECT_H
#define VISUAL_OBJECT_H

#include "const_render.h"
#include "template_property_types.h"
#include "rapidjson/document.h"
#include "json_utils.h"
#include "logger.h"

struct Visual
{
	enum
	{
		MODEL_DEFINED = (1 << 0),
		RENDERMODE_DEFINED = (1 << 1),
		COLOR_DEFINED = (1 << 2),
		ALPHA_DEFINED = (1 << 3),
		RENDERFX_DEFINED = (1 << 4),
		SCALE_DEFINED = (1 << 5),
		FRAMERATE_DEFINED = (1 << 6),
		BEAMWIDTH_DEFINED = (1 << 7),
		BEAMNOISE_DEFINED = (1 << 8),
		BEAMSCROLLRATE_DEFINED = (1 << 9),
		LIFE_DEFINED = (1 << 10),
		RADIUS_DEFINED = (1 << 11),
		BEAMFLAGS_DEFINED = (1 << 12),
		DECAY_DEFINED = (1 << 13),
		WAVE_DEFINED = (1 << 14),
	};

	enum
	{
		WAVETYPE_CYLINDER,
		WAVETYPE_TORUS,
		WAVETYPE_DISK,
	};

	const char* model = nullptr;
	int rendermode = kRenderNormal;
	Color3 rendercolor;
	IntRange renderamt{0};
	int renderfx = kRenderFxNone;
	FloatRange scale = 1.0f;
	FloatRange framerate = 0.0f;
	int beamWidth = 0;
	int beamNoise = 0;
	int beamScrollRate = 0;
	FloatRange life = 0.0f;
	IntRange radius{0};
	int beamFlags = 0;
	float decay = 0.0f;
	int waveType = WAVETYPE_CYLINDER;

	int modelIndex = 0;

	inline void SetModel(const char* model)
	{
		this->model = model;
		MarkAsDefined(MODEL_DEFINED);
	}
	inline void SetRenderMode(int rendermode)
	{
		this->rendermode = rendermode;
		MarkAsDefined(RENDERMODE_DEFINED);
	}
	inline void SetColor(Color3 rendercolor)
	{
		this->rendercolor = rendercolor;
		MarkAsDefined(COLOR_DEFINED);
	}
	inline void SetAlpha(IntRange alpha)
	{
		this->renderamt = alpha;
		MarkAsDefined(ALPHA_DEFINED);
	}
	inline void SetRenderFx(int renderfx)
	{
		this->renderfx = renderfx;
		MarkAsDefined(RENDERFX_DEFINED);
	}
	inline void SetScale(FloatRange scale)
	{
		this->scale = scale;
		MarkAsDefined(SCALE_DEFINED);
	}
	inline void SetFramerate(FloatRange framerate)
	{
		this->framerate = framerate;
		MarkAsDefined(FRAMERATE_DEFINED);
	}
	inline void SetBeamWidth(int width)
	{
		this->beamWidth = width;
		MarkAsDefined(BEAMWIDTH_DEFINED);
	}
	inline void SetBeamNoise(int noise)
	{
		this->beamNoise = noise;
		MarkAsDefined(BEAMNOISE_DEFINED);
	}
	inline void SetBeamScrollRate(int scrollRate)
	{
		this->beamScrollRate = scrollRate;
		MarkAsDefined(BEAMSCROLLRATE_DEFINED);
	}
	inline void SetLife(FloatRange life)
	{
		this->life = life;
		MarkAsDefined(LIFE_DEFINED);
	}
	inline void SetRadius(IntRange radius)
	{
		this->radius = radius;
		MarkAsDefined(RADIUS_DEFINED);
	}
	inline void SetBeamFlags(int flags)
	{
		this->beamFlags = flags;
		MarkAsDefined(BEAMFLAGS_DEFINED);
	}
	inline void SetDecay(float decay)
	{
		this->decay = decay;
		MarkAsDefined(DECAY_DEFINED);
	}
	inline void SetWaveType(int waveType)
	{
		this->waveType = waveType;
		MarkAsDefined(WAVE_DEFINED);
	}

	inline bool HasModel() const {
		return model && *model;
	}
	inline bool HasDefined(int param) const {
		return (defined & param) != 0;
	}
	inline void MarkAsDefined(int param) {
		defined |= param;
	}
	void DoPrecache();
	void CompleteFrom(const Visual& visual);

private:
	int defined = 0;

	inline bool ShouldCompleteFrom(const Visual& visual, int param) const {
		return visual.HasDefined(param) && !HasDefined(param);
	}
};

namespace detail
{
void ParseRestVisualFromJSON(Visual& visual, const rapidjson::Value& value);
}

template<typename ConstantStringMaker>
Visual ParseVisualFromJSON(const rapidjson::Value& value, const ConstantStringMaker& makeConstantString)
{
	Visual visual;

	HandleJSONMember(value, "model", [&visual, &makeConstantString](const rapidjson::Value& value) {
		visual.SetModel(makeConstantString(value.GetString()));
	});

	HandleJSONMember(value, "sprite", [&visual, &makeConstantString](const rapidjson::Value& value) {
		visual.SetModel(makeConstantString(value.GetString()));
	});

	detail::ParseRestVisualFromJSON(visual, value);
	return visual;
}

#endif
