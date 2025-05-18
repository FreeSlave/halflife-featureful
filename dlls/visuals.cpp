#include "visuals.h"
#include "customentity.h"
#include "error_collector.h"
#include "logger.h"
#include "util_shared.h"

#include "json_utils.h"

using namespace rapidjson;

const char* visualsSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"$ref": "definitions.json#/visual"
	}
}
)";

void Visual::DoPrecache()
{
	if (HasModel())
	{
#if SERVER_DLL
		modelIndex = PRECACHE_MODEL(model);
#endif
	}
}

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
}

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

const char* VisualSystem::Schema() const
{
	return visualsSchema;
}

bool VisualSystem::ReadFromDocument(const Document& document, const char *fileName)
{
	for (auto scriptIt = document.MemberBegin(); scriptIt != document.MemberEnd(); ++scriptIt)
	{
		const char* name = scriptIt->name.GetString();

		const Value& value = scriptIt->value;
		if (value.IsObject())
			AddVisualFromJsonValue(name, value);
		else
			g_errorCollector.AddFormattedError("%s: visual '%s' is not an object!\n", fileName, name);
	}

	return true;
}

void VisualSystem::AddVisualFromJsonValue(const char *name, const Value &value)
{
	Visual visual;

	HandleJSONMember(value, "model", [&visual, this](const Value& value) {
		std::string str = value.GetString();
		auto strIt = _modelStringSet.find(str);
		if (strIt == _modelStringSet.end())
		{
			auto p = _modelStringSet.insert(str);
			strIt = p.first;
		}
		visual.SetModel(strIt->c_str());
	});

	HandleJSONMember(value, "sprite", [&visual, this, name](const Value& value) {
		if (visual.HasDefined(Visual::MODEL_DEFINED))
		{
			LOG_WARNING("Visual \"%s\" has both 'model' and 'sprite' properties defined!\n", name);
		}
		else
		{
			std::string str = value.GetString();
			auto strIt = _modelStringSet.find(str);
			if (strIt == _modelStringSet.end())
			{
				auto p = _modelStringSet.insert(str);
				strIt = p.first;
			}
			visual.SetModel(strIt->c_str());
		}
	});

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
		for (size_t i=0; i<arr.Size(); ++i)
		{
			const char* str = arr[i].GetString();
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

	_visuals[name] = visual;
}

void VisualSystem::EnsureVisualExists(const std::string& name)
{
	auto it = _visuals.find(name);
	if (it == _visuals.end())
		_visuals[name] = Visual();
}

const Visual* VisualSystem::GetVisual(const char *name)
{
	if (!name || *name == '\0')
		return nullptr;
	_temp = name; // reuse the same std::string for search to avoid reallocation
	auto it = _visuals.find(_temp);
	if (it != _visuals.end())
		return &it->second;
	return nullptr;
}

const Visual* VisualSystem::ProvideDefaultVisual(const char *name, const Visual &visual, bool doPrecache)
{
	_temp = name;
	auto it = _visuals.find(_temp);
	if (it != _visuals.end())
	{
		Visual& existing = it->second;
		existing.CompleteFrom(visual);

		if (doPrecache)
			existing.DoPrecache();

		return &existing;
	}
	else
	{
		auto inserted = _visuals.insert(std::make_pair(_temp, visual));
		if (inserted.second)
		{
			Visual* insertedVisual = &inserted.first->second;
			if (doPrecache)
				insertedVisual->DoPrecache();
			return insertedVisual;
		}
		// Should never get here: if it already existed it should have used the first if branch.
		return nullptr;
	}
}

static void PrintRange(const char* name, FloatRange range)
{
	if (range.max <= range.min)
	{
		LOG("%s: %g. ", name, range.min);
	}
	else
	{
		LOG("%s: %g-%g. ", name, range.min, range.max);
	}
}

void VisualSystem::DumpVisualImpl(const char *name, const Visual &visual) const
{
	LOG("%s:\n", name);

	LOG("Model/Sprite: \"%s\"\n", visual.model ? visual.model : "");

	LOG("Rendermode: %s. Color: (%d, %d, %d). Alpha: %d. Renderfx: %s. ",
		  RenderModeToString(visual.rendermode),
		  visual.rendercolor.r, visual.rendercolor.g, visual.rendercolor.b,
		  visual.renderamt,
		  RenderFxToString(visual.renderfx));

	PrintRange("Scale", visual.scale);
	PrintRange("Framerate", visual.framerate);

	if (visual.HasDefined(Visual::BEAMWIDTH_DEFINED))
	{
		LOG("Beam width: %d. Beam noise: %d. Beam scoll rate: %d. ", visual.beamWidth, visual.beamNoise, visual.beamScrollRate);
	}

	if (visual.HasDefined(Visual::LIFE_DEFINED))
	{
		PrintRange("Life", visual.life);
	}

	if (visual.HasDefined(Visual::RADIUS_DEFINED))
	{
		if (visual.radius.max <= visual.radius.min)
		{
			LOG("Radius: %d. ", visual.radius.min);
		}
		else
		{
			LOG("Radius: %d-%d. ", visual.radius.min, visual.radius.max);
		}
	}

	if (visual.HasDefined(Visual::DECAY_DEFINED))
	{
		LOG("Decay: %g. ", visual.decay);
	}

	if (visual.HasDefined(Visual::BEAMFLAGS_DEFINED))
	{
		const int beamFlags = visual.beamFlags;
		LOG("Beam flags: ");
		if (FBitSet(beamFlags, BEAM_FSINE))
			LOG("Sine; ");
		if (FBitSet(beamFlags, BEAM_FSOLID))
			LOG("Solid; ");
		if (FBitSet(beamFlags, BEAM_FSHADEIN))
			LOG("Shadein; ");
		if (FBitSet(beamFlags, BEAM_FSHADEOUT))
			LOG("Shadeout; ");
	}

	LOG("\n\n");
}

void VisualSystem::DumpVisuals() const
{
	for (const auto& p : _visuals)
	{
		DumpVisualImpl(p.first.c_str(),  p.second);
	}
}

void VisualSystem::DumpVisual(const char *name) const
{
	std::string temp = name;
	if (temp[temp.size()-1] == '.' || temp[temp.size()-1] == '#')
	{
		bool foundSomething = false;
		for (const auto& p : _visuals)
		{
			if (strnicmp(p.first.c_str(), temp.c_str(), temp.size()) == 0)
			{
				foundSomething = true;
				DumpVisualImpl(p.first.c_str(),  p.second);
			}
		}
		if (foundSomething)
			return;
	}
	else
	{
		auto it = _visuals.find(temp);
		if (it != _visuals.end())
		{
			DumpVisualImpl(name, it->second);
			return;
		}
	}
	LOG("Couldn't find a visual for %s\n", name);
}

VisualSystem g_VisualSystem;
