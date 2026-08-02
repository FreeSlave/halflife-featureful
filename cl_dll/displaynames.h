#include <string>
#include <map>
#include "icase_compare.h"
#include "json_config.h"

class DisplayNames : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	const char* GetDisplayName(const char* name) const;
	void SetDisplayName(const char* name, const char* localized);
private:
	std::map<std::string, std::string, CaseInsensitiveCompare> _displayNames;
};
