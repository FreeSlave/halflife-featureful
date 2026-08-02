/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include "mobility_int.h"

DECLARE_MESSAGE( m_Health, Health )
DECLARE_MESSAGE( m_Health, Damage )
DECLARE_MESSAGE( m_Health, Battery )

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] =
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
};

int CHudHealth::Init()
{
	HOOK_MESSAGE( Health );
	HOOK_MESSAGE( Damage );
	HOOK_MESSAGE( Battery );

	m_iHealth = 100;
	m_iMaxHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset( m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES );

	m_iBat = 0;
	m_iMaxBat = 100;
	m_fArmorFade = 0;

	gHUD.AddHudElem( this );
	return 1;
}

void CHudHealth::Reset()
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;

	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit()
{
	m_hSprite = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );
	m_HUD_divider = gHUD.GetSpriteIndex( "divider" );

	giDmgHeight = gHUD.GetSpriteRect( m_HUD_dmg_bio ).right - gHUD.GetSpriteRect( m_HUD_dmg_bio ).left;
	giDmgWidth = gHUD.GetSpriteRect( m_HUD_dmg_bio ).bottom - gHUD.GetSpriteRect( m_HUD_dmg_bio ).top;

	m_HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	m_HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_fArmorFade = 0;

	return 1;
}

int CHudHealth::MsgFunc_Health( const char *pszName, int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();
	m_iMaxHealth = READ_SHORT();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if( x != m_iHealth )
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}
	return 1;
}

int CHudHealth::MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	long bitsDamage = READ_LONG(); // damage bits

	Vector vecFrom = READ_VECTOR();

	UpdateTiles( gHUD.m_flTime, bitsDamage );

	// Actually took damage?
	if( damageTaken > 0 || armor > 0 )
	{
		CalcDamageDirection( vecFrom );

                if( gMobileEngfuncs && damageTaken > 0 )
                {
			float time = damageTaken * 4.0f;

			if( time > 200.0f )
				time = 200.0f;
			gMobileEngfuncs->pfnVibrate( time, 0 );
                }
	}

	return 1;
}

int CHudHealth::MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();
	m_iMaxBat = READ_SHORT();

	if( x != m_iBat )
	{
		m_fArmorFade = FADE_TIME;
		m_iBat = x;
	}

	return 1;
}

// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetHealthColor( int &r, int &g, int &b )
{
	if( m_iHealth > 25 )
	{
		UnpackRGB( r, g, b, gHUD.HUDColor() );
	}
	else
	{
		UnpackRGB( r, g, b, gHUD.HUDColorCritical() );
	}
}

void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
#if 0
	int iHealth = m_iHealth;

	if( iHealth > 25 )
		iHealth -= 25;
	else if( iHealth < 0 )
		iHealth = 0;

	g = iHealth * 255 / 100;
	r = 255 - g;
	b = 0;
#else
	if( m_iHealth > 25 )
	{
		UnpackRGB( r, g, b, RGB_YELLOWISH );
	}
	else
	{
		UnpackRGB( r, g, b, RGB_REDISH );
	}
#endif
}

int CHudHealth::Draw( float flTime )
{
	if( ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if( !m_hSprite )
		m_hSprite = LoadSprite( PAIN_NAME );

	bool hasSuit = gHUD.HasSuit();

	// Only draw health if we have the suit.
	if (hasSuit || gHUD.DrawHUDNoSuit())
	{
		const bool drawArmor = m_iBat > 0 || hasSuit;
		const int armorStartX = DrawHealth(drawArmor);
		if (drawArmor)
			DrawArmor(armorStartX);
	}

	DrawDamage( flTime );
	return DrawPain( flTime );
}

int CHudHealth::DrawHealth(bool drawSeparator)
{
	int a = gHUD.MinHUDAlpha();

	// Has health changed? Flash the health #
	if( m_fFade )
	{
		m_fFade -= ( (float)gHUD.m_flTimeDelta * 20.0f );
		if( m_fFade <= 0 )
		{
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a += ( m_fFade / FADE_TIME ) * 128;
	}

	// If health is getting low, make it bright red
	if( m_iHealth <= 15 )
		a = 255;

	int r, g, b;
	GetHealthColor( r, g, b );
	ScaleColors( r, g, b, a );

	const int HealthWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	int CrossWidth = gHUD.GetSpriteRect( m_HUD_cross ).right - gHUD.GetSpriteRect( m_HUD_cross ).left;

	int y = CHud::Renderer().PerceviedScreenHeight() - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = CrossWidth / 2;

	CHud::Renderer().SPR_DrawAdditive( gHUD.GetSprite( m_HUD_cross ), r, g, b, x, y, &gHUD.GetSpriteRect( m_HUD_cross ) );

	x += CrossWidth;

	const int digitFlag = m_iHealth >= 1000 ? DHN_4DIGITS : DHN_3DIGITS;
	x = gHUD.DrawHudNumber( x, y + gHUD.m_iHudNumbersYOffset, digitFlag | DHN_DRAWZERO, m_iHealth, r, g, b );

	x += HealthWidth / 2;

	if (drawSeparator)
	{
		UnpackRGB( r, g, b, gHUD.HUDColor() );

		if (gHUD.clientFeatures.use_divider_sprite && m_HUD_divider != -1)
		{
			const wrect_t& rect = gHUD.GetSpriteRect(m_HUD_divider);
			const int dividerY = y + gHUD.m_iHudNumbersYOffset + gHUD.m_iFontHeight / 2 - (rect.bottom - rect.top) / 2;
			CHud::Renderer().SPR_DrawAdditiveWithAlphaScale(gHUD.GetSprite(m_HUD_divider), r, g, b, a, x, dividerY, &rect);
		}
		else
		{
			int iHeight = gHUD.m_iFontHeight;
			int iWidth = HealthWidth / 10;

			CHud::Renderer().FillRGBA( x, y + gHUD.m_iHudNumbersYOffset, iWidth, iHeight, r, g, b, a );
		}
	}

	return x + HealthWidth / 2;
}

void CHudHealth::DrawArmor(int startX)
{
	wrect_t suitEmptyRect = gHUD.GetSpriteRect(m_HUD_suit_empty);
	wrect_t suitFullRect = gHUD.GetSpriteRect(m_HUD_suit_full);
	const int batHeight = suitFullRect.bottom - suitEmptyRect.top;

	wrect_t rc = suitFullRect;
	rc.top  += batHeight * ( (float)( 100 - ( Q_min( 100, m_iBat ) ) ) * 0.01f );	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	int a = gHUD.MinHUDAlpha();

	// Has health changed? Flash the health #
	if( m_fArmorFade )
	{
		if( m_fArmorFade > FADE_TIME )
			m_fArmorFade = FADE_TIME;

		m_fArmorFade -= ( (float)gHUD.m_flTimeDelta * 20.0f );
		if( m_fArmorFade <= 0 )
		{
			m_fArmorFade = 0;
		}

		// Fade the health number back to dim
		a += ( m_fArmorFade / FADE_TIME ) * 128;
	}

	int r, g, b;
	UnpackRGB( r, g, b, gHUD.HUDColor() );
	ScaleColors( r, g, b, a );

	int iOffset = ( suitEmptyRect.bottom - suitEmptyRect.top ) / 6;

	int y = CHud::Renderer().PerceviedScreenHeight() - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = gHUD.DrawArmorNearHealth() ? startX : CHud::Renderer().PerceviedScreenWidth() / 5;

	CHud::Renderer().SPR_DrawAdditive( gHUD.GetSprite(m_HUD_suit_empty), r, g, b,  x, y - iOffset, &suitEmptyRect );

	if( rc.bottom > rc.top )
	{
		CHud::Renderer().SPR_DrawAdditive( gHUD.GetSprite(m_HUD_suit_full), r, g, b, x, y - iOffset + ( rc.top - suitFullRect.top ), &rc );
	}

	x += ( suitEmptyRect.right - suitEmptyRect.left );

	const int digitFlag = m_iBat >= 1000 ? DHN_4DIGITS : DHN_3DIGITS;
	x = gHUD.DrawHudNumber( x, y + gHUD.m_iHudNumbersYOffset, digitFlag | DHN_DRAWZERO, m_iBat, r, g, b );
}

void CHudHealth::CalcDamageDirection( Vector vecFrom )
{
	Vector forward, right, up;
	float side, front;
	Vector vecOrigin, vecAngles;

	if( !vecFrom[0] && !vecFrom[1] && !vecFrom[2] )
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}

	memcpy( vecOrigin, gHUD.m_vecOrigin, sizeof(Vector) );
	memcpy( vecAngles, gHUD.m_vecAngles, sizeof(Vector) );

	VectorSubtract( vecFrom, vecOrigin, vecFrom );

	float flDistToTarget = vecFrom.NormalizeInPlace();

	AngleVectors( vecAngles, forward, right, up );

	front = DotProduct( vecFrom, right );
	side = DotProduct( vecFrom, forward );

	if( flDistToTarget <= 50 )
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if( side > 0.0f )
		{
			if( side > 0.3f )
				m_fAttackFront = Q_max( m_fAttackFront, side );
		}
		else
		{
			float f = fabs( side );
			if( f > 0.3f )
				m_fAttackRear = Q_max( m_fAttackRear, f );
		}

		if( front > 0.0f )
		{
			if( front > 0.3f )
				m_fAttackRight = Q_max( m_fAttackRight, front );
		}
		else
		{
			float f = fabs( front );
			if( f > 0.3f )
				m_fAttackLeft = Q_max( m_fAttackLeft, f );
		}
	}
}

int CHudHealth::DrawPain( float flTime )
{
	if( !( m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight) )
		return 1;

	int r, g, b;
	int x, y, a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;

	// SPR_Draw top
	if( m_fAttackFront > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackFront, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 0 ) / 2;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite, 0 ) * 3;
		SPR_DrawAdditive( 0, x, y, NULL );
		m_fAttackFront = Q_max( 0, m_fAttackFront - fFade );
	} else
		m_fAttackFront = 0;

	if( m_fAttackRight > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackRight, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 + SPR_Width( m_hSprite, 1 ) * 2;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite,1 ) / 2;
		SPR_DrawAdditive( 1, x, y, NULL );
		m_fAttackRight = Q_max( 0, m_fAttackRight - fFade );
	}
	else
		m_fAttackRight = 0;

	if( m_fAttackRear > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackRear, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 2 ) / 2;
		y = ScreenHeight / 2 + SPR_Height( m_hSprite, 2 ) * 2;
		SPR_DrawAdditive( 2, x, y, NULL );
		m_fAttackRear = Q_max( 0, m_fAttackRear - fFade );
	}
	else
		m_fAttackRear = 0;

	if( m_fAttackLeft > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackLeft, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 3 ) * 3;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite,3 ) / 2;
		SPR_DrawAdditive( 3, x, y, NULL );

		m_fAttackLeft = Q_max( 0, m_fAttackLeft - fFade );
	} else
		m_fAttackLeft = 0;

	return 1;
}

int CHudHealth::DrawDamage( float flTime )
{
	int i, r, g, b, a;
	DAMAGE_IMAGE *pdmg;

	if( !m_bitsDamage )
		return 1;

	UnpackRGB( r, g, b, gHUD.HUDColor() );

	a = (int)( fabs( sin( flTime * 2.0f ) ) * 256.0f );

	ScaleColors( r, g, b, a );

	// check for bits that should be expired
	for( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg = &m_dmg[i];

			// Draw all the items
			CHud::Renderer().SPR_DrawAdditive( gHUD.GetSprite( m_HUD_dmg_bio + i ), r, g, b, pdmg->x, pdmg->y, &gHUD.GetSpriteRect( m_HUD_dmg_bio + i ) );

			pdmg->fExpire = Q_min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for( int j = 0; j < NUM_DMG_TYPES; j++ )
				{
					pdmg = &m_dmg[j];
					if( ( pdmg->y ) && ( pdmg->y < y ) )
						pdmg->y += giDmgHeight;

				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}

void CHudHealth::UpdateTiles( float flTime, long bitsDamage )
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;

	for( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if( !pdmg->fBaseline )
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if( bitsOn & giDmgFlags[i] )
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth / 8;
			pdmg->y = CHud::Renderer().PerceviedScreenHeight() - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;

			// move everyone else up
			for( int j = 0; j < NUM_DMG_TYPES; j++ )
			{
				if( j == i )
					continue;

				pdmg = &m_dmg[j];
				if( pdmg->y )
					pdmg->y -= giDmgHeight;
			}
			// pdmg = &m_dmg[i];
		}
	}

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}
