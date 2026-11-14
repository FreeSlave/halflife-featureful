#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "logger.h"
#include "parsetext.h"

#include <algorithm>
#include <cctype>

DECLARE_MESSAGE( m_HudHint, HudHint )

#define HINT_DURATION 5.0f

int CHudHint::Init()
{
	gHUD.AddHudElem(this);
	HOOK_MESSAGE( HudHint );
	parsedKbAct = false;
	return 1;
}

int CHudHint::VidInit()
{
	hints.clear();

	if (!parsedKbAct)
	{
		ParseKbAct();
		parsedKbAct = true;
	}
	//for (const auto& p : kbActMap)
	//	gEngfuncs.Con_Printf("kb_act.lst: %s : %s\n", p.first.c_str(), p.second.c_str());

	return 1;
}

int CHudHint::Draw(float time)
{
	if (hints.empty())
		return 1;

	int r, g, b;
	UnpackRGB(r, g, b, gHUD.HUDTextColor());

	const int lineHeight = CHud::UtfText::LineHeight();
	const int padding = lineHeight * 3 / 4;
	const int margin = lineHeight / 2;
	int rectY = ScreenHeight / 2;

	for (const auto& hint : hints)
	{
		const int keyWidth = hint.key.empty() ? 0 : CHud::UtfText::LineWidth(hint.key.c_str());
		const int textWidth = CHud::UtfText::LineWidth(hint.text.c_str());
		const int separatorWidth = hint.key.empty() ? 0 : CHud::UtfText::WidestCharacterWidth() * 2;

		const int rectWidth = padding * 2 + keyWidth + separatorWidth + textWidth;
		const int rectHeight = padding * 2 + lineHeight;
		const int rectX = ScreenWidth - padding - rectWidth;

		gEngfuncs.pfnFillRGBABlend(rectX, rectY, rectWidth, rectHeight, 0, 0, 0, 120);

		const int textX = rectX + padding;
		const int textY = rectY + padding;

		if (!hint.key.empty())
			CHud::UtfText::DrawString(textX, textY, hint.key.c_str(), r, g, b);
		CHud::UtfText::DrawString(textX + keyWidth + separatorWidth, textY, hint.text.c_str(), r, g, b);

		rectY += rectHeight + margin;
	}

	return 1;
}

void CHudHint::Update(float flTime, float flTimeDelta)
{
	hints.erase(std::remove_if(hints.begin(), hints.end(), [flTime](const Hint& hint) { return hint.startTime + HINT_DURATION < flTime; }), hints.end());
}

int CHudHint::MsgFunc_HudHint(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const char* command = READ_STRING();
	const char* text = nullptr;

	Hint hint;
	hint.startTime = gEngfuncs.GetClientTime();

	if (*command != '\0')
	{
		const char* boundKey = gEngfuncs.Key_LookupBinding(command[0] == '+' ? command+1 : command);
		boundKey = (boundKey && *boundKey) ? boundKey : "UNBOUND";
\
		hint.key = boundKey;
		std::transform(hint.key.begin(), hint.key.end(), hint.key.begin(), [](unsigned char c) { return std::toupper(c); });

		auto it = kbActMap.find(command);

		text = READ_STRING();
		if (!*text)
		{
			if (it != kbActMap.end())
			{
				text = it->second.c_str();
			}
		}
	}
	else
	{
		text = READ_STRING();
	}

	if (*text)
	{
		if (*text != '#')
		{
			text = gHUD.m_messageStrings.GetText(text);
		}

		if (text && *text)
		{
			hint.text = text;
		}
	}

	if (!hint.text.empty())
	{
		m_iFlags |= HUD_ACTIVE;
		hints.push_back(std::move(hint));
	}

	return 1;
}

void CHudHint::ParseKbAct()
{
	const char* fileName = "gfx/shell/kb_act.lst";
	int fileSize;
	char *pfile = (char*)gEngfuncs.COM_LoadFile(fileName, 5, &fileSize);

	if (pfile)
	{
		gEngfuncs.Con_DPrintf("Parsing %s\n", fileName);

		int lineNum = 1;

		int i = 0;
		while (i < fileSize)
		{
			if (IsSpaceCharacter(pfile[i]))
			{
				++i;
				if (pfile[i] == '\n')
				{
					lineNum++;
				}
			}
			else
			{
				int commandStart;
				int commandEnd;
				if (!ConsumePossiblyQuotedString(pfile, i, fileSize, commandStart, commandEnd))
				{
					LOG_ERROR("%s: error parsing the command name at line %d\n", fileName, lineNum);
					ConsumeLine(pfile, i, fileSize);
					lineNum++;
					continue;
				}
				SkipSpacesAndTabs(pfile, i, fileSize);

				int textStart;
				int textEnd;
				if (!ConsumePossiblyQuotedString(pfile, i, fileSize, textStart, textEnd))
				{
					LOG_ERROR("%s: error parsing the command text at line %d\n", fileName, lineNum);
					ConsumeLine(pfile, i, fileSize);
					lineNum++;
					continue;
				}
				ConsumeLine(pfile, i, fileSize);
				lineNum++;

				std::string command = std::string(pfile+commandStart, pfile+commandEnd);
				if (command == "blank")
					continue;

				kbActMap.insert(std::make_pair(std::move(command), std::string(pfile+textStart, pfile+textEnd)));
			}
		}
		gEngfuncs.COM_FreeFile(pfile);
	}
}
