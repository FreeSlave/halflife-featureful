#include "hud.h"
#include "cl_util.h"
#include "monsterinfo.h"
#include "parsemsg.h"
#include "string_utils.h"
#include "util_shared.h"
#include "triangleapi.h"

DECLARE_MESSAGE(m_DamageInfo, ShowDamage)

int CHudDamageInfo::Init()
{
	HOOK_MESSAGE(ShowDamage);
	gHUD.AddHudElem(this);

	Reset();
	return 1;
}

int CHudDamageInfo::VidInit()
{
	Reset();
	return 1;
}

int CHudDamageInfo::Draw(float flTime)
{
	for (int i = 0; i < MAX_DAM_INFO; i++)
	{
		if (!info[i].damage)
			continue;

		if (flTime > info[i].timeLeft)
			continue;

		Vector			screen;
		if (gEngfuncs.pTriAPI->WorldToScreen(info[i].pos, screen))
			continue;	// object is behind viewer
		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		screen[2] = 0.0f;
		int r, g, b;
		UnpackRGB(r, g, b, RGB_YELLOWISH);
		CHud::UtfText::DrawNumberString(screen[0], screen[1],100,info[i].damage,r,g,b);
	}
	return 1;
}

void CHudDamageInfo::Reset()
{
}

int CHudDamageInfo::MsgFunc_ShowDamage(const char *pszName, int iSize, void *pbuf)
{
	m_iFlags |= HUD_ACTIVE;
	BEGIN_READ(pbuf, iSize);
	Vector position = READ_VECTOR();
	float amount = READ_LONG();

	DamageInfo_t temp_info;
	memset(&temp_info, 0, sizeof(temp_info));
	temp_info.damage = amount;
	temp_info.pos = position;
	temp_info.timeLeft = gHUD.m_flTime + 5;
	info[digit_count] = temp_info;
	digit_count++;
	if (digit_count >= MAX_DAM_INFO)
		digit_count = 0;

	return 1;
}
