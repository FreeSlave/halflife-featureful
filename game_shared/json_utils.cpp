#include "file_utils.h"
#include "json_utils.h"
#include "logger.h"
#include "const_sound.h"

#include "color_utils.h"
#include "parsetext.h"
#include "error_collector.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/schema.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;

constexpr const char definitions[] = R"(
{
	"alpha": {
		"type": "integer",
		"minimum": 0,
		"maximum": 255
	},
	"color": {
		"type": ["string", "array", "null"],
		"pattern": "^([0-9]{1,3}[ ]+[0-9]{1,3}[ ]+[0-9]{1,3})|((#|0x)[0-9a-fA-F]{6})$",
		"items": {
			"type": "integer",
			"minimum": 0,
			"maximum": 255
		},
		"minItems": 3,
		"maxItems": 3
	},
	"range": {
		"type": ["string", "object", "number", "array"],
		"pattern": "^[0-9]+(\\.[0-9]+)?(,[0-9]+(\\.[0-9]+)?)?$",
		"properties": {
			"min": {
				"type": "number"
			},
			"max": {
				"type": "number"
			}
		},
		"additionalProperties": false,
		"items": {
			"type": "number"
		},
		"minItems": 2,
		"maxItems": 2
	},
	"range_int": {
		"type": ["string", "object", "integer", "array"],
		"pattern": "^[0-9]+(,[0-9]+)?$",
		"properties": {
			"min": {
				"type": "integer"
			},
			"max": {
				"type": "integer"
			}
		},
		"additionalProperties": false,
		"items": {
			"type": "integer"
		},
		"minItems": 2,
		"maxItems": 2
	},
	"vector": {
		"type": ["array"],
		"items": {
			"type": "number"
		},
		"minItems": 3,
		"maxItems": 3
	},
	"attenuation": {
		"oneof": [
			{
				"enum": [
					"Norm",
					"norm",
					"Idle",
					"idle",
					"Static",
					"static",
					"None",
					"none"
				]
			},
			{
				"type": "number",
				"minimum": 0
			}
		]
	},
	"soundscript": {
		"type": ["object", "string"],
		"properties": {
			"waves": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"maxItems": 10
			},
			"channel": {
				"enum": [
					"Auto",
					"auto",
					"Weapon",
					"weapon",
					"Voice",
					"voice",
					"Item",
					"item",
					"Body",
					"body",
					"Static",
					"static"
				]
			},
			"volume": {
				"$ref": "#/range"
			},
			"attenuation": {
				"$ref": "#/attenuation"
			},
			"pitch": {
				"$ref": "#/range_int"
			}
		},
		"additionalProperties": false
	},
	"visual": {
		"type": ["object", "string"],
		"properties": {
			"model": {
				"type": "string"
			},
			"sprite": {
				"type": "string"
			},
			"rendermode": {
				"enum": [
					"Normal",
					"normal",
					"Color",
					"color",
					"Texture",
					"texture",
					"Glow",
					"glow",
					"Solid",
					"solid",
					"Additive",
					"additive"
				]
			},
			"color": {
				"$ref": "#/color"
			},
			"alpha": {
				"$ref": "#/alpha"
			},
			"renderfx": {
				"oneof": [
					{
						"enum": [
							"Normal",
							"normal",
							"Constant Glow",
							"constant glow",
							"Constant glow",
							"Distort",
							"distort",
							"Hologram",
							"hologram",
							"Glow Shell",
							"glow shell",
							"Glow shell"
						]
					},
					{
						"type": "integer",
						"minimum": 0,
						"maximum": 20
					}
				]
			},
			"scale": {
				"$ref": "#/range",
				"minimum": 0.0
			},
			"framerate": {
				"type": "number",
				"minimum": 0.0
			},
			"width": {
				"type": "integer",
				"minimum": 1
			},
			"noise": {
				"type": "integer"
			},
			"scrollrate": {
				"type": "integer"
			},
			"life": {
				"$ref": "#/range"
			},
			"radius": {
				"$ref": "#/range_int"
			},
			"beamflags": {
				"type": "array",
				"items": {
					"enum": [
						"Sine",
						"sine",
						"Solid",
						"solid",
						"Shadein",
						"shadein",
						"Shadeout",
						"shadeout"
					]
				}
			},
			"decay": {
				"type": "number"
			}
		},
		"additionalProperties": false
	}
}
)";

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

class DefinitionsProvider : public IRemoteSchemaDocumentProvider
{
public:
	DefinitionsProvider(const SchemaDocument* schema): _schema(schema) {}
	const SchemaDocument* GetRemoteDocument(const char* uri, SizeType length) {
		return _schema;
	}
private:
	const SchemaDocument* _schema;
};

bool ReadJsonDocumentWithSchema(Document &document, const char *pMemFile, int fileSize, const char *schemaText, const char* fileName)
{
	if (!fileName)
		fileName = "";

	Document definitionsSchemaDocument;
	definitionsSchemaDocument.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(definitions);
	ParseResult parseResult = definitionsSchemaDocument;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}
	SchemaDocument definitionsSchema(definitionsSchemaDocument);

	Document schemaDocument;
	schemaDocument.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(schemaText);
	parseResult = schemaDocument;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}

	DefinitionsProvider provider(&definitionsSchema);
	SchemaDocument schema(schemaDocument, 0, 0, &provider);

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
		Pointer schemaKeywordPointer = schemaPointer.Append(validator.GetInvalidSchemaKeyword());
		Value* schemaPartValue = GetValueByPointer(schemaDocument, schemaKeywordPointer);
		if (schemaPartValue)
		{
			Writer<StringBuffer> writer(schemaPartBuffer);
			schemaPartValue->Accept(writer);
		}

		char buf[1028];
		_snprintf(buf, sizeof(buf), "%s: value %s of property '%s' doesn't match the constraint '%s' in '%s': %s\n",
			fileName,
			badValueBuffer.GetString(),
			docPathBuffer.GetString(),
			validator.GetInvalidSchemaKeyword(),
			schemaPathBuffer.GetString(),
			schemaPartBuffer.GetString());
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

bool UpdatePropertyFromJson(std::string& str, Value& jsonValue, const char* key)
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

bool UpdatePropertyFromJson(int& i, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetInt();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(float& f, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		f = it->value.GetFloat();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(bool& b, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		b = it->value.GetBool();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(char& c, Value& jsonValue, const char* key)
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

bool UpdatePropertyFromJson(Color3& color, Value& jsonValue, const char* key)
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
			Value::Array arr = it->value.GetArray();
			color.r = arr[0].GetInt();
			color.g = arr[1].GetInt();
			color.b = arr[2].GetInt();
			return true;
		}
	}
	return false;
}

bool UpdatePropertyFromJson(FloatRange& floatRange, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value& value = it->value;
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
			const char* str = value.GetString();
			const char* found = strchr(str, ',');
			floatRange.min = atof(str);
			if (found) {
				found++;
				floatRange.max = atof(found);
			} else {
				floatRange.max = floatRange.min;
			}
		}
		else if (value.IsArray())
		{
			Value::Array arr = value.GetArray();
			floatRange.min = arr[0].GetFloat();
			floatRange.max = arr[1].GetFloat();
		}

		if (floatRange.min > floatRange.max) {
			floatRange.min = floatRange.max;
		}
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(IntRange& intRange, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value& value = it->value;
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
			Value::Array arr = value.GetArray();
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

bool UpdatePropertyFromJson(Vector& vector, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value::Array arr = it->value.GetArray();
		vector.x = arr[0].GetFloat();
		vector.y = arr[1].GetFloat();
		vector.z = arr[2].GetFloat();

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

bool UpdateAttenuationFromJson(float& attn, Value& jsonValue)
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
