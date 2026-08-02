#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "kbutton.h"
#include "keydefs.h"
#include "string_utils.h"
#include "window_geometry.h"

DECLARE_MESSAGE( m_MessageBox, MessageBox )

int CHudMessageBox::Init()
{
	HOOK_MESSAGE(MessageBox);
	gHUD.AddHudElem(this);
	return 1;
}

int CHudMessageBox::VidInit()
{
	messageBoxes.clear();
	m_iFlags = 0;

	WindowGeometry geometry = GetWindowGeometry();
	const int height = geometry.height * ScreenHeight;
	const int paddingY = geometry.paddingVertical * height;
	const int LineHeight = CHud::UtfText::LineHeight();

	m_iMaxRowsPerWindow = (height - paddingY * 2) / LineHeight;
	m_iMaxRowsPerWindow = Q_max(1, m_iMaxRowsPerWindow);

	return 1;
}

int CHudMessageBox::Draw(float flTime)
{
	if (messageBoxes.empty())
		return 1;

	if (gHUD.TopLevelWindowIsActive())
		return 1;

	int r, g, b;
	UnpackRGB(r, g, b, gHUD.HUDTextColor());

	WindowGeometry geometry = GetWindowGeometry();

	const int xrect = (1.0f - geometry.width) / 2 * ScreenWidth;
	const int width = geometry.width * ScreenWidth;
	const int yrect = (1.0f - geometry.height) / 2 * ScreenHeight;
	const int height = geometry.height * ScreenHeight;

	const int LineHeight = CHud::UtfText::LineHeight();

	RectangleRenderProperties rectProps;
	rectProps.frameColor = Color3(r, g, b);
	gHUD.DrawDarkRectangle(xrect, yrect, width, height, rectProps);

	const int paddingX = geometry.paddingHorizontal * width;
	const int paddingY = geometry.paddingVertical * height;

	const int xmax = ((1.0f + geometry.width) / 2 - geometry.paddingHorizontal * geometry.width) * ScreenWidth;

	const int xtext = xrect + paddingX;
	int ytext = yrect + paddingY;

	const MessageBoxData& messageBoxData = messageBoxes.back();

	const bool scrollEnabled = messageBoxData.lineOffsets.size() > m_iMaxRowsPerWindow;

	int startRow = scrollEnabled ? messageBoxData.scrollLines : 0;
	int endRow = scrollEnabled ? messageBoxData.scrollLines + m_iMaxRowsPerWindow : messageBoxData.lineOffsets.size();
	endRow = Q_min(endRow, messageBoxData.lineOffsets.size());

	if (startRow > 0)
	{
		CHud::UtfText::DrawString(xtext, ytext - LineHeight / 3 * 2, xmax, "^^^^^^", 255, 255, 255);
	}

	for (int i=startRow; i<endRow; ++i)
	{
		const auto& lineOffset = messageBoxData.lineOffsets[i];
		CHud::UtfText::DrawString(xtext, ytext, xmax, messageBoxData.message.c_str() + lineOffset.first, r, g, b, lineOffset.second - lineOffset.first);
		ytext += LineHeight;
	}

	if (endRow < messageBoxData.lineOffsets.size())
	{
		CHud::UtfText::DrawString(xtext, ytext - LineHeight / 3, xmax, "......", 255, 255, 255);
	}

	return 1;
}

WindowGeometry CHudMessageBox::GetWindowGeometry()
{
	return WindowGeometry();
}

int CHudMessageBox::MsgFunc_MessageBox(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	bool operation = READ_BYTE() != 0;
	int messageBoxId = READ_LONG();

	if (operation == 0)
	{
		for (auto it = messageBoxes.begin(); it != messageBoxes.end(); ++it)
		{
			if (it->messageBoxId == messageBoxId)
			{
				messageBoxes.erase(it);
				break;
			}
		}

		return 1;
	}

	MessageBoxData data;

	data.messageBoxId = messageBoxId;
	const char* messageId = READ_STRING();

	if (!messageId || !*messageId)
	{
		gEngfuncs.Con_Printf("Got MessageBox message, but the message id is empty!\n");
		return 1;
	}

	data.message = gHUD.m_messageStrings.GetText(messageId, messageId);

	const WindowGeometry geometry = GetWindowGeometry();

	const int maxwidth = (geometry.width - geometry.paddingHorizontal * geometry.width * 2) * ScreenWidth;

	data.lineOffsets = CHud::UtfText::CalcLineOffsets(data.message.c_str(), maxwidth);
	data.showTime = gEngfuncs.GetClientTime();

	messageBoxes.push_back(data);

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

bool CHudMessageBox::HandleClientInput()
{
	if (messageBoxes.empty())
		return false;

	if (messageBoxes.back().showTime + 0.1f <= gEngfuncs.GetClientTime())
	{
		char buf[64];
		sprintf(buf, "close_messagebox %d\n", messageBoxes.back().messageBoxId);
		gEngfuncs.pfnClientCmd(buf);

		messageBoxes.pop_back();

		return true;
	}
	return false;
}

bool CHudMessageBox::HandleKeyDown(int keynum)
{
	if (messageBoxes.empty())
		return false;

	switch (keynum) {
	case K_MWHEELDOWN:
		ScrollDown();
		return true;
	case K_MWHEELUP:
		ScrollUp();
		return true;
	case K_PGDN:
		PageDown();
		return true;
	case K_PGUP:
		PageUp();
		return true;
	default:
		return false;
	}
}

bool CHudMessageBox::HasActiveMessageBoxes()
{
	return !messageBoxes.empty();
}

void CHudMessageBox::ScrollDown()
{
	if (messageBoxes.empty())
		return;

	MessageBoxData& messageBoxData = messageBoxes.back();

	if (messageBoxData.lineOffsets.empty())
		return;

	messageBoxData.scrollLines++;

	const int maxScroll = messageBoxData.lineOffsets.size() - m_iMaxRowsPerWindow + 1;

	if (messageBoxData.scrollLines > maxScroll)
		messageBoxData.scrollLines = maxScroll;
}

void CHudMessageBox::ScrollUp()
{
	if (messageBoxes.empty())
		return;

	MessageBoxData& messageBoxData = messageBoxes.back();

	if (messageBoxData.lineOffsets.empty())
		return;

	messageBoxData.scrollLines--;
	if (messageBoxData.scrollLines < 0)
		messageBoxData.scrollLines = 0;
}

void CHudMessageBox::PageDown()
{
	if (messageBoxes.empty())
		return;

	MessageBoxData& messageBoxData = messageBoxes.back();

	if (messageBoxData.lineOffsets.empty())
		return;

	messageBoxData.scrollLines += m_iMaxRowsPerWindow;

	const int maxScroll = messageBoxData.lineOffsets.size() - m_iMaxRowsPerWindow + 1;

	if (messageBoxData.scrollLines > maxScroll)
		messageBoxData.scrollLines = maxScroll;
}

void CHudMessageBox::PageUp()
{
	if (messageBoxes.empty())
		return;

	MessageBoxData& messageBoxData = messageBoxes.back();

	if (messageBoxData.lineOffsets.empty())
		return;

	messageBoxData.scrollLines -= m_iMaxRowsPerWindow;
	if (messageBoxData.scrollLines < 0)
		messageBoxData.scrollLines = 0;
}
