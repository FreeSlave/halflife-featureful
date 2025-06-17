#include <string>
#include <map>
#include "fixed_string.h"
#include "icase_compare.h"
#include "json_config.h"

class MessageStrings : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	const char* GetText(const char* id) const;
	void SetText(const char* id, const char* text);
private:
	std::map<fixed_string<64>, std::string, CaseInsensitiveCompare> _messages;
};
