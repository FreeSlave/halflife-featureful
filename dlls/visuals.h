#pragma once
#ifndef VISUALS_H
#define VISUALS_H

#include "visual_object.h"
#include "json_config.h"

#include <map>
#include <set>
#include <string>
#include "icase_compare.h"

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
	inline BuildVisual& WaveType(int waveType) {
		visual.SetWaveType(waveType);
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
	void AddVisualFromJsonValue(const char* name, const rapidjson::Value& value);
	void EnsureVisualExists(const std::string& name);
	const Visual* GetVisual(const char* name);
	const Visual* ProvideDefaultVisual(const char* name, const Visual& visual, bool doPrecache);
	const Visual* ProvideDefaultVisual(const char* name, const Visual& visual, const char* mixinName, const Visual& mixinVisual);
	void DumpVisuals() const;
	void DumpVisual(const char* name) const;
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
private:
	void DumpVisualImpl(const char* name, const Visual& visual) const;

	std::map<std::string, Visual, CaseInsensitiveCompare> _visuals;
	std::set<std::string> _modelStringSet;
	std::string _temp;
};

extern VisualSystem g_VisualSystem;

#endif
