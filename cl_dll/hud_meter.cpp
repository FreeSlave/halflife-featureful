#include <cmath>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Meter, SoundVolume )

int CHudMeter::Init()
{
	HOOK_MESSAGE( SoundVolume );

	m_iFlags = HUD_ACTIVE;

	hud_speedometer = CVAR_CREATE("hud_speedometer", "0", FCVAR_ARCHIVE);
	hud_speedometer_below_cross = CVAR_CREATE("hud_speedometer_below_cross", "0", FCVAR_ARCHIVE);
	hud_speedometer_height = CVAR_CREATE("hud_speedometer_height", "0", FCVAR_ARCHIVE);
	hud_soundlevelmeter = CVAR_CREATE("hud_soundlevelmeter", "0", FCVAR_ARCHIVE);

	gHUD.AddHudElem(this);
	return 0;
}

int CHudMeter::VidInit()
{
	soundVolume = 0;
	return 1;
}

extern float g_ServerMaxSpeed;

int CHudMeter::Draw(float time)
{
	int r, g, b;
	UnpackRGB(r, g, b, gHUD.HUDColor());

	if (hud_speedometer->value != 0.0f)
	{
		int y;
		if (hud_speedometer_below_cross->value != 0.0f)
			y = CHud::Renderer().PerceviedScreenHeight() / 2 + gHUD.m_iFontHeight / 2;
		else if (hud_speedometer_height->value != 0.0f)
			y = hud_speedometer_height->value;
		else
			y = CHud::Renderer().PerceviedScreenHeight() - gHUD.m_iFontHeight * 2.5f;

		int x = CHud::Renderer().PerceviedScreenWidth() / 2;

		gHUD.DrawHudNumberCentered(x, y, speed, r, g, b);

		if (hud_speedometer->value >= 2.0f)
		{
			float clientMaxSpeed = gEngfuncs.GetClientMaxspeed();
			if (!clientMaxSpeed)
				clientMaxSpeed = g_ServerMaxSpeed;
			gHUD.DrawHudNumberCentered(x, y + gHUD.m_iFontHeight, clientMaxSpeed, r, g, b);
		}
	}

	if (hud_soundlevelmeter->value != 0.0f && soundVolume > 0)
	{
		int y = gHUD.m_iFontHeight * 1.5f;
		gHUD.DrawHudNumberCentered(CHud::Renderer().PerceviedScreenWidth()/2, y, soundVolume, r, g, b);
	}

	return 0;
}

int CHudMeter::MsgFunc_SoundVolume( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	soundVolume = READ_SHORT();
	return 1;
}

void CHudMeter::UpdateSpeed(const float velocity[2])
{
	speed = std::round(std::hypot(velocity[0], velocity[1]));
}
