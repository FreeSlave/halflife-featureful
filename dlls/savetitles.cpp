
#include <cstring>
#include <string>
#include <vector>
#include <utility>

#include "util.h"
#include "parsetext.h"
#include "savetitles.h"

struct TitleComment
{
	const char *mapname;
	const char *titlename;
};

TitleComment gTitleComments[] =
{
	// default Half-Life map titles
	// ordering is important
	// strings hw.so| grep T0A0TITLE -B 50 -A 150
	{ "T0A0", "#T0A0TITLE" },
	{ "C0A0", "#C0A0TITLE" },
	{ "C1A0", "#C0A1TITLE" },
	{ "C1A1", "#C1A1TITLE" },
	{ "C1A2", "#C1A2TITLE" },
	{ "C1A3", "#C1A3TITLE" },
	{ "C1A4", "#C1A4TITLE" },
	{ "C2A1", "#C2A1TITLE" },
	{ "C2A2", "#C2A2TITLE" },
	{ "C2A3", "#C2A3TITLE" },
	{ "C2A4D", "#C2A4TITLE2" },
	{ "C2A4E", "#C2A4TITLE2" },
	{ "C2A4F", "#C2A4TITLE2" },
	{ "C2A4G", "#C2A4TITLE2" },
	{ "C2A4", "#C2A4TITLE1" },
	{ "C2A5", "#C2A5TITLE" },
	{ "C3A1", "#C3A1TITLE" },
	{ "C3A2", "#C3A2TITLE" },
	{ "C4A1A", "#C4A1ATITLE" },
	{ "C4A1B", "#C4A1ATITLE" },
	{ "C4A1C", "#C4A1ATITLE" },
	{ "C4A1D", "#C4A1ATITLE" },
	{ "C4A1E", "#C4A1ATITLE" },
	{ "C4A1", "#C4A1TITLE" },
	{ "C4A2", "#C4A2TITLE" },
	{ "C4A3", "#C4A3TITLE" },
	{ "C5A1", "#C5TITLE" },
	{ "OFBOOT", "#OF_BOOT0TITLE" },
	{ "OF0A", "#OF1A1TITLE" },
	{ "OF1A1", "#OF1A3TITLE" },
	{ "OF1A2", "#OF1A3TITLE" },
	{ "OF1A3", "#OF1A3TITLE" },
	{ "OF1A4", "#OF1A3TITLE" },
	{ "OF1A", "#OF1A5TITLE" },
	{ "OF2A1", "#OF2A1TITLE" },
	{ "OF2A2", "#OF2A1TITLE" },
	{ "OF2A3", "#OF2A1TITLE" },
	{ "OF2A", "#OF2A4TITLE" },
	{ "OF3A1", "#OF3A1TITLE" },
	{ "OF3A2", "#OF3A1TITLE" },
	{ "OF3A", "#OF3A3TITLE" },
	{ "OF4A1", "#OF4A1TITLE" },
	{ "OF4A2", "#OF4A1TITLE" },
	{ "OF4A3", "#OF4A1TITLE" },
	{ "OF4A", "#OF4A4TITLE" },
	{ "OF5A", "#OF5A1TITLE" },
	{ "OF6A1", "#OF6A1TITLE" },
	{ "OF6A2", "#OF6A1TITLE" },
	{ "OF6A3", "#OF6A1TITLE" },
	{ "OF6A4b", "#OF6A4TITLE" },
	{ "OF6A4", "#OF6A4TITLE" },
	{ "OF6A5", "#OF6A4TITLE" },
	{ "OF6A", "#OF6A4TITLE" },
	{ "OF7A", "#OF7A0TITLE" },
	{ "ba_tram", "#BA_TRAMTITLE" },
	{ "ba_security", "#BA_SECURITYTITLE" },
	{ "ba_main", "#BA_SECURITYTITLE" },
	{ "ba_elevator", "#BA_SECURITYTITLE" },
	{ "ba_canal", "#BA_CANALSTITLE" },
	{ "ba_yard", "#BA_YARDTITLE" },
	{ "ba_xen", "#BA_XENTITLE" },
	{ "ba_hazard", "#BA_HAZARD" },
	{ "ba_power", "#BA_POWERTITLE" },
	{ "ba_teleport1", "#BA_POWERTITLE" },
	{ "ba_teleport", "#BA_TELEPORTTITLE" },
	{ "ba_outro", "#BA_OUTRO" },
};

std::vector<std::pair<std::string, std::string> > g_SaveTitles;

const char* GetSaveTitleForMap(const char* mapname)
{
	for(const auto& title : g_SaveTitles)
	{
		// compare if strings are equal at beginning
		size_t len = title.first.size();
		if( !strnicmp( mapname, title.first.c_str(), len ))
		{
			return title.second.c_str();
		}
	}
	for(size_t i = 0; i < ARRAYSIZE( gTitleComments ); i++)
	{
		// compare if strings are equal at beginning
		size_t len = strlen( gTitleComments[i].mapname );
		if( !strnicmp( mapname, gTitleComments[i].mapname, len ))
		{
			return gTitleComments[i].titlename;
		}
	}
	return nullptr;
}

void ReadSaveTitles()
{
	const char* fileName = "save_titles.txt";
	int filePos = 0;
	int fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe(fileName, &fileSize);
	if (!pMemFile)
		return;

	char buffer[512] = { 0 };
	int lineNum = 0;
	while(memfgets(pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1)) {
		lineNum++;
		int i = 0;
		SkipSpaceCharacters(buffer, i, sizeof(buffer));
		if (buffer[i] == '\0') // it's an empty line, skip
			continue;
		if (buffer[i] == '/') // it's a comment, skip
			continue;
		int keyNameStart;
		int keyNameEnd;
		if (!ConsumePossiblyQuotedString(buffer, i, sizeof(buffer), keyNameStart, keyNameEnd))
		{
			ALERT(at_error, "%s: error parsing the mapname prefix on line %d\n", fileName, lineNum);
			break;
		}
		SkipSpaceCharacters(buffer, i, sizeof(buffer));
		int valueStart;
		int valueEnd;
		if (!ConsumePossiblyQuotedString(buffer, i, sizeof(buffer), valueStart, valueEnd))
		{
			ALERT(at_error, "%s: error parsing the title on line %d\n", fileName, lineNum);
			break;
		}
		g_SaveTitles.push_back(std::make_pair(std::string(buffer+keyNameStart, buffer+keyNameEnd), std::string(buffer+valueStart, buffer+valueEnd)));
	}
	for (auto it = g_SaveTitles.begin(); it != g_SaveTitles.end(); ++it)
	{
		ALERT(at_console, "Save title: %s : %s\n", it->first.c_str(), it->second.c_str());
	}
	g_engfuncs.pfnFreeFile(pMemFile);
}
