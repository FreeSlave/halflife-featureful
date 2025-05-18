#include "logger.h"
#include "string_utils.h"
#include "json_utils.h"

#include "followers.h"

using namespace rapidjson;

const char followersSchema[] = R"(
{
	"type": "object",
	"properties": {
		"fast_recruit_monsters": {
			"type": "array",
			"items": {
				"type": "string",
				"pattern": "^monster_.*$"
			},
			"uniqueItems": true
		},
		"fast_recruit_range": {
			"type": "number",
			"exclusiveMinimum": 0
		}
	},
	"additionalProperties": false
}
)";

const char* FollowersDescription::Schema() const
{
	return followersSchema;
}

bool FollowersDescription::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	auto monstersIt = document.FindMember("fast_recruit_monsters");
	if (monstersIt != document.MemberEnd())
	{
		const Value& a = monstersIt->value;
		for (auto it = a.Begin(); it != a.End(); ++it)
		{
			const char* recruitName = it->GetString();
			LOG("Registered recruit: %s\n", recruitName);
			fastRecruitMonsters.push_back(std::string(recruitName));
		}
	}

	UpdatePropertyFromJson(fastRecruitRange, document, "fast_recruit_range");

	return true;
}

FollowersDescription g_FollowersDescription;
