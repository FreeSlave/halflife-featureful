#pragma once
#ifndef JOURNAL_CONFIG
#define JOURNAL_CONFIG

#include <string>
#include <utility>
#include <vector>
#include "json_config.h"

class JournalConfig : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	struct Section
	{
		std::string name;
		std::string header;
		std::string notification;
		std::string notificationRight;
		std::string notificationSound;
		bool showInventory = false;
		bool alwaysShow = false;
	};
	std::pair<std::vector<Section>::const_iterator, std::vector<Section>::const_iterator> SectionsRange() const {
		return std::make_pair(sections.begin(), sections.end());
	}
	bool IsEmpy() const {
		return sections.empty();
	}
private:
	std::vector<Section> sections;
};

#endif
