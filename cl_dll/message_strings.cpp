#include "message_strings.h"
#include "logger.h"
#include "file_utils.h"
#include "text_utils.h"

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

void MessageStrings::ReadLegacyJournalFiles()
{
	ParseLegacyJournalFile("maps/objectives.txt", "objective");
	ParseLegacyJournalFile("maps/thoughts.txt", "thought");
	ParseLegacyJournalFile("maps/hints.txt", "hint");
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

#define QUEST_NAME_SIZE 24

void MessageStrings::ParseQuestTexts(char* pfile, int length, const char* objectType, const char* fileName)
{
	int currentTokenStart = 0;
	int i = 0;
	while ( i<length )
	{
		if (pfile[i] == ' ' || pfile[i] == '\r' || pfile[i] == '\n')
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			currentTokenStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);
			int tokenLength = i-currentTokenStart;
			if (!tokenLength || tokenLength >= QUEST_NAME_SIZE)
			{
				LOG_ERROR("invalid objective name length! Max is %d\n", QUEST_NAME_SIZE-1);
				ConsumeLine(pfile, i, length);
				continue;
			}

			char name[QUEST_NAME_SIZE];
			char message[512];

			strncpy(name, pfile + currentTokenStart, tokenLength);
			name[tokenLength] = '\0';

			SkipSpacesAndTabs(pfile, i, length);
			currentTokenStart = i;
			ConsumeLine(pfile, i, length);

			tokenLength = i-currentTokenStart;

			if (!tokenLength || tokenLength >= sizeof(message))
			{
				LOG_ERROR("Invalid %s description length for %s! Max is %d\n", objectType, name, sizeof(message)-1);
				continue;
			}

			strncpy(message, pfile + currentTokenStart, tokenLength);
			message[tokenLength] = '\0';

			const char* existingText = GetText(name);
			if (existingText)
			{
				LOG_ERROR("%s: message %s is already registered!\n", fileName, name);
			}
			else
			{
				SetText(name, message);
			}
		}
	}
}

bool MessageStrings::ParseLegacyJournalFile(const char* fileName, const char* objectType)
{
	int length = 0;
	char* pfile = ReadFileContents(fileName, length);

	if (!pfile)
	{
		LOG_ERROR("Couldn't open file %s.\n", fileName);
		return false;
	}

	LOG("Parsing %s\n", fileName);
	ParseQuestTexts(pfile, length, objectType, fileName);
	FreeFileContents(pfile);
	return true;
}
