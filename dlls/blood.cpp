#include <cstring>
#include <utility>
#include "blood_types.h"

int BloodTypeFromName(const char* name)
{
	static constexpr std::pair<const char*, int> pairs[] = {
		{"red", BLOOD_COLOR_RED},
		{"yellow", BLOOD_COLOR_YELLOW},
		{"no", DONT_BLEED},
		{"darkred", BLOOD_COLOR_DARKRED},
		{"brightyellow", BLOOD_COLOR_BRIGHTYELLOW},
		{"green", BLOOD_COLOR_BRIGHTGREEN},
		{"white", BLOOD_COLOR_WHITE},
		{"grey", BLOOD_COLOR_GREY},
		{"gray", BLOOD_COLOR_GREY},
		{"brown", BLOOD_COLOR_BROWN},
		{"olive", BLOOD_COLOR_OLIVE},
		{"greygreen", BLOOD_COLOR_GREYGREEN},
		{"graygreen", BLOOD_COLOR_GREYGREEN},
		{"orange", BLOOD_COLOR_ORANGE},
		{"goldish", BLOOD_COLOR_GOLDISH},
		{"peach", BLOOD_COLOR_PEACH},
		{"magenta", BLOOD_COLOR_MAGENTA},
		{"tan", BLOOD_COLOR_TAN},
		{"blue", BLOOD_COLOR_BLUE},
		{"darkblue", BLOOD_COLOR_DARKBLUE},
		{"bluish", BLOOD_COLOR_BLUISH},
	};

	for (auto p : pairs)
	{
		if (stricmp(name, p.first) == 0)
			return p.second;
	}
	return BLOOD_COLOR_INVALID;
}

int GetBloodStreamColor(int bloodColor)
{
	switch (bloodColor) {
	case BLOOD_COLOR_RED:
		return 70;
	case BLOOD_COLOR_GREY:
		return 2;
	}
	if (bloodColor <= 127)
	{
		int row = bloodColor / 16;
		int column = bloodColor % 16;
		if (column > 6)
		{
			return row * 16 + 6;
		}
	}
	return bloodColor;
}

bool IsReddishBlood(int bloodColor)
{
	return (bloodColor >= 247 && bloodColor <= 251) || (bloodColor >= 70 && bloodColor <= 79);
}

bool IsYellowishBlood(int bloodColor)
{
	return (bloodColor >= 192 && bloodColor <= 200) || (bloodColor >= 59 && bloodColor <= 63);
}
