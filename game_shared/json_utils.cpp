#include "file_utils.h"
#include "json_utils.h"
#include "logger.h"
#include "const_sound.h"
#include "safe_snprintf.h"

#include "color_utils.h"
#include "parsetext.h"
#include "error_collector.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/schema.h"
#include "rapidjson/error/en.h"

#include "json_schemas.h"

using namespace rapidjson;

static void CalculateLineAndColumnFromOffset(const char* pMemFile, size_t offset, size_t& line, size_t& column)
{
	const char* cur = pMemFile;
	line = 1;
	column = 0;
	size_t i = 0;
	bool nextLine = false;
	while (cur && *cur && i <= offset)
	{
		if (nextLine)
		{
			nextLine = false;
			line++;
			column = 0;
		}
		if (*cur == '\n')
		{
			nextLine = true;
		}
		++column;
		++cur;
		++i;
	}
}

static void ReportParseErrors(const char* fileName, ParseResult& parseResult, const char *pMemFile)
{
	size_t errorLine, errorColumn;
	CalculateLineAndColumnFromOffset(pMemFile, parseResult.Offset(), errorLine, errorColumn);
	g_errorCollector.AddFormattedError("%s: JSON parse error: %s (Line %zu, column %zu)", fileName, GetParseError_En(parseResult.Code()), errorLine, errorColumn);
}

class GeneralDefinitionsProvider : public IRemoteSchemaDocumentProvider
{
public:
	GeneralDefinitionsProvider(const Document& schemaDocument): _schema(schemaDocument) {}
	const SchemaDocument* GetRemoteDocument(const char* uri, SizeType length) {
		return &_schema;
	}
private:
	SchemaDocument _schema;
};

class DefinitionsProvider : public IRemoteSchemaDocumentProvider
{
public:
	DefinitionsProvider(const Document& schemaDocument, const Document& weaponSchemaDocument): _generalProvider(schemaDocument), _weaponSchema(weaponSchemaDocument, 0, 0, &_generalProvider) {}
	const SchemaDocument* GetRemoteDocument(const char* uri, SizeType length) {
		if (uri && *uri && strncmp(uri, "weapons.json", length) == 0)
		{
			return &_weaponSchema;
		}
		return _generalProvider.GetRemoteDocument(uri, length);
	}
private:
	GeneralDefinitionsProvider _generalProvider;
	SchemaDocument _weaponSchema;
};

IRemoteSchemaDocumentProvider* GetDefinitionsProvider()
{
	static std::unique_ptr<DefinitionsProvider> provider;
	static bool tried = false;

	if (tried)
		return provider.get();

	tried = true;

	if (!provider)
	{
		Document definitionsSchemaDocument;
		definitionsSchemaDocument.Parse(json_schemas::definitions);
		ParseResult parseResult = definitionsSchemaDocument;
		if (!parseResult)
		{
			ReportParseErrors("definitions", parseResult, json_schemas::definitions);
			return nullptr;
		}

		Document weaponsSchemaDocument;
		weaponsSchemaDocument.Parse(json_schemas::weapons);
		parseResult = weaponsSchemaDocument;

		if (!parseResult)
		{
			ReportParseErrors("weapons", parseResult, json_schemas::weapons);
			return nullptr;
		}

		provider = std::unique_ptr<DefinitionsProvider>(new DefinitionsProvider(definitionsSchemaDocument, weaponsSchemaDocument));
	}
	return provider.get();
}

bool ReadJsonDocumentWithSchema(Document &document, const char *pMemFile, int fileSize, const char *schemaText, const char* fileName)
{
	if (!fileName)
		fileName = "";

	Document schemaDocument;
	schemaDocument.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(schemaText);
	ParseResult parseResult = schemaDocument;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}

	IRemoteSchemaDocumentProvider* provider = GetDefinitionsProvider();
	if (!provider)
		return false;
	SchemaDocument schema(schemaDocument, 0, 0, provider);

	document.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(pMemFile, fileSize);
	parseResult = document;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}

	SchemaValidator validator(schema);
	if (!document.Accept(validator))
	{
		Pointer schemaPointer = validator.GetInvalidSchemaPointer();
		StringBuffer schemaPathBuffer;
		schemaPointer.Stringify(schemaPathBuffer);

		Pointer docPointer = validator.GetInvalidDocumentPointer();
		StringBuffer docPathBuffer;
		docPointer.Stringify(docPathBuffer);

		StringBuffer badValueBuffer;
		Value *badVal = GetValueByPointer(document, docPointer);
		if (badVal)
		{
			Writer<StringBuffer> writer(badValueBuffer);
			badVal->Accept(writer);
		}

		StringBuffer schemaPartBuffer;
		const char* invalidKeyword = validator.GetInvalidSchemaKeyword();
		auto& errorVal = validator.GetError();
		if (errorVal.HasMember(invalidKeyword))
		{
			Writer<StringBuffer> writer(schemaPartBuffer);
			errorVal[invalidKeyword].Accept(writer);
		}

		const char* keyword = validator.GetInvalidSchemaKeyword();

		char buf[1028];
		if (strcmp(keyword, "additionalProperties") == 0)
		{
			safe_snprintf(buf, sizeof(buf), "%s: unknown property \"%s\" is prohibited\n", fileName, docPathBuffer.GetString());
		}
		else
		{
			safe_snprintf(buf, sizeof(buf), "%s: property \"%s\" : %s doesn't match the constraint '%s' in '%s': %s\n",
				fileName,
				docPathBuffer.GetString(),
				badValueBuffer.GetString(),
				keyword,
				schemaPathBuffer.GetString(),
				schemaPartBuffer.GetString());
		}

		g_errorCollector.AddError(buf);

		return false;
	}
	return true;
}

bool ReadJsonDocumentWithSchemaFromFile(Document &document, const char *fileName, const char *schemaText)
{
	int fileSize;
	char *pMemFile = nullptr;
	pMemFile = ReadFileContents(fileName, fileSize);
	if (!pMemFile)
		return false;

	LOG("Parsing %s\n", fileName);

	const bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, schemaText, fileName);
	FreeFileContents(pMemFile);
	return success;
}

bool UpdatePropertyFromJson(std::string& str, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.IsNull())
		{
			str.clear();
			return true;
		}
		else if (it->value.IsString())
		{
			str = it->value.GetString();
			return true;
		}
		else
			return false;
	}
	return false;
}

bool UpdatePropertyFromJson(short& i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = static_cast<short>(it->value.GetInt());
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(int& i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetInt();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(unsigned int& i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetUint();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(std::int64_t & i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetInt64();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(std::uint64_t & i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetUint64();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(float& f, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		f = it->value.GetFloat();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(bool& b, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		b = it->value.GetBool();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(char& c, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.GetStringLength() == 1)
		{
			c = *it->value.GetString();
			return true;
		}
	}
	return false;
}

bool UpdatePropertyFromJson(Color3& color, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.IsString())
		{
			const char* colorStr = it->value.GetString();
			int packedColor;
			if (ParseColor(colorStr, packedColor)) {
				UnpackRGB(color.r, color.g, color.b, packedColor);
				return true;
			}
		}
		else if (it->value.IsArray())
		{
			Value::ConstArray arr = it->value.GetArray();
			color.r = arr[0].GetInt();
			color.g = arr[1].GetInt();
			color.b = arr[2].GetInt();
			return true;
		}
	}
	return false;
}

FloatRange FloatRangeFromJSON(const rapidjson::Value& value)
{
	FloatRange floatRange;
	if (value.IsNumber())
	{
		floatRange.min = value.GetFloat();
		floatRange.max = floatRange.min;
	}
	else if (value.IsObject())
	{
		auto minIt = value.FindMember("min");
		auto maxIt = value.FindMember("max");
		if (minIt != value.MemberEnd())
		{
			if (minIt->value.IsFloat())
				floatRange.min = minIt->value.GetFloat();
		}
		if (maxIt != value.MemberEnd())
		{
			if (maxIt->value.IsFloat())
				floatRange.max = maxIt->value.GetFloat();
		}
	}
	else if (value.IsString())
	{
		ParseFloatRange(value.GetString(), floatRange);
	}
	else if (value.IsArray())
	{
		Value::ConstArray arr = value.GetArray();
		floatRange.min = arr[0].GetFloat();
		floatRange.max = arr[1].GetFloat();
	}

	if (floatRange.min > floatRange.max) {
		floatRange.min = floatRange.max;
	}
	return floatRange;
}

IntRange IntRangeFromJSON(const rapidjson::Value& value)
{
	IntRange intRange;
	if (value.IsInt())
	{
		intRange.min = value.GetInt();
		intRange.max = intRange.min;
	}
	else if (value.IsObject())
	{
		auto minIt = value.FindMember("min");
		auto maxIt = value.FindMember("max");
		if (minIt != value.MemberEnd())
		{
			if (minIt->value.IsInt())
				intRange.min = minIt->value.GetInt();
		}
		if (maxIt != value.MemberEnd())
		{
			if (maxIt->value.IsInt())
				intRange.max = maxIt->value.GetInt();
		}
	}
	else if (value.IsString())
	{
		ParseIntRange(value.GetString(), intRange);
	}
	else if (value.IsArray())
	{
		Value::ConstArray arr = value.GetArray();
		intRange.min = arr[0].GetInt();
		intRange.max = arr[1].GetInt();
	}

	if (intRange.min > intRange.max) {
		intRange.min = intRange.max;
	}
	return intRange;
}

bool UpdatePropertyFromJson(FloatRange& floatRange, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		floatRange = FloatRangeFromJSON(it->value);
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(IntRange& intRange, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		const Value& value = it->value;
		if (value.IsInt())
		{
			intRange.min = value.GetInt();
			intRange.max = intRange.min;
		}
		else if (value.IsObject())
		{
			auto minIt = value.FindMember("min");
			auto maxIt = value.FindMember("max");
			if (minIt != value.MemberEnd())
			{
				if (minIt->value.IsInt())
					intRange.min = minIt->value.GetInt();
			}
			if (maxIt != value.MemberEnd())
			{
				if (maxIt->value.IsInt())
					intRange.max = maxIt->value.GetInt();
			}
		}
		else if (value.IsString())
		{
			const char* str = value.GetString();
			const char* found = strchr(str, ',');
			intRange.min = atoi(str);
			if (found) {
				found++;
				intRange.max = atoi(found);
			} else {
				intRange.max = intRange.min;
			}
		}
		else if (value.IsArray())
		{
			Value::ConstArray arr = value.GetArray();
			intRange.min = arr[0].GetInt();
			intRange.max = arr[1].GetInt();
		}

		if (intRange.min > intRange.max) {
			intRange.min = intRange.max;
		}
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(Vector& vector, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value::ConstArray arr = it->value.GetArray();
		vector.x = arr[0].GetFloat();
		vector.y = arr[1].GetFloat();
		vector.z = arr[2].GetFloat();

		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(tribool& b, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		b = it->value.GetBool();
		return true;
	}
	return false;
}

static bool ParseAttenuation(const char* str, float& attenuation)
{
	constexpr std::pair<const char*, float> attenuations[] = {
		{"norm", ATTN_NORM},
		{"idle", ATTN_IDLE},
		{"static", ATTN_STATIC},
		{"none", ATTN_NONE},
	};

	for (auto& p : attenuations)
	{
		if (stricmp(str, p.first) == 0)
		{
			attenuation = p.second;
			return true;
		}
	}
	return false;
}

bool UpdateAttenuationFromJson(float& attn, const Value& jsonValue)
{
	if (jsonValue.IsString())
	{
		return ParseAttenuation(jsonValue.GetString(), attn);
	}
	else if (jsonValue.IsNumber())
	{
		attn = jsonValue.GetFloat();
		return true;
	}
	return false;
}

void UpdatePlayerShake(PlayerShake& shake, const Value& value)
{
	if (value.IsNull())
	{
		shake = PlayerShake();
	}
	else if (value.IsObject())
	{
		UpdatePropertyFromJson(shake.radius, value, "radius");
		UpdatePropertyFromJson(shake.amplitude, value, "amplitude");
		UpdatePropertyFromJson(shake.duration, value, "duration");
		UpdatePropertyFromJson(shake.frequency, value, "frequency");
	}
}
