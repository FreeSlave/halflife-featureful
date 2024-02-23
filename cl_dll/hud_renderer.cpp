#include <cassert>

#include "hud.h"
#include "cl_util.h"

#include "triangleapi.h"

#include "hud_renderer.h"

// Note: TriAPI rendering won't work in software!

extern cvar_t *hud_scale;
extern cvar_t *hud_sprite_offset;

HudSpriteRenderer::HudSpriteRenderer() {
	sprite = -1;
	sprite_model = NULL;
	sprite_color.r = 0;
	sprite_color.g = 0;
	sprite_color.b = 0;

	crosshair_color.r = 0;
	crosshair_color.g = 0;
	crosshair_color.b = 0;
}

float HudSpriteRenderer::GetHUDScale() const {
	return currentScale;
}

bool HudSpriteRenderer::IsCustomScale() const {
	return currentScale != 1.0f;
}

int HudSpriteRenderer::PerceviedScreenWidth() {
	if (IsCustomScale())
		return static_cast<int>(gHUD.m_scrinfo.iWidth / GetHUDScale());
	else
		return gHUD.m_scrinfo.iWidth;
}

int HudSpriteRenderer::PerceviedScreenHeight() {
	if (IsCustomScale())
		return static_cast<int>(gHUD.m_scrinfo.iHeight / GetHUDScale());
	else
		return gHUD.m_scrinfo.iHeight;
}

int HudSpriteRenderer::ScaleScreen(int value) {
	return static_cast<int>(value * GetHUDScale());
}

int HudSpriteRenderer::UnscaleScreen(int value) {
	return static_cast<int>(value / GetHUDScale());
}

void HudSpriteRenderer::SPR_DrawInternal(int frame, float x, float y, float width, float height, const wrect_t *dimensions, int mode) {
	if (!sprite_model) {
		return;
	}

	float s1 = 0.0f;
	float s2 = 0.0f;
	float t1 = 0.0f;
	float t2 = 0.0f;

	if (width == -1.0f && height == -1.0f) {
		width = SPR_Width(sprite, frame);
		height = SPR_Height(sprite, frame);
	}

	if (dimensions) {
		wrect_t rc = *dimensions;

		if (rc.left <= 0 || rc.left >= width) {
			rc.left = 0;
		}

		if (rc.top <= 0 || rc.top >= height) {
			rc.top = 0;
		}

		if (rc.right <= 0 || rc.right > width) {
			rc.right = width;
		}

		if (rc.bottom <= 0 || rc.bottom > height) {
			rc.bottom = height;
		}

		float offset = 0.0f;

		if (GetHUDScale() > 1.0f && hud_sprite_offset) {
			offset = hud_sprite_offset->value;
		}

		s1 = (rc.left + offset) / width;
		t1 = (rc.top + offset) / height;

		s2 = (rc.right - offset) / width;
		t2 = (rc.bottom - offset) / height;

		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	} else {
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	x = ScaleScreen(x);
	y = ScaleScreen(y);

	width = ScaleScreen(width);
	height = ScaleScreen(height);

	if (!gEngfuncs.pTriAPI->SpriteTexture(sprite_model, frame)) {
		return;
	}

	gEngfuncs.pTriAPI->Color4ub(sprite_color.r, sprite_color.g, sprite_color.b, 255);

	gEngfuncs.pTriAPI->RenderMode(mode);

	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		gEngfuncs.pTriAPI->TexCoord2f(s1, t1);
		gEngfuncs.pTriAPI->Vertex3f(x, y, 0.0f);
		gEngfuncs.pTriAPI->TexCoord2f(s2, t1);
		gEngfuncs.pTriAPI->Vertex3f(x + width, y, 0.0f);
		gEngfuncs.pTriAPI->TexCoord2f(s2, t2);
		gEngfuncs.pTriAPI->Vertex3f(x + width, y + height, 0.0f);
		gEngfuncs.pTriAPI->TexCoord2f(s1, t2);
		gEngfuncs.pTriAPI->Vertex3f(x, y + height, 0.0f);
	gEngfuncs.pTriAPI->End();

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

cvar_t* crosshair_force_custom = NULL;
cvar_t* crosshair_debug = NULL;
cvar_t* crosshair_even_scale = NULL;
cvar_t* crosshair_size = NULL;

void HudSpriteRenderer::Init() {
	crosshair_force_custom = CVAR_CREATE("crosshair_force_custom", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	crosshair_debug = CVAR_CREATE("crosshair_debug", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	crosshair_even_scale = CVAR_CREATE("crosshair_even_scale", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	crosshair_size = CVAR_CREATE("crosshair_size", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}

int HudSpriteRenderer::VidInit() {
	crosshairSizePrevValue = crosshair_size ? (int)crosshair_size->value : 0;

	if (gHUD.hasHudScaleInEngine || gHUD.UsingHighResSprites())
		return 1;

	float average_scale = 1.0f;

	float x_scale;
	float y_scale;

	const int iWidth = gHUD.m_scrinfo.iWidth;
	const int iHeight = gHUD.m_scrinfo.iHeight;

	if (iWidth > 1024) {
		if (iWidth >= 1280) {
			x_scale = iWidth / 960.0f;
		} else {
			x_scale = iWidth / 640.0f;
		}

		if (iHeight >= 1024 || iWidth > 1280) {
			y_scale = iHeight / 720.0f;
		} else {
			y_scale = iHeight / 480.0f;
		}

		if (x_scale > 0.0f && y_scale > 0.0f) {
			average_scale = sqrtf(0.5f * (x_scale * x_scale + y_scale * y_scale));

			float doubleScale = average_scale * 2.0f;
			float rounded = floor(doubleScale + 0.5f);
			average_scale = rounded / 2.0f;
		}
	}
	hud_auto_scale_value = average_scale;

	return 1;
}

enum
{
	CROSSHAIR_SIZE_MATCH_HUD_SCALE = 0,
	CROSSHAIR_SIZE_MATCH_SCREEN_RESOLUTION = 1,
	CROSSHAIR_SIZE_SMALL,
	CROSSHAIR_SIZE_MEDIUM,
	CROSSHAIR_SIZE_LARGE
};

void HudSpriteRenderer::HUD_Frame(double time) {
	(void)time;
	RecalcHUDScale();

	if (crosshair.crosshair.sprite > 0)
	{
		bool shouldResetCrosshair = false;

		const int crosshairSizeCurrent = crosshair_size ? (int)crosshair_size->value : 0;
		if (crosshairSizeCurrent != crosshairSizePrevValue)
		{

			shouldResetCrosshair = true;
			gEngfuncs.Con_DPrintf("Resetting crosshair because user changed the preferred crosshair size\n");
		}
		else if (cachedHudScale != hudScalePrevValue && crosshairSizeCurrent == CROSSHAIR_SIZE_MATCH_HUD_SCALE)
		{
			shouldResetCrosshair = true;
			gEngfuncs.Con_DPrintf("Resetting crosshair because the hud scale has been changed (new scale: %g vs old scale: %g)\n", cachedHudScale, hudScalePrevValue);
		}

		crosshairSizePrevValue = crosshairSizeCurrent;
		hudScalePrevValue = cachedHudScale;

		if (shouldResetCrosshair)
		{
			gHUD.ResetCrosshair();
		}
	}
}

void HudSpriteRenderer::SPR_Set(HSPRITE hPic, int r, int g, int b) {
	if (IsCustomScale()) {
		SPR_SetInternal(hPic, r, g, b);
	} else {
		::SPR_Set(hPic, r, g, b);
	}

}

void HudSpriteRenderer::SPR_SetInternal(HSPRITE hPic, int r, int g, int b) {
	sprite = hPic;
	sprite_model = const_cast<model_t *>(gEngfuncs.GetSpritePointer(sprite));
	sprite_color.r = r;
	sprite_color.g = g;
	sprite_color.b = b;
}

void HudSpriteRenderer::SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc) {
	if (IsCustomScale()) {
		SPR_DrawInternal(frame, x, y, -1.0f, -1.0f, prc, kRenderTransAdd);
	} else {
		::SPR_DrawAdditive(frame, x, y, prc);
	}
}

void HudSpriteRenderer::SPR_DrawAdditive(HSPRITE hPic, int r, int g, int b, int x, int y, const wrect_t *prc, int frame) {
	SPR_Set(hPic, r, g, b);
	SPR_DrawAdditive(frame, x, y, prc);
}

void HudSpriteRenderer::FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a) {
	if (IsCustomScale()) {
		::FillRGBA(ScaleScreen(x), ScaleScreen(y), ScaleScreen(width), ScaleScreen(height), r, g, b, a);
	} else {
		::FillRGBA(x, y, width, height, r, g, b, a);
	}
}

static int GetCrosshairSpriteRes( int width, int height )
{
	int i;

	if( width < 640 )
		i = 320;
	else if( width < 1280 )
		i = 640;
	else
	{
		if( height <= 720 )
			i = 640;
		else if( width <= 2560 || height <= 1600 )
			i = 1280;
		else
		{
			i = 2560;
		}
	}

	return i;
}

void HudSpriteRenderer::SetCrosshair(const CrosshairSpriteData &crosshairData, int r, int g, int b)
{
	SetCrosshairData(crosshairData, r, g, b);

	const bool forceCustomCrosshairRendering = crosshair_force_custom && crosshair_force_custom->value && gHUD.m_iHardwareMode != 0;
	const bool crosshairDebug = crosshair_debug && crosshair_debug->value;
	float crosshairScale = cachedHudScale;
	if (crosshair_even_scale && crosshair_even_scale->value) {
		crosshairScale = floor(crosshairScale + 0.5f);
	}

	bool wants1280 = false;
	bool wants2560 = false;
	if (crosshair_size && crosshair_size->value >= 1)
	{
		if (crosshair_size->value >= CROSSHAIR_SIZE_LARGE)
		{
			wants1280 = true;
			wants2560 = true;
		}
		else if (crosshair_size->value >= CROSSHAIR_SIZE_MEDIUM)
		{
			wants1280 = true;
			wants2560 = false;
		}
		else if (crosshair_size->value >= CROSSHAIR_SIZE_SMALL)
		{
			wants1280 = false;
			wants2560 = false;
			crosshairScale = 1.0f;
		}
		else
		{
			const int preferredSpriteRes = GetCrosshairSpriteRes(ScreenWidth, ScreenHeight);
			wants1280 = preferredSpriteRes == 1280;
			wants2560 = preferredSpriteRes == 2560;
		}
	}
	else
	{
		wants1280 = crosshairScale == 2.0f;
		wants2560 = crosshairScale == 3.0f;
	}

	if (wants2560 && crosshairData.crosshair_2560.sprite > 0 && !forceCustomCrosshairRendering)
	{
		if (crosshairDebug)
			gEngfuncs.Con_DPrintf("Setting crosshair of 2560 resolution in the engine\n");
		customCrosshairRendering = false;
		gEngfuncs.pfnSetCrosshair(crosshairData.crosshair_2560.sprite, crosshairData.crosshair_2560.rect, r, g, b);
	}
	else if (wants1280 && crosshairData.crosshair_1280.sprite > 0 && !forceCustomCrosshairRendering)
	{
		if (crosshairDebug)
			gEngfuncs.Con_DPrintf("Setting crosshair of 1280 resolution in the engine\n");
		customCrosshairRendering = false;
		gEngfuncs.pfnSetCrosshair(crosshairData.crosshair_1280.sprite, crosshairData.crosshair_1280.rect, r, g, b);
	}
	else if ((crosshairScale == 1.0f || gHUD.m_iHardwareMode == 0) && !forceCustomCrosshairRendering)
	{
		if (crosshairData.crosshair.sprite > 0 && crosshairDebug)
			gEngfuncs.Con_DPrintf("Setting crosshair in the engine\n");
		customCrosshairRendering = false;
		gEngfuncs.pfnSetCrosshair(crosshairData.crosshair.sprite, crosshairData.crosshair.rect, r, g, b);
	}
	else if (gHUD.m_iHardwareMode != 0)
	{
		if (crosshairData.crosshair.sprite > 0 && crosshairDebug)
			gEngfuncs.Con_DPrintf("Enabling custom crosshair rendering\n");
		customCrosshairRendering = true;

		wrect_t crosshair_rect;
		crosshair_rect.left = 0;
		crosshair_rect.right = 0;
		crosshair_rect.top = 0;
		crosshair_rect.bottom = 0;

		gEngfuncs.pfnSetCrosshair(0, crosshair_rect, 0, 0, 0);
	}
}

void HudSpriteRenderer::SetCrosshairData(const CrosshairSpriteData &crosshairData, int r, int g, int b)
{
	crosshair = crosshairData;
	crosshair_color.r = r;
	crosshair_color.g = g;
	crosshair_color.b = b;
}

void HudSpriteRenderer::DrawCrosshair()
{
	// using original crosshair rendering
	if (!customCrosshairRendering)
		return;

	SpriteRectPair spriteRectPair = crosshair.crosshair;
	if (crosshair.crosshair_2560.sprite > 0 && currentScale >= 3.0f)
	{
		currentScale = currentScale / 3.0f;
		spriteRectPair = crosshair.crosshair_2560;
	}
	else if (crosshair.crosshair_1280.sprite > 0 && currentScale >= 2.0f)
	{
		currentScale = currentScale / 2.0f;
		spriteRectPair = crosshair.crosshair_1280;
	}
	else if (crosshair.crosshair.sprite <= 0)
	{
		return;
	}

	const int width = spriteRectPair.rect.right - spriteRectPair.rect.left;
	const int height = spriteRectPair.rect.bottom - spriteRectPair.rect.top;

	const int x = PerceviedScreenWidth() >> 1;
	const int y = PerceviedScreenHeight() >> 1;

	if (IsCustomScale())
	{
		SPR_SetInternal(spriteRectPair.sprite, crosshair_color.r, crosshair_color.g, crosshair_color.b);
		SPR_DrawInternal(0, x - 0.5f * width, y - 0.5f * height, -1.0f, -1.0f, &spriteRectPair.rect, kRenderTransTexture);
	}
	else
	{
		::SPR_Set(spriteRectPair.sprite, crosshair_color.r, crosshair_color.g, crosshair_color.b);
		::SPR_DrawHoles(0, x - 0.5f * width, y - 0.5f * height, &spriteRectPair.rect);
	}
}

void HudSpriteRenderer::RecalcHUDScale()
{
	if (gHUD.hasHudScaleInEngine || gHUD.m_iHardwareMode == 0 || gHUD.UsingHighResSprites())
	{
		cachedHudScale = 1.0f;
		return;
	}

	float scale = hud_scale ? hud_scale->value : gHUD.clientFeatures.hud_scale.defaultValue;
	if (scale <= 0.0f)
		scale = hud_auto_scale_value;

	cachedHudScale = scale;
}

HudSpriteRenderer& HudSpriteRenderer::DefaultScale()
{
	currentScale = cachedHudScale;
	return *this;
}

HudSpriteRenderer& HudSpriteRenderer::RelativeScale(float multiplier)
{
	if (gHUD.m_iHardwareMode != 0)
		currentScale = cachedHudScale * multiplier;
	else
		currentScale = 1.0f;
	return *this;
}
