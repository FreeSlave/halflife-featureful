#include "json_config.h"
#include "json_utils.h"

bool JSONConfig::ReadFromFile(const char *fileName)
{
	rapidjson::Document document;
	if (!ReadJsonDocumentWithSchemaFromFile(document, fileName, Schema()))
		return false;
	return ReadFromDocument(document, fileName);
}

bool JSONConfig::ReadFromContents(const char *contents, const char *fileName)
{
	rapidjson::Document document;
	if (!ReadJsonDocumentWithSchema(document, contents, strlen(contents), Schema(), fileName))
		return false;
	return ReadFromDocument(document, fileName);
}
