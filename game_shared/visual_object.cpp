#include "visual_object.h"
#include "customentity.h"

#include <cstring>

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

	float framerate;
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
