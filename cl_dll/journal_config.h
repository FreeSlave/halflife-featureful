#pragma once
#ifndef JOURNAL_CONFIG
#define JOURNAL_CONFIG

#include <string>
#include <utility>
#include <vector>
#include "json_config.h"
#include "window_geometry.h"
#include "optional.h"
#include "template_property_types.h"

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
	struct Position
	{
		float x;
		float y;
	};
	struct Render
	{
		optional<Color3> textColor;
		optional<Color3> notificationTextColor;
		optional<Color3> frameColor;
		Color3 backgroundColor{};
		int frameAlpha = 255;
		int backgroundAlpha = 160;
		bool frameBlend = false;
		bool backgroundBlend = true;
	};

	std::pair<std::vector<Section>::const_iterator, std::vector<Section>::const_iterator> SectionsRange() const {
		return std::make_pair(sections.begin(), sections.end());
	}
	bool IsEmpy() const {
		return sections.empty();
	}
	const WindowGeometry& GetWindowGeometry() const {
		return geometry;
	}
	const Position& NotificationPosition() const {
		return notificationPosition;
	}
	const Render& RenderProps() const {
		return render;
	}
private:
	std::vector<Section> sections;
	WindowGeometry geometry;
	Position notificationPosition{1.0f / 18, 1.0f / 5};
	Render render;
};

#endif
