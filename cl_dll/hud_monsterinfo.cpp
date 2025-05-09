#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "string_utils.h"

DECLARE_MESSAGE( m_MonsterInfo, MonsterInfo )

int CHudMonsterInfo::Init()
{
	HOOK_MESSAGE(MonsterInfo);
	gHUD.AddHudElem(this);

	Reset();

	m_pCvarShowMonsterInfo = CVAR_CREATE("hud_showmonsterinfo", "1", FCVAR_ARCHIVE);

	return 1;
}

int CHudMonsterInfo::VidInit()
{
	Reset();
	return 1;
}

int CHudMonsterInfo::Draw(float flTime)
{
	if (m_pCvarShowMonsterInfo->value <= 0)
		return 0;

	if (!*displayName)
		return 0;

	const int xPos = 0.1 * ScreenWidth;
	int yPos = 0.6 * ScreenHeight;

	int r, g, b;
	if (isAlly)
	{
		r = 0;
		g = 255;
		b = 0;
	}
	else
	{
		r = 255;
		g = 0;
		b = 0;
	}

	CHud::UtfText::DrawString(xPos, yPos, displayName, r, g, b);

	yPos += CHud::UtfText::LineHeight();
	CHud::UtfText::DrawString(xPos, yPos, healthDisplay, r, g, b);

	if (isPlayer)
	{
		yPos += CHud::UtfText::LineHeight();
		CHud::UtfText::DrawString(xPos, yPos, armorDisplay, r, g, b);
	}

	return 1;
}

void CHudMonsterInfo::Reset()
{
	displayName[0] = '\0';
	healthDisplay[0] = '\0';
	armorDisplay[0] = '\0';
}

int CHudMonsterInfo::MsgFunc_MonsterInfo(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const char* name = READ_STRING();

	if (!*name)
	{
		Reset();
		return 1;
	}

	health = READ_SHORT();
	maxHealth = READ_SHORT();
	armor = READ_SHORT();
	isPlayer = READ_BYTE() ? true : false;
	isAlly = READ_BYTE() ? true : false;

	if(!(m_iFlags & HUD_ACTIVE))
		m_iFlags |= HUD_ACTIVE;

	strncpyEnsureTermination(displayName, name);
	if (isPlayer)
	{
		safe_snprintf(healthDisplay, sizeof(healthDisplay), "Health: %d", health);
		safe_snprintf(armorDisplay, sizeof(armorDisplay), "Armor: %d", armor);
	}
	else
	{
		safe_snprintf(healthDisplay, sizeof(healthDisplay), "Health: %d/%d", health, maxHealth);
	}

	return 1;
}
