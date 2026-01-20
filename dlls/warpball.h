#pragma once
#ifndef WARPBALL_H
#define WARPBALL_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "const_render.h"
#include "const_sound.h"
#include "template_property_types.h"
#include "json_config.h"

#define WARPBALL_RED_DEFAULT 77
#define WARPBALL_GREEN_DEFAULT 210
#define WARPBALL_BLUE_DEFAULT 130

#define WARPBALL_BEAM_RED_DEFAULT 20
#define WARPBALL_BEAM_GREEN_DEFAULT 243
#define WARPBALL_BEAM_BLUE_DEFAULT 20

#define ALIEN_TELEPORT_SOUND "debris/alien_teleport.wav"

#define WARPBALL_SPRITE "sprites/fexplo1.spr"
#define WARPBALL_SPRITE2 "sprites/xflare1.spr"
#define WARPBALL_BEAM "sprites/lgtning.spr"
#define WARPBALL_SOUND1 "debris/beamstart2.wav"
#define WARPBALL_SOUND2 "debris/beamstart7.wav"

struct WarpballSound
{
	WarpballSound(): sound(), volume(1.0f), attenuation(ATTN_NORM), pitch(100) {}
	const char* sound;
	float volume;
	float attenuation;
	IntRange pitch;
};

struct WarpballSprite
{
	WarpballSprite():
		sprite(),
		color(),
		alpha(255),
		scale(1.0f),
		framerate(12.0f),
		rendermode(kRenderGlow),
		renderfx(kRenderFxNoDissipation)
	{}
	const char* sprite;
	Color3 color;
	int alpha;
	float scale;
	float framerate;
	int rendermode;
	int renderfx;
};

struct WarpballBeam
{
	WarpballBeam():
		sprite(),
		texture(0),
		color(),
		alpha(220),
		width(30),
		noise(65),
		life(0.5f, 1.6f) {}
	const char* sprite;
	int texture;
	Color3 color;
	int alpha;
	int width;
	int noise;
	FloatRange life;
};

struct WarpballLight
{
	WarpballLight():
		color(),
		radius(192),
		life(1.5f) {}
	Color3 color;
	int radius;
	float life;
	inline bool IsDefined() const {
		return life > 0.0 && radius > 0;
	}
};

typedef PlayerShake WarpballShake;

struct WarpballAiSound
{
	WarpballAiSound(): type(0), radius(192), duration(0.3f) {}
	int type;
	int radius;
	float duration;
	inline bool IsDefined() const {
		return type != 0 && duration > 0.0f && radius > 0;
	}
};

struct WarpballPosition
{
	WarpballPosition(): verticalShift(0.0f), defined(false) {}
	float verticalShift;
	bool defined;
	inline bool IsDefined() const {
		return defined;
	}
};

struct WarpballTemplate
{
	WarpballTemplate():
		beamRadius(192),
		beamCount(10, 20),
		spawnDelay(0.0f) {}
	WarpballSound sound1;
	WarpballSound sound2;

	WarpballSprite sprite1;
	WarpballSprite sprite2;

	WarpballBeam beam;
	int beamRadius;
	IntRange beamCount;

	WarpballLight light;
	WarpballShake shake;

	WarpballAiSound aiSound;
	float spawnDelay;
	WarpballPosition position;
};

struct WarpballTemplateCatalog : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	const WarpballTemplate* FindWarpballTemplate(const char* warpballName, const char* entityClassname = nullptr);
	void PrecacheWarpballTemplate(const char* name, const char* entityClassname);
	void DumpWarpballTemplates() const;

private:
	WarpballTemplate* GetWarpballTemplateMutable(const char* warpballName, const char* entityClassname);
	WarpballTemplate* GetWarpballTemplateByName(const char* warpballName);
	bool AddWarpballTemplate(const rapidjson::Value& allTemplatesJsonValue, const char* templateName, const rapidjson::Value& templateJsonValue, const char* fileName, std::vector<std::string> inheritanceChain = std::vector<std::string>());

	void AssignWarpballSound(WarpballSound& sound, const rapidjson::Value& soundJson);
	void AssignWarpballSprite(WarpballSprite& sprite, const rapidjson::Value& spriteJson);
	void AssignWarpballBeam(WarpballBeam& beam, const rapidjson::Value& beamJson);

	bool UpdateStringFromJson(const char*& str, const rapidjson::Value& jsonValue, const char* key);
	const char* MakeConstantString(const char* str);

	std::map<std::string, std::map<std::string, std::string> > _entityMappings;
	std::map<std::string, WarpballTemplate> _templates;
	std::set<std::string> _stringSet;
};

extern WarpballTemplateCatalog g_WarpballCatalog;

#if SERVER_DLL
class CBaseEntity;

void PlayWarpballEffect(CBaseEntity* pInitiator, const WarpballTemplate& warpballTemplate, const Vector& vecOrigin, edict_t* playSoundEnt);
void PlayWarpballEffect(const WarpballTemplate& warpballTemplate, const Vector& vecOrigin, edict_t* playSoundEnt);
#endif

#endif
