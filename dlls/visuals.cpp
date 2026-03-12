#include "visuals.h"
#include "customentity.h"
#include "error_collector.h"
#include "logger.h"
#include "util_shared.h"

using namespace rapidjson;

const char* visualsSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"$ref": "definitions.json#/visual_object"
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
	if (ShouldCompleteFrom(visual, WAVE_DEFINED))
	{
		SetWaveType(visual.waveType);
	}
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
		AddVisualFromJsonValue(name, value);
	}

	return true;
}

void VisualSystem::AddVisualFromJsonValue(const char *name, const Value &value)
{
	auto makeConstantString = [this](const char* modelStr) {
		std::string str = modelStr;
		auto strIt = _modelStringSet.find(str);
		if (strIt == _modelStringSet.end())
		{
			auto p = _modelStringSet.insert(str);
			strIt = p.first;
		}
		return strIt->c_str();
	};

	_visuals[name] = ParseVisualFromJSON(value, name, makeConstantString);
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

	if (visual.HasDefined(Visual::WAVE_DEFINED))
	{
		const int waveType = visual.waveType;
		LOG("Beam Wave Type: ");
		if (waveType == Visual::WAVETYPE_TORUS)
		{
			LOG("Torus");
		}
		else if (waveType == Visual::WAVETYPE_DISK)
		{
			LOG("Disk");
		}
		else
		{
			LOG("Cylinder");
		}
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
