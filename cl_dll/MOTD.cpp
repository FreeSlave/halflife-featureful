/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// MOTD.cpp
//
// for displaying a server-sent message of the day
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "kbutton.h"
#include "string_utils.h"
#include "keydefs.h"

int CHudMOTD::Init()
{
	gHUD.AddHudElem( this );

	m_bShow = false;

	m_iFlags &= ~HUD_ACTIVE;  // start out inactive
	m_szMOTD[0] = 0;

	return 1;
}

int CHudMOTD::VidInit()
{
	// Load sprites here
	return 1;
}

void CHudMOTD::Reset()
{
	m_iFlags &= ~HUD_ACTIVE;  // start out inactive
	m_szMOTD[0] = 0;
	m_bShow = false;
	m_scrollLines = 0;
	m_lineOffsets.clear();
}

#define MIN_TEXT_XPOS (ScreenWidth / 64)

int CHudMOTD::Draw( float fTime )
{
	if (!m_bShow)
		return 1;

	const int LineHeight = CHud::UtfText::LineHeight();
	const int WidestCharacterWidth = CHud::UtfText::WidestCharacterWidth();

	const int PaddingWidth = WidestCharacterWidth;
	const int PaddingHeight = LineHeight;

	const int MarginHeight = LineHeight;

	const int FirstRowMinY = PaddingHeight + MarginHeight;

	// find the top of where the MOTD should be drawn,  so the whole thing is centered in the screen
	int ypos = ( ScreenHeight - LineHeight * m_lineOffsets.size() ) / 2; // shift it up slightly
	int xpos = ( ScreenWidth - WidestCharacterWidth * m_iMaxLength ) / 2;

	const int minTextXPos = MIN_TEXT_XPOS;
	if (xpos < minTextXPos)
		xpos = minTextXPos;

	const int visibleRows = Q_min(m_lineOffsets.size(), m_iMaxRowsPerWindow);

	int ypos_r = ypos;

	const bool scrollEnabled = m_lineOffsets.size() > m_iMaxRowsPerWindow;
	if (scrollEnabled)
	{
		ypos = ypos_r = FirstRowMinY;
	}

	int xmax = xpos + m_iMaxLength * WidestCharacterWidth;
	if (xmax > ScreenWidth - minTextXPos)
		xmax = ScreenWidth - minTextXPos;

	gHUD.DrawDarkRectangle(xpos - PaddingWidth, ypos_r - PaddingHeight, xmax - xpos + PaddingWidth*2, visibleRows * LineHeight + PaddingHeight*2);

	int r = 255;
	int g = 180;
	int b = 0;

	int startRow = scrollEnabled ? m_scrollLines : 0;
	int endRow = scrollEnabled ? m_scrollLines + m_iMaxRowsPerWindow : m_lineOffsets.size();
	endRow = Q_min(endRow, m_lineOffsets.size());

	if (startRow > 0)
	{
		CHud::UtfText::DrawString(xpos, ypos - LineHeight / 3 * 2, xmax, "^^^^^^", 255, 255, 0);
	}

	for (int i=startRow; i<endRow; ++i)
	{
		const auto& lineOffset = m_lineOffsets[i];
		CHud::UtfText::DrawString(xpos, ypos, xmax, m_szMOTD + lineOffset.first, r, g, b, lineOffset.second - lineOffset.first);
		ypos += LineHeight;
	}

	if (endRow < m_lineOffsets.size())
	{
		CHud::UtfText::DrawString(xpos, ypos - LineHeight / 3, xmax, "......", 255, 255, 0);
	}

	return 1;
}

bool CHudMOTD::HandleMOTDMessage( const char *pszName, int iSize, void *pbuf )
{
	if( m_iFlags & HUD_ACTIVE )
	{
		Reset(); // clear the current MOTD in prep for this one
	}

	BEGIN_READ( pbuf, iSize );

	int is_finished = READ_BYTE();
	strcatEnsureTermination( m_szMOTD, READ_STRING() );

	if( is_finished )
	{
		m_iMaxLength = 0;
		m_iFlags |= HUD_ACTIVE;

		m_lineOffsets = CHud::UtfText::CalcLineOffsets(m_szMOTD, MaxTextWidth());

		for (const auto& lineOffset : m_lineOffsets)
		{
			int length = lineOffset.second - lineOffset.first;
			if (length > m_iMaxLength)
				m_iMaxLength = length;
		}

		const int LineHeight = CHud::UtfText::LineHeight();

		const int PaddingHeight = LineHeight;
		const int MarginHeight = LineHeight;

		const int FirstRowMinY = PaddingHeight + MarginHeight;
		const int MaxTextYPos = ScreenHeight - PaddingHeight - MarginHeight;
		const int MaxTextHeight = MaxTextYPos - FirstRowMinY;

		m_iMaxRowsPerWindow = MaxTextHeight / LineHeight;
		m_iMaxRowsPerWindow = Q_max(1, m_iMaxRowsPerWindow);
	}

	return is_finished ? true : false;
}

int CHudMOTD::MaxTextWidth()
{
	return ScreenWidth - MIN_TEXT_XPOS * 2;
}

bool CHudMOTD::HandleKeyDown(int keynum)
{
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

void CHudMOTD::ScrollDown()
{
	if (m_lineOffsets.empty())
		return;

	m_scrollLines++;

	const int maxScroll = m_lineOffsets.size() - m_iMaxRowsPerWindow + 1;

	if (m_scrollLines > maxScroll)
		m_scrollLines = maxScroll;
}

void CHudMOTD::ScrollUp()
{
	m_scrollLines--;
	if (m_scrollLines < 0)
		m_scrollLines = 0;
}

void CHudMOTD::PageDown()
{
	if (m_lineOffsets.empty())
		return;

	m_scrollLines += m_iMaxRowsPerWindow;

	const int maxScroll = m_lineOffsets.size() - m_iMaxRowsPerWindow + 1;

	if (m_scrollLines > maxScroll)
		m_scrollLines = maxScroll;
}

void CHudMOTD::PageUp()
{
	if (m_lineOffsets.empty())
		return;

	m_scrollLines -= m_iMaxRowsPerWindow;
	if (m_scrollLines < 0)
		m_scrollLines = 0;
}
