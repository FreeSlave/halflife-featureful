#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "arraysize.h"
#include "string_utils.h"
#include "util_shared.h"
#include "triangleapi.h"
#include "combattext.h"

extern Vector v_origin;

DECLARE_MESSAGE(m_CombatText, CombatText)

int CHudCombatText::Init()
{
	HOOK_MESSAGE(CombatText);
	gHUD.AddHudElem(this);

	hud_combattext = CVAR_CREATE("hud_combattext", "0", FCVAR_ARCHIVE);
	hud_combattext_time = CVAR_CREATE("hud_combattext_time", "1.5", FCVAR_ARCHIVE);
	hud_combattext_speed = CVAR_CREATE("hud_combattext_speed", "32", FCVAR_ARCHIVE);

	Reset();
	return 1;
}

int CHudCombatText::VidInit()
{
	Reset();
	return 1;
}

int CHudCombatText::Draw(float flTime)
{
	if (hud_combattext->value == 0.0f)
		return 1;

	float textLifeTime = hud_combattext_time->value;
	textLifeTime = Q_max(textLifeTime, 0.5f);

	const float textSpeed = hud_combattext_speed->value;

	for (auto& text : texts)
	{
		if (text.damage <= 0)
			continue;

		if (text.startTime + textLifeTime < flTime)
			continue;

		const float distance = (v_origin - text.pos).Length();
		const float velocity = textSpeed * distance / 128.0f;

		text.pos.z += gHUD.m_flTimeDelta * velocity;
		Vector screen;
		if (gEngfuncs.pTriAPI->WorldToScreen(text.pos, screen))
			continue;
		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		screen[2] = 0.0f;
		int r, g, b;
		switch(text.type)
		{
		case COMBATTEXT_CRITICAL:
			r = 255;
			g = 0;
			b = 0;
			break;
		case COMBATTEXT_POWERSHIELD:
			r = 255;
			g = 85;
			b = 255;
			break;
		default:
			UnpackRGB(r, g, b, RGB_YELLOWISH);
			break;
		}

		CHud::UtfText::DrawFloatNumberString(screen[0], screen[1],120,text.damage,r,g,b);
	}
	return 1;
}

void CHudCombatText::Reset()
{
	lastSlot = 0;
	for (auto& item : texts)
	{
		item = CombatText{};
	}
}

int CHudCombatText::MsgFunc_CombatText(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	Vector position = READ_VECTOR();
	float amount = READ_LONG() * 0.01f;
	int type = READ_BYTE();

	if (hud_combattext->value == 0.0f)
		return 1;

	float textLifeTime = hud_combattext_time->value;
	textLifeTime = Q_max(textLifeTime, 0.5f);

	const float distance = (v_origin - position).Length();

	const float height = 4 * distance / 128.0f;
	float threshold = height * 0.75f;

	const float clientTime = gEngfuncs.GetClientTime();
	for (auto& text : texts)
	{
		if (text.damage <= 0)
			continue;

		if (text.startTime + textLifeTime < clientTime)
			continue;

		if ((position - text.pos).IsLengthLessThanOrEqual(threshold))
		{
			position.z -= height;
		}
	}

	m_iFlags |= HUD_ACTIVE;

	CombatText ctext;
	ctext.damage = amount;
	ctext.pos = position;
	ctext.startTime = gHUD.m_flTime;
	ctext.type = type;
	texts[lastSlot] = ctext;
	lastSlot++;
	if (lastSlot >= ARRAYSIZE(texts))
		lastSlot = 0;

	return 1;
}
