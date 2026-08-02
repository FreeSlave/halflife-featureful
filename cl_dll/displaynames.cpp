#include "displaynames.h"

const char* displayNamesSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"type": "string"
	}
}
)";

const char* DisplayNames::Schema() const
{
	return displayNamesSchema;
}

bool DisplayNames::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	for (auto it = document.MemberBegin(); it != document.MemberEnd(); ++it)
	{
		const char* id = it->name.GetString();
		const char* text = it->value.GetString();
		SetDisplayName(id, text);
	}
	return true;
}

const char* DisplayNames::GetDisplayName(const char *name) const
{
	if (!name || !*name)
		return nullptr;
	const auto& it =_displayNames.find(name);
	if (it != _displayNames.end())
	{
		return it->second.c_str();
	}
	return nullptr;
}

void DisplayNames::SetDisplayName(const char *name, const char *localized)
{
	if (name && localized)
		_displayNames[name] = localized;
}
