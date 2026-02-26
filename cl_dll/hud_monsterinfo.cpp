#include "hud.h"
#include "cl_util.h"
#include "monsterinfo.h"
#include "parsemsg.h"
#include "string_utils.h"
#include "util_shared.h"

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
		g = 75;
		b = 75;
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

	const int update = READ_BYTE();

	if (update == MONSTERINFO_CLEAR)
	{
		Reset();
		return 1;
	}

	const char* name = nullptr;

	if (update == MONSTERINFO_FULLUPDATE)
	{
		name = READ_STRING();

		if(!(m_iFlags & HUD_ACTIVE))
			m_iFlags |= HUD_ACTIVE;
	}

	health = READ_SHORT();
	maxHealth = READ_SHORT();
	armor = READ_SHORT();

	const int monsterInfoFlags = READ_BYTE();

	isMonster = FBitSet(monsterInfoFlags, MONSTERINFO_FLAG_MONSTER);
	isPlayer = FBitSet(monsterInfoFlags, MONSTERINFO_FLAG_PLAYER);
	isAlly = FBitSet(monsterInfoFlags, MONSTERINFO_FLAG_ALLY);
	isMachine = FBitSet(monsterInfoFlags, MONSTERINFO_FLAG_MACHINE);

	if (name)
	{
		const char* localizedName = nullptr;
		if (!isPlayer) // don't localize player's names
		{
			localizedName = gHUD.m_displayNames.GetDisplayName(name);
		}

		if (localizedName)
			strncpyEnsureTermination(displayName, localizedName);
		else
			strncpyEnsureTermination(displayName, name);
	}

	const char* healthString = isMachine ? gHUD.m_messageStrings.GetText("__SHOWINFO_STRENGTH", "Strength") : gHUD.m_messageStrings.GetText("__SHOWINFO_HEALTH", "Health");

	if (isPlayer)
	{
		const char* armorString = gHUD.m_messageStrings.GetText("__SHOWINFO_ARMOR", "Armor");

		safe_snprintf(healthDisplay, sizeof(healthDisplay), "%s: %d", healthString, health);
		safe_snprintf(armorDisplay, sizeof(armorDisplay), "%s: %d", armorString, armor);
	}
	else if (isMonster)
	{
		if (health > 0)
			safe_snprintf(healthDisplay, sizeof(healthDisplay), "%s: %d/%d", healthString, health, maxHealth);
		else
			safe_snprintf(healthDisplay, sizeof(healthDisplay), "%s: %d", healthString, health);
	}
	else
	{
		const char* strengthString = gHUD.m_messageStrings.GetText("__SHOWINFO_STRENGTH", "Strength");
		safe_snprintf(healthDisplay, sizeof(healthDisplay), "%s: %d", strengthString, health);
	}

	return 1;
}
