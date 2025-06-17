#include "message_strings.h"

const char* messageStringSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"type": "string"
	}
}
)";

const char* MessageStrings::Schema() const
{
	return messageStringSchema;
}

bool MessageStrings::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	for (auto it = document.MemberBegin(); it != document.MemberEnd(); ++it)
	{
		const char* id = it->name.GetString();
		const char* text = it->value.GetString();
		SetText(id, text);
	}
	return true;
}

const char* MessageStrings::GetText(const char *id) const
{
	const auto& it =_messages.find(id);
	if (it != _messages.end())
	{
		return it->second.c_str();
	}
	return nullptr;
}

void MessageStrings::SetText(const char *id, const char *text)
{
	_messages[id] = text;
}
