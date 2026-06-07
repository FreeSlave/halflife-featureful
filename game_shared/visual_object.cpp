#include "visual_object.h"
#include "customentity.h"

#include <cstring>

void Visual::CompleteFrom(const Visual &visual)
{
	if (ShouldCompleteFrom(visual, MODEL_DEFINED))
	{
		SetModel(visual.model);
	}
	if (ShouldCompleteFrom(visual, RENDERMODE_DEFINED))
	{
		SetRenderMode(visual.rendermode);
	}
	if (ShouldCompleteFrom(visual, COLOR_DEFINED))
	{
		SetColor(visual.rendercolor);
	}
	if (ShouldCompleteFrom(visual, ALPHA_DEFINED))
	{
		SetAlpha(visual.renderamt);
	}
	if (ShouldCompleteFrom(visual, RENDERFX_DEFINED))
	{
		SetRenderFx(visual.renderfx);
	}
	if (ShouldCompleteFrom(visual, SCALE_DEFINED))
	{
		SetScale(visual.scale);
	}
	if (ShouldCompleteFrom(visual, FRAMERATE_DEFINED))
	{
		SetFramerate(visual.framerate);
	}
	if (ShouldCompleteFrom(visual, BEAMWIDTH_DEFINED))
	{
		SetBeamWidth(visual.beamWidth);
	}
	if (ShouldCompleteFrom(visual, BEAMNOISE_DEFINED))
	{
		SetBeamNoise(visual.beamNoise);
	}
	if (ShouldCompleteFrom(visual, BEAMSCROLLRATE_DEFINED))
	{
		SetBeamScrollRate(visual.beamScrollRate);
	}
	if (ShouldCompleteFrom(visual, LIFE_DEFINED))
	{
		SetLife(visual.life);
	}
	if (ShouldCompleteFrom(visual, RADIUS_DEFINED))
	{
		SetRadius(visual.radius);
	}
	if (ShouldCompleteFrom(visual, BEAMFLAGS_DEFINED))
	{
		SetBeamFlags(visual.beamFlags);
	}
	if (ShouldCompleteFrom(visual, DECAY_DEFINED))
	{
		SetDecay(visual.decay);
	}
	if (ShouldCompleteFrom(visual, WAVE_DEFINED))
	{
		SetWaveType(visual.waveType);
	}
}

void Visual::DoPrecache()
{
	if (HasModel())
	{
#if SERVER_DLL
		modelIndex = PRECACHE_MODEL(model);
#endif
	}
}

using namespace rapidjson;

static bool ParseRenderMode(const char* str, int& rendermode)
{
	constexpr std::pair<const char*, int> modes[] = {
		{"normal", kRenderNormal},
		{"color", kRenderTransColor},
		{"texture", kRenderTransTexture},
		{"glow", kRenderGlow},
		{"solid", kRenderTransAlpha},
		{"additive", kRenderTransAdd},
	};

	for (auto& p : modes)
	{
		if (stricmp(str, p.first) == 0)
		{
			rendermode = p.second;
			return true;
		}
	}
	return false;
}

static bool ParseRenderFx(const char* str, int& renderfx)
{
	constexpr std::pair<const char*, int> modes[] = {
		{"normal", kRenderFxNone},
		{"constant glow", kRenderFxNoDissipation},
		{"distort", kRenderFxDistort},
		{"hologram", kRenderFxHologram},
		{"glow shell", kRenderFxGlowShell},
	};

	for (auto& p : modes)
	{
		if (stricmp(str, p.first) == 0)
		{
			renderfx = p.second;
			return true;
		}
	}
	return false;
}

namespace detail
{
void ParseRestVisualFromJSON(Visual& visual, const Value& value)
{
	HandleJSONMember(value, "rendermode", [&visual](const Value& value) {
		if (value.IsString())
		{
			int rendermode;
			if (ParseRenderMode(value.GetString(), rendermode))
			{
				visual.SetRenderMode(rendermode);
			}
		}
		else if (value.IsInt())
		{
			visual.SetRenderMode(value.GetInt());
		}
	});

	Color3 color;
	if (UpdatePropertyFromJson(color, value, "color"))
	{
		visual.SetColor(color);
	}

	int renderamt;
	if (UpdatePropertyFromJson(renderamt, value, "alpha"))
	{
		visual.SetAlpha(renderamt);
	}

	HandleJSONMember(value, "renderfx", [&visual](const Value& value) {
		if (value.IsString())
		{
			int renderfx;
			if (ParseRenderFx(value.GetString(), renderfx))
			{
				visual.SetRenderFx(renderfx);
			}
		}
		else if (value.IsInt())
		{
			visual.SetRenderFx(value.GetInt());
		}
	});

	FloatRange scale;
	if (UpdatePropertyFromJson(scale, value, "scale"))
	{
		visual.SetScale(scale);
	}

	FloatRange framerate;
	if (UpdatePropertyFromJson(framerate, value, "framerate"))
	{
		visual.SetFramerate(framerate);
	}

	int beamWidth, beamNoise, beamScrollRate;
	if (UpdatePropertyFromJson(beamWidth, value, "width"))
	{
		visual.SetBeamWidth(beamWidth);
	}
	if (UpdatePropertyFromJson(beamNoise, value, "noise"))
	{
		visual.SetBeamNoise(beamNoise);
	}
	if (UpdatePropertyFromJson(beamScrollRate, value, "scrollrate"))
	{
		visual.SetBeamScrollRate(beamScrollRate);
	}

	FloatRange life;
	if (UpdatePropertyFromJson(life, value, "life"))
	{
		visual.SetLife(life);
	}

	IntRange radius;
	if (UpdatePropertyFromJson(radius, value, "radius"))
	{
		visual.SetRadius(radius);
	}

	HandleJSONMember(value, "beamflags", [&visual](const Value& value) {
		int beamFlags = 0;
		Value::ConstArray arr = value.GetArray();
		for (auto& item : arr)
		{
			const char* str = item.GetString();
			if (stricmp(str, "sine") == 0)
				beamFlags |= BEAM_FSINE;
			else if (stricmp(str, "solid") == 0)
				beamFlags |= BEAM_FSOLID;
			else if (stricmp(str, "shadein") == 0)
				beamFlags |= BEAM_FSHADEIN;
			else if (stricmp(str, "shadeout") == 0)
				beamFlags |= BEAM_FSHADEOUT;
		}
		visual.SetBeamFlags(beamFlags);
	});

	float decay;
	if (UpdatePropertyFromJson(decay, value, "decay"))
	{
		visual.SetDecay(decay);
	}

	HandleJSONMember(value, "wave", [&visual](const Value& value) {
		int waveType = Visual::WAVETYPE_CYLINDER;
		const char* str = value.GetString();
		if (stricmp(str, "torus") == 0)
		{
			waveType = Visual::WAVETYPE_TORUS;
		}
		else if (stricmp(str, "disk") == 0)
		{
			waveType = Visual::WAVETYPE_DISK;
		}
		visual.SetWaveType(waveType);
	});
}
}
