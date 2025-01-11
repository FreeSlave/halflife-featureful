#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_ErrorCollection, ParseErrors )

int CHudErrorCollection::Init()
{
	m_pCvarDeveloper = nullptr;
	gHUD.AddHudElem(this);
	m_iFlags &= ~HUD_ACTIVE;
	HOOK_MESSAGE(ParseErrors);
	return 1;
}

int CHudErrorCollection::VidInit()
{
	m_pCvarDeveloper = gEngfuncs.pfnGetCvarPointer("developer");
	return 1;
}

void CHudErrorCollection::Reset()
{
	if (m_clientErrorString.empty())
		m_iFlags &= ~HUD_ACTIVE;
	else
		m_iFlags |= HUD_ACTIVE;
	m_serverErrorString.clear();
}

int CHudErrorCollection::Draw(float flTime)
{
	if (!m_pCvarDeveloper || m_pCvarDeveloper->value == 0)
		return 1;

	if (m_serverErrorString.empty() && m_clientErrorString.empty())
		return 1;

	const int LineHeight = CHud::UtfText::LineHeight();
	int ypos = LineHeight * 2;
	int xpos = 30;
	int xmax = ScreenWidth;

	int r = 255;
	int g = 140;
	int b = 0;

	if (m_serverErrorString.size())
	{
		CHud::UtfText::DrawString(xpos, ypos, xmax, "SERVER ERRORS:", r, g, b);
		ypos += LineHeight;
		ypos = DrawMultiLineString(m_serverErrorString.c_str(), xpos, ypos, xmax, LineHeight);
		ypos += LineHeight;
	}

	if (m_clientErrorString.size())
	{
		CHud::UtfText::DrawString(xpos, ypos, xmax, "CLIENT ERRORS:", r, g, b);
		ypos += LineHeight;
		DrawMultiLineString(m_clientErrorString.c_str(), xpos, ypos, xmax, LineHeight);
	}


	return 1;
}

int CHudErrorCollection::MsgFunc_ParseErrors(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	int is_finished = READ_BYTE();
	const char* str = READ_STRING();

	m_serverErrorString += str;

	if (is_finished)
	{
		m_iFlags |= HUD_ACTIVE;
	}

	return is_finished ? 1 : 0;
}

void CHudErrorCollection::SetClientErrors(const std::string &str)
{
	m_clientErrorString = str;
	if (m_clientErrorString.size())
	{
		m_iFlags |= HUD_ACTIVE;
	}
}

int CHudErrorCollection::DrawMultiLineString(const char *str, int xpos, int ypos, int xmax, const int LineHeight)
{
	int r = 255;
	int g = 140;
	int b = 0;

	const char *ch = str;
	while(*ch)
	{
		const char *next_line = ch;
		for(; *next_line != '\n' && *next_line != '\0'; next_line++)
			;
		CHud::UtfText::DrawString( xpos, ypos, xmax, ch, r, g, b, next_line - ch );

		ypos += LineHeight;

		ch = next_line;
		if (*ch == '\n')
			ch++;
	}
	return ypos;
}
