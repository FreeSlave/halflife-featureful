#pragma once
#ifndef FOLLOWERS_H
#define FOLLOWERS_H

#include <vector>
#include <string>
#include "json_config.h"

class FollowersDescription : public JSONConfig
{
public:
	FollowersDescription(): fastRecruitRange(500.0f) {}
	float FastRecruitRange() const {
		return fastRecruitRange;
	}
	std::vector<std::string>::const_iterator RecruitsBegin() const {
		return fastRecruitMonsters.begin();
	}
	std::vector<std::string>::const_iterator RecruitsEnd() const {
		return fastRecruitMonsters.cend();
	}
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
private:
	std::vector<std::string> fastRecruitMonsters;
	float fastRecruitRange;
};

extern FollowersDescription g_FollowersDescription;

#endif
