#include "objecthint_manager.h"
#include "hud.h"
#include "event_api.h"
#include "color_utils.h"

#include <cassert>

static void SetParentEntityIndex(TEMPENTITY* te, int entindex)
{
	te->entity.curstate.iuser1 = entindex;
}

static int GetParentEntityIndex(const TEMPENTITY* te)
{
	return te->entity.curstate.iuser1;
}

static cl_entity_t* GetParentEntity(const TEMPENTITY* te)
{
	int entindex = GetParentEntityIndex(te);
	if (entindex)
		return gEngfuncs.GetEntityByIndex(entindex);
	return nullptr;
}

static void SetExpirationTime(TEMPENTITY* te, float t)
{
	te->entity.curstate.fuser1 = t;
	te->die = t + 1.0f;
}

static float GetExpirationTime(TEMPENTITY* te)
{
	return te->entity.curstate.fuser1;
}

static void SetScaleFactor(TEMPENTITY* te, float scale)
{
	te->entity.curstate.fuser2 = scale;
}

static void SetSizeVector(TEMPENTITY* te, const Vector& size)
{
	te->entity.curstate.vuser1 = size;
}

static void SetSpriteScaleFromSize(TEMPENTITY* te, const Vector& size)
{
	te->entity.curstate.scale = size.Length() * 0.015625f * te->entity.curstate.fuser2;
}

static void SetOffsetVector(TEMPENTITY* te, const Vector& offset)
{
	te->entity.curstate.vuser2 = offset;
}

static color24 UnpackRGB(int rgb)
{
	color24 color;
	color.r = (rgb & 0xFF0000) >> 16;
	color.g = (rgb & 0xFF00) >> 8;
	color.b = rgb & 0xFF;
	return color;
}

static void SetColor(TEMPENTITY* te, color24 color)
{
	te->entity.curstate.iuser2 = PackRGB(color.r,  color.g, color.b);
	if (color.r == 0 && color.g == 0 && color.b == 0)
		te->entity.curstate.rendercolor = UnpackRGB(gHUD.HUDColor());
	else
		te->entity.curstate.rendercolor = color;
}

static void UpdateObjectHintParams(TEMPENTITY* te, const ObjectHint &objectHint)
{
	SetColor(te, objectHint.color);

	SetScaleFactor(te, objectHint.scaleFactor);
	SetSpriteScaleFromSize(te, objectHint.size);
	SetSizeVector(te, objectHint.size);

	cl_entity_t* ent = GetParentEntity(te);
	if (ent)
		SetOffsetVector(te, ent->origin - objectHint.center);
	else
		SetOffsetVector(te, Vector(0,0,0));
}

void ObjectHintManager::SetHint(const ObjectHint &objectHint)
{
	TEMPENTITY* existingHint = ExistingCurrentHint(objectHint.entindex);
	if (existingHint)
	{
		if (strcmp(existingHint->entity.model->name, objectHint.sprite) == 0)
		{
			existingHint->entity.origin = objectHint.center;
			UpdateObjectHintParams(existingHint, objectHint);

			SetExpirationTime(existingHint, gEngfuncs.GetClientTime() + 0.2f);
			return;
		}
		else
		{
			existingHint->die = gEngfuncs.GetClientTime();
			_independentHints.erase(objectHint.entindex);
		}
	}

	if (!*objectHint.sprite)
		return;

	model_t* model = EnsureSpriteLoaded(objectHint.sprite);
	if (!model)
		return;

	Vector pos = objectHint.center;
	TEMPENTITY* te = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, model);
	if (!te)
		return;

	SetParentEntityIndex(te, objectHint.entindex);

	te->flags |= FTENT_PERSIST;
	te->entity.curstate.rendermode = kRenderGlow;
	te->entity.curstate.renderfx = kRenderFxNoDissipation;
	te->entity.curstate.renderamt = 255;

	UpdateObjectHintParams(te, objectHint);

	SetExpirationTime(te, gEngfuncs.GetClientTime() + 0.2f);

	if (objectHint.interactable)
	{
		RemoveInteractable();
		_interactableHint = te;
	}
	else
	{
		_independentHints[objectHint.entindex] = te;
	}
}

void ObjectHintManager::RemoveInteractable()
{
	if (_interactableHint)
	{
		_interactableHint->die = gEngfuncs.GetClientTime();
		_interactableHint = nullptr;
	}
}

void ObjectHintManager::Update()
{
	const float clientTime = gEngfuncs.GetClientTime();
	if (gHUD.m_pCvarDraw->value == 0.0f)
	{
		if (_independentHints.size())
		{
			for (auto it = _independentHints.begin(); it != _independentHints.end(); ++it)
			{
				it->second->die = clientTime;
			}
		}
		Clear();
		return;
	}

	if (_interactableHint)
	{
		if (GetExpirationTime(_interactableHint) <= clientTime)
			RemoveInteractable();
		else
			UpdateHint(_interactableHint);
	}
	for (auto it = _independentHints.begin(); it != _independentHints.end();)
	{
		if (GetExpirationTime(it->second) <= clientTime)
		{
			it->second->die = clientTime;
			_independentHints.erase(it++);
		}
		else
		{
			UpdateHint(it->second);
			++it;
		}
	}
}

void ObjectHintManager::UpdateHint(TEMPENTITY* te)
{
	cl_entity_t* ent = GetParentEntity(te);
	if (ent)
	{
		te->entity.origin = ent->origin - te->entity.curstate.vuser2;
	}
	if (te->entity.curstate.iuser2 == 0)
		te->entity.curstate.rendercolor = UnpackRGB(gHUD.HUDColor());
}

model_t* ObjectHintManager::EnsureSpriteLoaded(const char* name)
{
	assert(name && *name);
	std::string sname = name;
	auto it = _loadedSprites.find(sname);
	if (it != _loadedSprites.end())
	{
		return it->second;
	}
	model_t* model = const_cast<model_t*>(gEngfuncs.GetSpritePointer(gEngfuncs.pfnSPR_Load(name)));
	_loadedSprites[sname] = model;
	return model;
}

void ObjectHintManager::Clear()
{
	_loadedSprites.clear();
	_interactableHint = nullptr;
	_independentHints.clear();
}

TEMPENTITY* ObjectHintManager::ExistingCurrentHint(int entindex)
{
	if (_interactableHint && GetParentEntityIndex(_interactableHint) == entindex)
	{
		return _interactableHint;
	}
	auto it = _independentHints.find(entindex);
	if (it != _independentHints.end())
	{
		return it->second;
	}
	return nullptr;
}
