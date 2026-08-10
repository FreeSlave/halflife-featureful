#include <cassert>

#include "hud.h"
#include "cl_util.h"

#include "triangleapi.h"

#include "hud_renderer.h"

// Note: TriAPI rendering won't work in software!

extern cvar_t *hud_scale;
extern cvar_t *hud_sprite_offset;

static void ScaledSetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b)
{
	CHud::Renderer().SetCrosshair(hspr, rc, r, g, b);
}

HudSpriteRenderer::HudSpriteRenderer() {
	origSpriteEngfuncs.pfnSetCrosshair = NULL;

	sprite = -1;
	sprite_model = NULL;
	sprite_color.r = 0;
	sprite_color.g = 0;
	sprite_color.b = 0;

	crosshair = -1;
	crosshair_model = NULL;
	crosshair_dimensions.left = 0;
	crosshair_dimensions.right = 0;
	crosshair_dimensions.top = 0;
	crosshair_dimensions.bottom = 0;

	crosshair_color.r = 0;
	crosshair_color.g = 0;
	crosshair_color.b = 0;
}

void HudSpriteRenderer::EnableCustomCrosshair() {
	if (CustomCrosshairRenderingEnabled())
		return;
	gEngfuncs.pfnSetCrosshair = ScaledSetCrosshair;
	gEngfuncs.Con_DPrintf("Enabling custom crosshair rendering\n");
	gHUD.ResetCrosshair();
}

void HudSpriteRenderer::DisableCustomCrosshair() {
	if (!CustomCrosshairRenderingEnabled())
		return;
	gEngfuncs.pfnSetCrosshair = origSpriteEngfuncs.pfnSetCrosshair;
	gEngfuncs.Con_DPrintf("Disabling custom crosshair rendering\n");
	gHUD.ResetCrosshair();
}

bool HudSpriteRenderer::CustomCrosshairRenderingEnabled()
{
	return gEngfuncs.pfnSetCrosshair != origSpriteEngfuncs.pfnSetCrosshair;
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

int HudSpriteRenderer::VidInit() {
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

void HudSpriteRenderer::Init() {
	origSpriteEngfuncs.pfnSetCrosshair = gEngfuncs.pfnSetCrosshair;
}

void HudSpriteRenderer::HUD_Frame(double time) {
	(void)time;
	RecalcHUDScale();

	if (cachedHudScale != 1.0f) {
		EnableCustomCrosshair();
	} else {
		DisableCustomCrosshair();
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

void HudSpriteRenderer::SPR_DrawAuto(HSPRITE hPic, int r, int g, int b, int x, int y, const wrect_t *prc, int frame) {
	const model_t* model = gEngfuncs.GetSpritePointer(hPic);
	if (model->type == mod_sprite)
	{
		const msprite_t* spriteDef = (const msprite_t*)model->cache.data;
		if (spriteDef)
		{
			switch(spriteDef->texFormat)
			{
			case SPR_INDEXALPHA:
				SPR_Set(hPic, r, g, b);
				if (IsCustomScale()) {
					SPR_DrawInternal(frame, x, y, -1.0f, -1.0f, prc, kRenderTransAlpha);
				} else {
					::SPR_DrawHoles(frame, x, y, prc);
				}
				break;
			case SPR_ALPHTEST:
				SPR_Set(hPic, r, g, b);
				if (IsCustomScale()) {
					SPR_DrawInternal(frame, x, y, -1.0f, -1.0f, prc, kRenderTransTexture);
				} else {
					::SPR_DrawHoles(frame, x, y, prc);
				}
				break;
			default:
				SPR_DrawAdditive(hPic, r, g, b, x, y, prc, frame);
				break;
			}
		}
	}
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

void HudSpriteRenderer::SPR_DrawAdditiveWithAlphaScale(HSPRITE hPic, int r, int g, int b, int a, int x, int y, const wrect_t *prc, int frame) {
	ScaleColors(r, g, b, a);
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

void HudSpriteRenderer::SetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b)
{
	SetCrosshairData(hspr, rc, r, g, b);

	wrect_t crosshair_rect;
	crosshair_rect.left = 0;
	crosshair_rect.right = 0;
	crosshair_rect.top = 0;
	crosshair_rect.bottom = 0;

	origSpriteEngfuncs.pfnSetCrosshair(0, crosshair_rect, 0, 0, 0);
}

void HudSpriteRenderer::SetCrosshairData(HSPRITE hspr, wrect_t rc, int r, int g, int b)
{
	crosshair = hspr;
	crosshair_model = const_cast<model_t *>(gEngfuncs.GetSpritePointer(crosshair));
	crosshair_dimensions = rc;
	crosshair_color.r = r;
	crosshair_color.g = g;
	crosshair_color.b = b;
}

void HudSpriteRenderer::DrawCrosshair()
{
	// using original crosshair rendering
	if (!CustomCrosshairRenderingEnabled())
		return;

	if (crosshair <= 0)
		return;

	const int width = crosshair_dimensions.right - crosshair_dimensions.left;
	const int height = crosshair_dimensions.bottom - crosshair_dimensions.top;

	const int x = PerceviedScreenWidth() >> 1;
	const int y = PerceviedScreenHeight() >> 1;

	if (IsCustomScale())
	{
		SPR_SetInternal(crosshair, crosshair_color.r, crosshair_color.g, crosshair_color.b);
		SPR_DrawInternal(0, x - 0.5f * width, y - 0.5f * height, -1.0f, -1.0f, &crosshair_dimensions, kRenderTransTexture);
	}
	else
	{
		::SPR_Set(crosshair, crosshair_color.r, crosshair_color.g, crosshair_color.b);
		::SPR_DrawHoles(0, x - 0.5f * width, y - 0.5f * height, &crosshair_dimensions);
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
