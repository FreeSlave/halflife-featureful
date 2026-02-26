#include "message_strings.h"
#include "file_utils.h"
#include "json_utils.h"

const char* messageStringSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"type": ["string", "object"],
		"properties": {
			"file": {
				"type": "string",
				"minLength": 1
			}
		},
		"required": ["file"],
		"additionalProperties": false
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
		if (it->value.IsString())
		{
			const char* text = it->value.GetString();
			SetText(id, text);
		}
		else if (it->value.IsObject())
		{
			std::string fileName;
			UpdatePropertyFromJson(fileName, it->value, "file");
			int fileSize;
			const char* text = ReadFileContents(fileName.c_str(), fileSize);
			if (text)
				SetText(id, text);
		}
	}
	return true;
}

const char* MessageStrings::GetText(const char *id, const char* fallback) const
{
	if (!id || !*id)
		return nullptr;
	const auto& it =_messages.find(id);
	if (it != _messages.end())
	{
		return it->second.c_str();
	}
	return fallback;
}

void MessageStrings::SetText(const char *id, const char *text)
{
	if (id && text)
		_messages[id] = text;
}
