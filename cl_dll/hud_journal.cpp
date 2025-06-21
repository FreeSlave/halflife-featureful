#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "parsemsg.h"
#include "string_utils.h"
#include "parsetext.h"
#include "text_utils.h"
#include "arraysize.h"

#include <algorithm>

DECLARE_MESSAGE( m_Journal, Journal )

int CHudJournal::Init()
{
	journalInit = false;
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( Journal );

	InitHUDData();

	return 1;
}

int CHudJournal::VidInit()
{
	notifications.clear();
	if (gHUD.IsDeveloperModeOn())
	{
		journalInit = false;
	}
	if (!journalInit)
	{
		InitJournal();
		journalInit = true;
	}

	for (JournalSection& section : sections)
	{
		section.messageId.clear();
		section.messageText = nullptr;
		section.lineOffsets.clear();
	}

	return 1;
}

void CHudJournal::InitHUDData()
{
	m_iFlags |= HUD_ACTIVE;
}

#define JOURNAL_MARGIN (ScreenWidth / 18 * 2)
#define JOURNAL_MARGIN_Y (ScreenHeight / 15 * 2)
#define JOURNAL_PADDING (ScreenWidth / 18)
#define JOURNAL_PADDING_Y (ScreenHeight / 15)
#define JOURNAL_MAX_XPOS (ScreenWidth - JOURNAL_PADDING - JOURNAL_MARGIN)
#define JOURNAL_WIDTH (ScreenWidth - JOURNAL_MARGIN * 2)
#define JOURNAL_TEXT_WIDTH (JOURNAL_WIDTH - JOURNAL_PADDING * 2)

void CHudJournal::Update(float flTime, float flTimeDelta)
{
	for (auto& notification : notifications)
	{
		if (notification.fadeTime <= 0)
		{
			notification.alpha -= Q_max(1, flTimeDelta * 60);
		}
		else
		{
			notification.fadeTime -= flTimeDelta;
		}
	}
	while (!notifications.empty() && notifications[0].alpha <= 0)
	{
		notifications.erase(notifications.begin());
	}
}

static void FillCharBufWithNumberSuffix(char* buf, int size, int count)
{
	safe_snprintf(buf, size, "x%d", count);
}

bool CHudJournal::ShouldDraw()
{
	if (sections.empty())
		return false;

	for (const auto& section : sections)
	{
		if (section.alwaysShow)
			return true;
		if (section.messageText)
			return true;
		if (section.showInventory)
		{
			for (const inventory_t& item : gHUD.m_StatusIcons.m_InventoryList)
			{
				if (item.CanRender())
					return true;
			}
		}
	}
	return false;
}

int CHudJournal::Draw(float flTime)
{
	if (!ShouldDraw())
		return 1;

	int r, g, b;
	const int currentHudColor = gHUD.HUDTextColor();
	UnpackRGB(r, g, b, currentHudColor);

	if (!m_iShowscoresHeld)
	{
		int ytext = JOURNAL_PADDING_Y * 3;
		const int lineHeight = CHud::UtfText::LineHeight();
		const int afterIndent = Q_max(lineHeight / 4, 1);

		for (auto& notification : notifications)
		{
			CHud::UtfText::DrawString(JOURNAL_PADDING, ytext, JOURNAL_MAX_XPOS, notification.message.c_str(), r, g, b);
			ytext += lineHeight + afterIndent;
		}
		return 1;
	}
	notifications.clear();

	const int xrect = JOURNAL_MARGIN;
	const int width = JOURNAL_WIDTH;
	const int yrect = JOURNAL_MARGIN_Y;
	const int height = ScreenHeight - JOURNAL_MARGIN_Y * 2;

	RectangleRenderProperties rectProps;
	rectProps.frameColor = Color3(r, g, b);
	gHUD.DrawDarkRectangle(xrect, yrect, width, height, rectProps);

	const int paddingX = JOURNAL_PADDING;
	const int paddingY = JOURNAL_PADDING_Y;

	const int xmax = JOURNAL_MAX_XPOS;

	const int xtext = xrect + paddingX;
	int ytext = yrect + paddingY;

	const int lineHeight = CHud::UtfText::LineHeight();
	const int afterHeaderIndent = Q_max(lineHeight / 4, 1);
	const int afterSection = Q_max(1, (lineHeight + afterHeaderIndent) / 2);

	HudSpriteRenderer& renderer = CHud::Renderer();

	for (size_t i = 0; i < sections.size(); ++i)
	{
		const JournalSection& section = sections[i];

		const bool hasHeader = section.headerMessage && *section.headerMessage;
		const bool hasMessage = section.messageText && *section.messageText;
		const bool isLastSection = sections.size() > 1 && i == sections.size() - 1;

		if (hasMessage || section.alwaysShow || section.showInventory)
		{
			int maxItemSpriteHeight = 0;

			if (section.showInventory)
			{
				for (const inventory_t& item : gHUD.m_StatusIcons.m_InventoryList)
				{
					if (item.CanRender())
					{
						maxItemSpriteHeight = Q_max(maxItemSpriteHeight, item.rc.bottom - item.rc.top);
					}
				}
			}

			if (isLastSection && section.showInventory)
			{
				int sectionHeight = 0;
				if (hasHeader)
				{
					sectionHeight += lineHeight + afterHeaderIndent;
				}
				if (hasMessage)
				{
					sectionHeight += lineHeight * section.lineOffsets.size();
				}
				sectionHeight += renderer.ScaleScreen(maxItemSpriteHeight);
				const int newY = ScreenHeight - yrect - paddingY - sectionHeight;
				ytext = Q_max(ytext, newY);
			}

			bool shouldShow = true;
			if (section.showInventory)
			{
				shouldShow = hasMessage || maxItemSpriteHeight > 0;
			}

			if (shouldShow)
			{
				if (hasHeader)
				{
					CHud::UtfText::DrawString(xtext, ytext, xmax, section.headerMessage, r, g, b);
					ytext += lineHeight;
					ytext += afterHeaderIndent;
				}
				if (hasMessage)
				{
					for (const auto& lineOffset : section.lineOffsets)
					{
						CHud::UtfText::DrawString(xtext, ytext, xmax, section.messageText + lineOffset.first, r, g, b, lineOffset.second - lineOffset.first);
						ytext += lineHeight;
					}
				}
				if (section.showInventory)
				{
					int xsprites = xtext;

					for (const inventory_t& item : gHUD.m_StatusIcons.m_InventoryList)
					{
						// TODO: deduplicate with inventory item rendering in status icons
						if (item.CanRender())
						{
							int r = item.r;
							int g = item.g;
							int b = item.b;
							if (r == 0 && g == 0 && b == 0)
								UnpackRGB(r, g, b, gHUD.HUDColor());

							int rText = r;
							int gText = g;
							int bText = b;
							ScaleColors(rText, gText, bText, gHUD.m_inventorySpec.TextAlpha());

							int alpha = item.a;
							if (alpha <= 0)
							{
								alpha = gHUD.m_inventorySpec.DefaultSpriteAlpha();
							}
							ScaleColors(r, g, b, alpha);

							renderer.SPR_DrawAdditive(item.spr, r, g, b, renderer.UnscaleScreen(xsprites), renderer.UnscaleScreen(ytext), &item.rc);
							xsprites += renderer.ScaleScreen(item.rc.right - item.rc.left);

							if (item.count > 1)
							{
								const int textLineHeight = CHud::AdditiveText::LineHeight();
								const int height = item.rc.bottom - item.rc.top;
								const int heightInScreenSpace = renderer.ScaleScreen(height);
								const int textYShift = heightInScreenSpace - Q_min(textLineHeight, heightInScreenSpace);

								char buf[24];
								FillCharBufWithNumberSuffix(buf, sizeof(buf), item.count);
								CHud::AdditiveText::DrawString(xsprites, ytext + textYShift, ScreenWidth, buf, rText, gText, bText);
								xsprites += CHud::AdditiveText::LineWidth(buf);
							}

							xsprites += 2;
						}
					}
					ytext += maxItemSpriteHeight;
				}
				ytext += afterSection;
			}
		}
	}

	return 1;
}

void CHudJournal::UserCmd_ShowJournal()
{
	m_iShowscoresHeld = true;
}

void CHudJournal::UserCmd_HideJournal()
{
	m_iShowscoresHeld = false;
}

int CHudJournal::MsgFunc_Journal(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	int notify = READ_BYTE();
	const char* sectionName = READ_STRING();

	JournalSection* section = nullptr;
	for (unsigned int i=0; i<sections.size(); ++i)
	{
		if (stricmp(sections[i].sectionName, sectionName) == 0)
		{
			section = &sections[i];
			break;
		}
	}

	if (!section)
	{
		gEngfuncs.Con_Printf("Unknown journal section %s!\n", sectionName);
		return 1;
	}

	const char* messageId = READ_STRING();
	const char* message = gHUD.m_messageStrings.GetText(messageId);

	if (notify // don't notify if requested not to
		&& *messageId // don't notify on record removal
		&& section->messageId != messageId // don't notify if record hasn't changed
	)
	{
		if (section->notificationMessage && *section->notificationMessage)
		{
			if (section->notificationMessageRight && *section->notificationMessageRight)
			{
				char buf[512];
				const char* binding = gEngfuncs.Key_LookupBinding("showscores");
				safe_snprintf(buf, sizeof(buf), "%s%s%s", section->notificationMessage, (binding && *binding) ? binding : "UNBOUND", section->notificationMessageRight);
				buf[sizeof(buf) - 1] = '\0';
				AddNotification(buf);
			}
			else
			{
				AddNotification(section->notificationMessage);
			}
		}
		if (section->notificationSound && *section->notificationSound)
		{
			Vector view_ofs;
			cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);
			gEngfuncs.pEventAPI->EV_PlaySound(pthisplayer->index, pthisplayer->origin + view_ofs, CHAN_STATIC, section->notificationSound, 1.0f, ATTN_NORM, 0, PITCH_NORM);
		}
	}

	if (*messageId)
	{
		section->messageId = messageId;
		if (message)
			section->messageText = message;
		else
			section->messageText = section->messageId.c_str();
	}
	else
	{
		section->messageId.clear();
		section->messageText = nullptr;
	}

	section->UpdateLineOffsets();

	return 1;
}

void CHudJournal::InitJournal()
{
	sections.clear();
	hasInventorySection = false;

	auto p = gHUD.m_journalConfig.SectionsRange();
	for (auto it = p.first; it != p.second; ++it)
	{
		JournalSection section;
		section.sectionName = it->name.c_str();
		section.headerMessage = nullptr;
		if (!it->header.empty())
		{
			section.headerMessage = gHUD.m_messageStrings.GetText(it->header.c_str());
			if (!section.headerMessage)
				section.headerMessage = it->header.c_str(); // for a fallback: display message id as text
		}

		section.showInventory = it->showInventory;
		section.alwaysShow = it->alwaysShow;
		section.notificationMessage = gHUD.m_messageStrings.GetText(it->notification.c_str());
		section.notificationMessageRight = gHUD.m_messageStrings.GetText(it->notificationRight.c_str());
		section.notificationSound = it->notificationSound.c_str();
		sections.push_back(section);

		if (section.showInventory)
			hasInventorySection = true;
	}
}

void CHudJournal::AddNotification(const char *message)
{
	if (!message || !*message)
		return;

	Notification notification;
	notification.message = message;
	notification.fadeTime = 3.0f;
	notification.alpha = 255;
	notifications.push_back(std::move(notification));
}

void CHudJournal::JournalSection::UpdateLineOffsets()
{
	if (!messageText || !*messageText)
		return;

	lineOffsets.clear();

	const char* str = messageText;
	const int maxwidth = JOURNAL_TEXT_WIDTH;

	if (CHud::ShouldUseConsoleFont())
	{
		WordBoundaries boundaries = SplitIntoWordBoundaries(str);

		unsigned int startWordIndex = 0;
		for (unsigned int j=0; j<boundaries.size();)
		{
			const int width = CHud::UtfText::LineWidth(str + boundaries[startWordIndex].wordStart, boundaries[j].wordEnd - boundaries[startWordIndex].wordStart);
			if (width > maxwidth) {
				if (j == startWordIndex) {
					lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[startWordIndex].wordEnd));
					startWordIndex = ++j;
				} else {
					lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[j-1].wordEnd));
					startWordIndex = j;
				}
			} else {
				if (j == boundaries.size() - 1) {
					lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[j].wordEnd));
				}

				++j;
			}

			if (lineOffsets.size() == lineOffsets.capacity())
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
				lineOffsets.push_back(std::make_pair(currentLine - messageText, str - messageText));
				break;
			}
			lineWidth += gHUD.m_scrinfo.charWidths[(unsigned char)*str];
			if (*str == ' ' || *str == '\n')
			{
				lastSpace = str;
			}
			if (lineWidth > maxwidth)
			{
				str = lastSpace;
			}
			if (*str == '\n' || lineWidth > maxwidth)
			{
				lineOffsets.push_back(std::make_pair(currentLine - messageText, str - messageText));
				lineWidth = 0;
				currentLine = str + 1;
				if (lineOffsets.size() == lineOffsets.capacity())
					break;
			}
			str++;
		}
		while(true);
	}
}
