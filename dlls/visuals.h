#pragma once
#ifndef VISUALS_H
#define VISUALS_H

#include "const_render.h"
#include "template_property_types.h"
#include "rapidjson/document.h"
#include "json_config.h"

#include <map>
#include <set>
#include <string>
#include "icase_compare.h"

struct BeamVisualParams
{
	constexpr BeamVisualParams() {}
	constexpr BeamVisualParams(int bWidth, int bNoise, int bScollRate = 0): width(bWidth), noise(bNoise), scollRate(bScollRate) {}
	int width = 0;
	int noise = 0;
	int scollRate = 0;
};

struct Visual
{
	enum
	{
		MODEL_DEFINED = (1 << 0),
		RENDERMODE_DEFINED = (1 << 1),
		COLOR_DEFINED = (1 << 2),
		ALPHA_DEFINED = (1 << 3),
		RENDERFX_DEFINED = (1 << 4),
		SCALE_DEFINED = (1 << 5),
		FRAMERATE_DEFINED = (1 << 6),
		BEAMWIDTH_DEFINED = (1 << 7),
		BEAMNOISE_DEFINED = (1 << 8),
		BEAMSCROLLRATE_DEFINED = (1 << 9),
		LIFE_DEFINED = (1 << 10),
		RADIUS_DEFINED = (1 << 11),
		BEAMFLAGS_DEFINED = (1 << 12),
		DECAY_DEFINED = (1 << 13),
	};

	const char* model = nullptr;
	int rendermode = kRenderNormal;
	Color3 rendercolor;
	int renderamt = 0;
	int renderfx = kRenderFxNone;
	FloatRange scale = 1.0f;
	FloatRange framerate = 0.0f;
	int beamWidth = 0;
	int beamNoise = 0;
	int beamScrollRate = 0;
	FloatRange life = 0.0f;
	IntRange radius = 0;
	int beamFlags = 0;
	float decay = 0.0f;

	int modelIndex = 0;

	inline void SetModel(const char* model)
	{
		this->model = model;
		MarkAsDefined(MODEL_DEFINED);
	}
	inline void SetRenderMode(int rendermode)
	{
		this->rendermode = rendermode;
		MarkAsDefined(RENDERMODE_DEFINED);
	}
	inline void SetColor(Color3 rendercolor)
	{
		this->rendercolor = rendercolor;
		MarkAsDefined(COLOR_DEFINED);
	}
	inline void SetAlpha(int alpha)
	{
		this->renderamt = alpha;
		MarkAsDefined(ALPHA_DEFINED);
	}
	inline void SetRenderFx(int renderfx)
	{
		this->renderfx = renderfx;
		MarkAsDefined(RENDERFX_DEFINED);
	}
	inline void SetScale(FloatRange scale)
	{
		this->scale = scale;
		MarkAsDefined(SCALE_DEFINED);
	}
	inline void SetFramerate(FloatRange framerate)
	{
		this->framerate = framerate;
		MarkAsDefined(FRAMERATE_DEFINED);
	}
	inline void SetBeamWidth(int width)
	{
		this->beamWidth = width;
		MarkAsDefined(BEAMWIDTH_DEFINED);
	}
	inline void SetBeamNoise(int noise)
	{
		this->beamNoise = noise;
		MarkAsDefined(BEAMNOISE_DEFINED);
	}
	inline void SetBeamScrollRate(int scrollRate)
	{
		this->beamScrollRate = scrollRate;
		MarkAsDefined(BEAMSCROLLRATE_DEFINED);
	}
	inline void SetLife(FloatRange life)
	{
		this->life = life;
		MarkAsDefined(LIFE_DEFINED);
	}
	inline void SetRadius(IntRange radius)
	{
		this->radius = radius;
		MarkAsDefined(RADIUS_DEFINED);
	}
	inline void SetBeamFlags(int flags)
	{
		this->beamFlags = flags;
		MarkAsDefined(BEAMFLAGS_DEFINED);
	}
	inline void SetDecay(float decay)
	{
		this->decay = decay;
		MarkAsDefined(DECAY_DEFINED);
	}

	inline bool HasModel() const {
		return model && *model;
	}
	inline bool HasDefined(int param) const {
		return (defined & param) != 0;
	}
	inline void MarkAsDefined(int param) {
		defined |= param;
	}
	void DoPrecache();
	void CompleteFrom(const Visual& visual);

private:
	int defined = 0;

	inline bool ShouldCompleteFrom(const Visual& visual, int param) const {
		return visual.HasDefined(param) && !HasDefined(param);
	}
};

struct NamedVisual : public Visual
{
	constexpr NamedVisual(const char* vName): name(vName) {}
	const char* name;
	operator const char* () const {
		return name;
	}
	const NamedVisual* mixin = nullptr;
};

struct BuildVisual
{
	BuildVisual(const char* name): visual(name) {}

	static BuildVisual Animated(const char* name)
	{
		return BuildVisual(name).Framerate(10.f);
	}
	static BuildVisual Spray(const char* name)
	{
		return BuildVisual(name).RenderMode(kRenderTransAlpha).Alpha(255).Framerate(0.5f);
	}

	NamedVisual Result() const {
		return visual;
	}
	operator NamedVisual() const {
		return visual;
	}

	inline BuildVisual& Model(const char* model) {
		visual.SetModel(model);
		return *this;
	}
	inline BuildVisual& RenderMode(int rendermode) {
		visual.SetRenderMode(rendermode);
		return *this;
	}
	inline BuildVisual& RenderColor(Color3 rendercolor) {
		visual.SetColor(rendercolor);
		return *this;
	}
	inline BuildVisual& RenderColor(int r, int g, int b) {
		return RenderColor(Color3(r, g, b));
	}
	inline BuildVisual& Alpha(int renderamt) {
		visual.SetAlpha(renderamt);
		return *this;
	}
	inline BuildVisual& RenderFx(int renderfx) {
		visual.SetRenderFx(renderfx);
		return *this;
	}
	inline BuildVisual& RenderProps(int rendermode, Color3 rendercolor, int renderamt, int renderfx)
	{
		return RenderMode(rendermode).RenderColor(rendercolor).Alpha(renderamt).RenderFx(renderfx);
	}
	inline BuildVisual& RenderProps(int rendermode, Color3 rendercolor, int renderamt)
	{
		return RenderMode(rendermode).RenderColor(rendercolor).Alpha(renderamt);
	}
	inline BuildVisual& RenderProps(int rendermode, Color3 rendercolor)
	{
		return RenderMode(rendermode).RenderColor(rendercolor);
	}
	inline BuildVisual& Scale(FloatRange scale)
	{
		visual.SetScale(scale);
		return *this;
	}
	inline BuildVisual& Framerate(FloatRange framerate)
	{
		visual.SetFramerate(framerate);
		return *this;
	}
	inline BuildVisual& BeamParams(int width, int noise, int scrollrate = 0)
	{
		return BeamWidth(width).BeamNoise(noise).BeamScrollRate(scrollrate);
	}
	inline BuildVisual& BeamWidth(int width)
	{
		visual.SetBeamWidth(width);
		return *this;
	}
	inline BuildVisual& BeamNoise(int noise)
	{
		visual.SetBeamNoise(noise);
		return *this;
	}
	inline BuildVisual& BeamScrollRate(int scrollrate)
	{
		visual.SetBeamScrollRate(scrollrate);
		return *this;
	}
	inline BuildVisual& Life(FloatRange life)
	{
		visual.SetLife(life);
		return *this;
	}
	inline BuildVisual& Radius(IntRange radius) {
		visual.SetRadius(radius);
		return *this;
	}
	inline BuildVisual& BeamFlags(int flags) {
		visual.SetBeamFlags(flags);
		return *this;
	}
	inline BuildVisual& Decay(float decay) {
		visual.SetDecay(decay);
		return *this;
	}
	inline BuildVisual& Mixin(const NamedVisual* mixin)
	{
		visual.mixin = mixin;
		return *this;
	}
private:
	NamedVisual visual;
};

class VisualSystem : public JSONConfig
{
public:
	void AddVisualFromJsonValue(const char* name, rapidjson::Value& value);
	void EnsureVisualExists(const std::string& name);
	const Visual* GetVisual(const char* name);
	const Visual* ProvideDefaultVisual(const char* name, const Visual& visual, bool doPrecache);
	const Visual* ProvideDefaultVisual(const char* name, const Visual& visual, const char* mixinName, const Visual& mixinVisual);
	void DumpVisuals() const;
	void DumpVisual(const char* name) const;
protected:
	const char* Schema() const;
	bool ReadFromDocument(rapidjson::Document& document, const char* fileName);
private:
	void DumpVisualImpl(const char* name, const Visual& visual) const;

	std::map<std::string, Visual, CaseInsensitiveCompare> _visuals;
	std::set<std::string> _modelStringSet;
	std::string _temp;
};

extern VisualSystem g_VisualSystem;

#endif
