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

	_visuals[name] = ParseVisualFromJSON(value, makeConstantString);
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

static void PrintRange(const char* name, IntRange range)
{
	if (range.IsProperRange())
	{
		LOG("%s: %d-%d. ", name, range.min, range.max);
	}
	else
	{
		LOG("%s: %d. ", name, range.min);
	}
}

static void PrintRange(const char* name, FloatRange range)
{
	if (range.IsProperRange())
	{
		LOG("%s: %g-%g. ", name, range.min, range.max);
	}
	else
	{
		LOG("%s: %g. ", name, range.min);
	}
}

void VisualSystem::DumpVisualImpl(const char *name, const Visual &visual) const
{
	LOG("%s:\n", name);

	LOG("Model/Sprite: \"%s\"\n", visual.model ? visual.model : "");

	LOG("Rendermode: %s. Color: (%d, %d, %d). Alpha: %d. Renderfx: %s. ",
		  RenderModeToString(visual.rendermode),
		  visual.rendercolor.r, visual.rendercolor.g, visual.rendercolor.b);

	PrintRange("Alpha", visual.renderamt);
	LOG("Renderfx: %s. ", RenderFxToString(visual.renderfx));

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
		PrintRange("Radius", visual.radius);
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
