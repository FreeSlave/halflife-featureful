#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "parsetext.h"
#include "arraysize.h"
#include "text_utils.h"

#include <algorithm>

extern cvar_t *cl_subtitles;

Caption_t::Caption_t(): profile(NULL), delay(0.0f), duration(0.0f)
{
	memset(name, 0, sizeof(name));
}

Caption_t::Caption_t(const char *captionName): profile(NULL), delay(0.0f), duration(0.0f)
{
	strncpyEnsureTermination(name, captionName);
}

DECLARE_MESSAGE( m_Caption, Caption )

DECLARE_COMMAND( m_Caption, DumpCaptions )

int CHudCaption::Init()
{
	captionsInit = false;
	memset(subtitles, 0, sizeof(subtitles));
	defaultProfile.r = 255;
	defaultProfile.g = 255;
	defaultProfile.b = 255;
	defaultProfile.firstLetter = '_';
	defaultProfile.secondLetter = '_';
	defaultCaption.profile = &defaultProfile;

	gHUD.AddHudElem( this );
	HOOK_MESSAGE(Caption);
	Reset();

	HOOK_COMMAND( "dump_captions", DumpCaptions );

	return 1;
}

int CHudCaption::VidInit()
{
	std::vector<Caption_t> oldCaptions;
	if (gHUD.IsDeveloperModeOn())
	{
		profiles.clear();
		oldCaptions = std::move(captions);
		captionsInit = false;
	}
	if (!captionsInit)
	{
		ParseCaptionsProfilesFile();
		ParseCaptionsFile();
		captionsInit = true;

		for (int i=0; i<sub_count; ++i)
		{
			if (subtitles[i].caption)
			{
				const Caption_t* caption = CaptionLookup(subtitles[i].caption->name);
				subtitles[i].caption = caption ? caption : &defaultCaption;
				if (!caption)
					subtitles[i].timeLeft = 0.0f;
			}
		}
	}
	m_hVoiceIcon = SPR_Load("sprites/voiceicon.spr");
	voiceIconWidth = SPR_Width(m_hVoiceIcon, 0);
	voiceIconHeight = SPR_Height(m_hVoiceIcon, 0);
	RecalculateLineOffsets();

	return 1;
}

void CHudCaption::Reset()
{
	m_iFlags = 0;
	memset(subtitles, 0, sizeof(subtitles));
	sub_count = 0;
}

#define SUB_START_XPOS (ScreenWidth / 20)
#define SUB_MAX_XPOS (ScreenWidth - ScreenWidth/20)
#define SUB_BORDER_LENGTH (ScreenWidth/160)

int CHudCaption::MsgFunc_Caption(const char *pszName, int iSize, void *pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	Subtitle_t sub;
	memset(&sub, 0, sizeof(sub));

	float holdTime = READ_BYTE();
	int radio = READ_BYTE();

	const char* captionName = READ_STRING();
	const Caption_t* caption = CaptionLookup(captionName);

	if (!caption)
		return 1;

	sub.caption = caption;

	if (caption->profile)
	{
		sub.r = caption->profile->r;
		sub.g = caption->profile->g;
		sub.b = caption->profile->b;
	}
	else
	{
		sub.r = defaultProfile.r;
		sub.g = defaultProfile.g;
		sub.b = defaultProfile.b;
	}

	sub.radio = radio != 0;

	if (caption->duration > 0)
	{
		sub.timeLeft = caption->duration;
	}
	else
	{
		if (holdTime <= 0)
		{
			int perceivedLength = 0;
			for (auto it = caption->message.begin(); it != caption->message.end(); ++it)
			{
				if (*it >= 0 && *it <= 127)
					perceivedLength += 2;
				else
					perceivedLength ++;
			}
			holdTime = 2 + perceivedLength/32.0f;
		}
		sub.timeLeft = holdTime;
	}
	sub.timeBeforeStart = caption->delay;
	gEngfuncs.Con_DPrintf("New caption: Hold time: %g. Current time: %g. Delay: %g\n", sub.timeLeft, gHUD.m_flTime, caption->delay);

	CalculateLineOffsets(sub);

	AddSubtitle(sub);

	return 1;
}

void CHudCaption::AddSubtitle(const Subtitle_t &sub)
{
	if ((unsigned)sub_count < ARRAYSIZE(subtitles))
	{
		subtitles[sub_count] = sub;
		sub_count++;
	}
}

void CHudCaption::CalculateLineOffsets(Subtitle_t &sub)
{
	const char* start = sub.caption->message.c_str();
	const char* str = start;
	const int xmax = SUB_MAX_XPOS - SUB_START_XPOS;

	if (CHud::ShouldUseConsoleFont())
	{
		WordBoundaries boundaries = SplitIntoWordBoundaries(sub.caption->message);

		unsigned int startWordIndex = 0;
		for (unsigned int j=0; j<boundaries.size();)
		{
			const int width = CHud::UtfText::LineWidth(str + boundaries[startWordIndex].wordStart, boundaries[j].wordEnd - boundaries[startWordIndex].wordStart);
			if (width > xmax) {
				if (j == startWordIndex) {
					sub.lineOffsets[sub.lineCount] = boundaries[startWordIndex].wordStart;
					sub.lineEndOffsets[sub.lineCount] = boundaries[startWordIndex].wordEnd;
					sub.lineCount++;

					startWordIndex = ++j;
				} else {
					sub.lineOffsets[sub.lineCount] = boundaries[startWordIndex].wordStart;
					sub.lineEndOffsets[sub.lineCount] = boundaries[j-1].wordEnd;
					sub.lineCount++;

					startWordIndex = j;
				}
			} else {
				if (j == boundaries.size() - 1) {
					sub.lineOffsets[sub.lineCount] = boundaries[startWordIndex].wordStart;
					sub.lineEndOffsets[sub.lineCount] = boundaries[j].wordEnd;
					sub.lineCount++;
				}

				++j;
			}

			if (sub.lineCount >= SUB_MAX_LINES)
				break;
		}
	}
	else
	{
		int lineWidth = 0;
		const char* currentLine = str;
		const char* lastSpace = str;
		do
		{
			if (*str == '\0')
			{
				sub.lineOffsets[sub.lineCount] = currentLine - start;
				sub.lineEndOffsets[sub.lineCount] = str - start;
				sub.lineCount++;
				break;
			}
			lineWidth += gHUD.m_scrinfo.charWidths[(unsigned char)*str];
			if (*str == ' ' || *str == '\n')
			{
				lastSpace = str;
			}
			if (lineWidth > xmax)
			{
				str = lastSpace;
			}
			if (*str == '\n' || lineWidth > xmax)
			{
				sub.lineOffsets[sub.lineCount] = currentLine - start;
				sub.lineEndOffsets[sub.lineCount] = str - start;
				sub.lineCount++;
				lineWidth = 0;
				currentLine = str + 1;
				if (sub.lineCount >= SUB_MAX_LINES)
					break;
			}
			str++;
		}
		while(true);
	}
}

void CHudCaption::RecalculateLineOffsets()
{
	for (int i=0; i<sub_count; ++i)
	{
		subtitles[i].lineCount = 0;
		memset(subtitles[i].lineOffsets, 0, sizeof(subtitles[i].lineOffsets));
		memset(subtitles[i].lineEndOffsets, 0, sizeof(subtitles[i].lineEndOffsets));
		CalculateLineOffsets(subtitles[i]);
	}
}

void CHudCaption::Update(float flTime, float flTimeDelta)
{
	if (!sub_count)
		return;

	int i;
	for (i=0; i<sub_count; ++i)
	{
		if(subtitles[i].timeLeft <= 0)
		{
			for (int j=i; j<sub_count-1; ++j)
			{
				subtitles[j] = subtitles[j+1];
			}
			memset(&subtitles[sub_count-1], 0, sizeof(subtitles[0]));
			sub_count--;
			break;
		}
	}

	for (i=0; i<sub_count; ++i)
	{
		if (subtitles[i].timeBeforeStart <= 0.0f) {
			subtitles[i].timeLeft -= flTimeDelta;
		} else {
			subtitles[i].timeBeforeStart -= flTimeDelta;
			if (subtitles[i].timeBeforeStart <= 0.0f) {
				subtitles[i].timeLeft += subtitles[i].timeBeforeStart;
			} else {
				continue;
			}
		}
	}
}

int CHudCaption::Draw(float flTime)
{
	if (!sub_count)
		return 0;

	const int lineHeight = CHud::UtfText::LineHeight();
	const int distanceBetweenSubs = Q_max(lineHeight / 5, 1);

	const int xpos = SUB_START_XPOS;
	const int xmax = SUB_MAX_XPOS;
	int ypos = ScreenHeight - CHud::Renderer().ScaleScreen(gHUD.m_iFontHeight + gHUD.m_iFontHeight/2) - lineHeight - SUB_BORDER_LENGTH;

	int i, j;

	int overallLineCount = 0;
	int maxLineWidth = 0;
	for (i=0; i<sub_count; ++i)
	{
		if (subtitles[i].timeBeforeStart > 0)
			continue;

		overallLineCount += subtitles[i].lineCount;
		for (j=0; j<subtitles[i].lineCount; ++j)
		{
			int lineWidth = 0;

			if (CHud::ShouldUseConsoleFont())
			{
				lineWidth += CHud::UtfText::LineWidth(subtitles[i].caption->message.c_str() + subtitles[i].lineOffsets[j], subtitles[i].lineEndOffsets[j] - subtitles[i].lineOffsets[j]);
			}
			else
			{
				const char* str = subtitles[i].caption->message.c_str() + subtitles[i].lineOffsets[j];
				while( str != subtitles[i].caption->message.c_str() + subtitles[i].lineEndOffsets[j] )
				{
					lineWidth += gHUD.m_scrinfo.charWidths[(unsigned char)*str];
					str++;
				}
			}

			if (lineWidth > maxLineWidth)
				maxLineWidth = lineWidth;
		}
	}

	if (overallLineCount == 0)
		return 0;

	ypos -= overallLineCount * lineHeight + (sub_count-1) * distanceBetweenSubs;

	if (cl_subtitles->value == 0)
		return 0;

	const int width = maxLineWidth + SUB_BORDER_LENGTH*2;
	const int height = overallLineCount * lineHeight + (sub_count-1) * distanceBetweenSubs + SUB_BORDER_LENGTH*2;
	gEngfuncs.pfnFillRGBABlend( xpos - SUB_BORDER_LENGTH, ypos - SUB_BORDER_LENGTH, width, height, 0, 0, 0, 255 * 0.6 );

	for (i=0; i<sub_count; ++i)
	{
		const Subtitle_t& sub = subtitles[i];
		for (j=0; j<sub.lineCount; ++j)
		{
			if (sub.timeBeforeStart > 0.0f) {
				continue;
			}

			if (j == 0 && m_hVoiceIcon != 0 && sub.radio)
			{
				SPR_Set( m_hVoiceIcon, sub.r, sub.g, sub.b );
				SPR_DrawAdditive( 0, xpos-SUB_BORDER_LENGTH-voiceIconWidth-Q_max(voiceIconWidth/8, 1), ypos + lineHeight/2 - voiceIconWidth/2, NULL );
			}

			CHud::UtfText::DrawString( xpos, ypos, xmax, sub.caption->message.c_str() + sub.lineOffsets[j], sub.r, sub.g, sub.b, sub.lineEndOffsets[j] - sub.lineOffsets[j] );
			ypos += lineHeight;
		}
		ypos += distanceBetweenSubs;
	}
	return 1;
}

void CHudCaption::UserCmd_DumpCaptions()
{
	gEngfuncs.Con_DPrintf("Captions. HUD time: %f\n", gHUD.m_flTime);
	for (int i=0; i<sub_count; ++i)
	{
		const Subtitle_t& subtitle = subtitles[i];
		if (subtitle.caption)
		{
			gEngfuncs.Con_DPrintf("Caption %d: `%s`. Line count: %d. Time left: %f\n",
							i+1, subtitle.caption->message.c_str(), subtitle.lineCount, subtitle.timeLeft);
		}
	}
}

#define MAX_CAPTION_NAME_LENGTH 31

static bool IsLatinLowerCase(char c)
{
	return c >= 'a' && c <= 'z';
}

static void ParseCaptionColor(const char* pfile, int& i, int length, CaptionProfile_t& profile)
{
	int rgb[3];
	for (int j=0; j<3; ++j)
	{
		SkipSpaces(pfile, i, length);
		rgb[j] = atoi(pfile + i);
		ConsumeNonSpaceCharacters(pfile, i, length);
	}
	profile.r = rgb[0];
	profile.g = rgb[1];
	profile.b = rgb[2];
}

static void ReportParsedCaptionProfile(const CaptionProfile_t& profile)
{
	gEngfuncs.Con_DPrintf("Parsed a caption profile. ID: %c%c. Color: (%d, %d, %d)\n", profile.firstLetter, profile.secondLetter, profile.r, profile.g, profile.b);
}

bool CHudCaption::ParseCaptionsProfilesFile()
{
	const char* fileName = "sound/captions_profiles.txt";
	int length = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &length );

	if( !pfile )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s.\n", fileName );
		return false;
	}

	int currentIdStart = 0;
	int i = 0;
	while ( i<length )
	{
		if (IsSpaceCharacter(pfile[i]))
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			currentIdStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);
			int tokenLength = i-currentIdStart;

			if (tokenLength == 2)
			{
				char firstLetter = *(pfile + currentIdStart);
				char secondLetter = *(pfile + currentIdStart + 1);
				if (IsLatinLowerCase(firstLetter) && IsLatinLowerCase(secondLetter))
				{
					CaptionProfile_t* existingProfile = CaptionProfileLookup(firstLetter, secondLetter);
					if (existingProfile)
					{
						gEngfuncs.Con_Printf("Multiple definitions of caption profile with ID '%c%c'! Skipping.\n", firstLetter, secondLetter);
						ConsumeLine(pfile, i, length);
						continue;
					}

					CaptionProfile_t profile;
					profile.firstLetter = firstLetter;
					profile.secondLetter = secondLetter;
					ParseCaptionColor(pfile, i, length, profile);
					profiles.push_back(profile);

					ReportParsedCaptionProfile(profile);
				}
				else
				{
					gEngfuncs.Con_Printf("Both characters in caption profile ID must be lowercase latin characters!\n");
					break;
				}
			}
			else
			{
				gEngfuncs.Con_Printf("Caption profile ID must be 2 characters!\n");
				break;
			}
		}
	}
	gEngfuncs.COM_FreeFile(pfile);
	return true;
}

bool CHudCaption::ParseCaptionsFile()
{
	const char* fileName = "sound/captions.txt";
	int length = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &length );

	if( !pfile )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s.\n", fileName );
		return false;
	}

	char captionName[sizeof(captions[0].name)];

	int currentTokenStart = 0;
	int i = 0;
	while ( i<length )
	{
		if (IsSpaceCharacter(pfile[i]))
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			currentTokenStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);
			unsigned int tokenLength = i-currentTokenStart;
			if (!tokenLength || tokenLength >= sizeof(captions[0].name))
			{
				gEngfuncs.Con_Printf("invalid caption name length! Max is %d\n", sizeof(captions[0].name)-1);
				ConsumeLine(pfile, i, length);
				continue;
			}

			strncpy(captionName, pfile + currentTokenStart, tokenLength);
			captionName[tokenLength] = '\0';

			// This code is left for compatibility with existing mods. We should define caption profiles in captions_profiles.txt now instead!
			if (tokenLength == 2 && IsLatinLowerCase(captionName[0]) && IsLatinLowerCase(captionName[1]))
			{
				char firstLetter = captionName[0];
				char secondLetter = captionName[1];
				CaptionProfile_t* existingProfile = CaptionProfileLookup(firstLetter, secondLetter);
				if (existingProfile)
				{
					gEngfuncs.Con_Printf("Redefining caption profile with ID '%c%c'!.\n", firstLetter, secondLetter);
					ParseCaptionColor(pfile, i, length, *existingProfile);
					ReportParsedCaptionProfile(*existingProfile);
				}
				else
				{
					CaptionProfile_t profile;
					profile.firstLetter = firstLetter;
					profile.secondLetter = secondLetter;
					ParseCaptionColor(pfile, i, length, profile);
					profiles.push_back(profile);
					ReportParsedCaptionProfile(profile);
				}
			}
			//
			else
			{
				Caption_t caption(captionName);

				do {
					SkipSpaces(pfile, i, length);
					currentTokenStart = i;
					ConsumeNonSpaceCharacters(pfile, i, length);

					tokenLength = i-currentTokenStart;
				} while (ParseFloatParameter(pfile, currentTokenStart, tokenLength, caption));

				if (tokenLength != 2 || !IsLatinLowerCase(pfile[currentTokenStart]) || !IsLatinLowerCase(pfile[currentTokenStart+1]))
				{
					gEngfuncs.Con_Printf("invalid caption profile for \"%s\"! Must be 2 lowercase latin characters\n", caption.name);
					ConsumeLine(pfile, i, length);
					continue;
				}

				char firstLetter = pfile[currentTokenStart];
				char secondLetter = pfile[currentTokenStart+1];
				caption.profile = CaptionProfileLookup(firstLetter, secondLetter);

				if (!caption.profile)
				{
					gEngfuncs.Con_Printf("Could not find a caption profile '%c%c' for %s\n", firstLetter, secondLetter, caption.name);
				}

				SkipSpaces(pfile, i, length);
				currentTokenStart = i;
				ConsumeLine(pfile, i, length);

				tokenLength = i-currentTokenStart;

				caption.message = std::string(pfile + currentTokenStart, tokenLength);
				captions.push_back(caption);
				//gEngfuncs.Con_DPrintf("Parsed a caption. Name: %s. Profile: %c%c. Text: %s\n", caption.name, caption.profile->firstLetter, caption.profile->secondLetter, caption.message);
			}
		}
	}
	std::sort(captions.begin(), captions.end(), [](const Caption_t& a, const Caption_t& b) {
		return strcmp(a.name, b.name) < 0;
	});

	gEngfuncs.COM_FreeFile(pfile);
	return true;
}

bool CHudCaption::ParseFloatParameter(char* pfile, int& currentTokenStart, unsigned int& tokenLength, Caption_t& caption)
{
	if (tokenLength <= 0)
		return false;
	char c = pfile[currentTokenStart];

	bool expectDuration = false;
	bool expectDelay = false;
	if (c == '!')
	{
		if (tokenLength <= 1)
			return false;
		currentTokenStart++;
		tokenLength--;
		expectDuration = true;
	}
	else if (c == '^')
	{
		if (tokenLength <= 1)
			return false;
		currentTokenStart++;
		tokenLength--;
		expectDelay = true;
	}
	// deprecated way to set delay, used in Induction, left for compatibility
	else if (c >= '1' && c <= '9')
	{
		expectDelay = true;
	}

	if (!expectDelay && !expectDuration) {
		return false;
	}

	char numbuf[8];
	strncpyEnsureTermination(numbuf, pfile + currentTokenStart, Q_min(tokenLength+1, sizeof(numbuf)));

	float value = atof(numbuf);
	if (expectDuration)
	{
		caption.duration = value;
	}
	else if (expectDelay)
	{
		caption.delay = value;
	}
	return true;
}

struct CaptionCompare
{
	bool operator ()(const Caption_t& lhs, const char* rhs)
	{
		return stricmp(lhs.name, rhs) < 0;
	}
	bool operator ()(const char* lhs, const Caption_t& rhs)
	{
		return stricmp(lhs, rhs.name) < 0;
	}
};

const Caption_t* CHudCaption::CaptionLookup(const char *name)
{
	auto result = std::equal_range(captions.begin(), captions.end(), name, CaptionCompare());
	if (result.first != result.second)
		return &(*result.first);
	return nullptr;
}

CaptionProfile_t* CHudCaption::CaptionProfileLookup(char firstLetter, char secondLetter)
{
	for (CaptionProfile_t& profile : profiles)
	{
		if (profile.firstLetter == firstLetter && profile.secondLetter == secondLetter)
		{
			return &profile;
		}
	}
	return nullptr;
}
