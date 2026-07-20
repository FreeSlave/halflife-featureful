#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_ErrorCollection, ParseErrors )
DECLARE_MESSAGE( m_ErrorCollection, Deprecation )

int CHudErrorCollection::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags &= ~HUD_ACTIVE;
	HOOK_MESSAGE(ParseErrors);
	HOOK_MESSAGE(Deprecation);

	m_pCvarShowDeprecations = CVAR_CREATE("cl_showdeprecations", "1", FCVAR_ARCHIVE);
	return 1;
}

int CHudErrorCollection::VidInit()
{
	m_deprecationMessages.clear();
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
	const bool shouldShowDeprecations = m_pCvarShowDeprecations && m_pCvarShowDeprecations->value && !m_deprecationMessages.empty();

	if (m_serverErrorString.empty() && m_clientErrorString.empty() && !shouldShowDeprecations)
		return 1;

	const int LineHeight = CHud::UtfText::LineHeight();
	const int CharacterWidth = CHud::UtfText::WidestCharacterWidth();
	int ypos = LineHeight * 2;
	int xpos = CharacterWidth;
	int xmax = ScreenWidth - CharacterWidth;

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
		ypos = DrawMultiLineString(m_clientErrorString.c_str(), xpos, ypos, xmax, LineHeight);
		ypos += LineHeight;
	}

	if (shouldShowDeprecations)
	{
		CHud::UtfText::DrawString(xpos, ypos, xmax, "DEPRECATIONS:", r, g, b);
		ypos += LineHeight;
		for (const auto& msg : m_deprecationMessages)
		{
			ypos = DrawMultiLineString(msg.c_str(), xpos, ypos, xmax, LineHeight);
			ypos += LineHeight;
		}
		CHud::UtfText::DrawString(xpos, ypos, xmax, "Note: set cl_showdeprecations to 0 to hide deprecation messages", r, g, b);
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

int CHudErrorCollection::MsgFunc_Deprecation(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	std::string msg = READ_STRING();
	m_deprecationMessages.push_back(std::move(msg));

	m_iFlags |= HUD_ACTIVE;

	return 1;
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
	return CHud::UtfText::DrawMultiLineString(str, xpos, ypos, xmax, LineHeight, 255, 140, 0);
}
