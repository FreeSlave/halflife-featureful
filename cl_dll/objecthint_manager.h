#pragma once
#ifndef HUD_OBJECTHINT_H
#define HUD_OBJECTHINT_H

#include "cl_dll.h"
#include "com_model.h"
#include "r_efx.h"

#include <map>
#include <string>

struct ObjectHint
{
	int entindex;
	color24 color;
	float scaleFactor;
	Vector center;
	Vector size;
	char sprite[64];
	bool interactable;
};

class ObjectHintManager
{
public:
	void SetHint(const ObjectHint& objectHint);
	void RemoveInteractable();
	void Update();
	model_t* EnsureSpriteLoaded(const char* name);
	void Clear();

private:
	void UpdateHint(TEMPENTITY* te);
	TEMPENTITY* ExistingCurrentHint(int entindex);

	std::map<std::string, model_t*> _loadedSprites;
	TEMPENTITY* _interactableHint;
	std::map<int, TEMPENTITY*> _independentHints;
};

#endif
