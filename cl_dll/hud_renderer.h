// Based on code from Xash3D and BugfixedHL-Rebased

#pragma once
#ifndef HUD_SCALED_H
#define HUD_SCALED_H

#include "cdll_int.h"
#include "com_model.h"
#include "ammo.h"

class HudSpriteRenderer {
public:
	HudSpriteRenderer();

	float GetHUDScale() const;
	bool IsCustomScale() const;

	int PerceviedScreenWidth();
	int PerceviedScreenHeight();

	int ScaleScreen(int value);
	int UnscaleScreen(int value);

	void SPR_SetInternal(HSPRITE hPic, int r, int g, int b);
	void SPR_DrawInternal(int frame, float x, float y, float width, float height, const wrect_t *dimensions, int mode);

	int VidInit();
	void Init();
	void HUD_Frame(double time);

	void SPR_Set(HSPRITE hPic, int r, int g, int b);
	void SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc);
	void SPR_DrawAdditive(HSPRITE hPic, int r, int g, int b, int x, int y, const wrect_t *prc, int frame = 0);

	void FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a);

	void SetCrosshair(const CrosshairSpriteData &crosshairData, int r, int g, int b);
	void SetCrosshairData(const CrosshairSpriteData& crosshairData, int r, int g, int b);
	void DrawCrosshair();

	HudSpriteRenderer& DefaultScale();
	HudSpriteRenderer& RelativeScale(float multiplier);
private:
	void RecalcHUDScale();

	HSPRITE sprite;
	model_t *sprite_model;
	color24 sprite_color;

	CrosshairSpriteData crosshair;
	color24 crosshair_color;
	bool customCrosshairRendering;
	float hudScalePrevValue;

	float hud_auto_scale_value;
	float cachedHudScale;
	float currentScale;

	int crosshairSizePrevValue;
};

#endif
