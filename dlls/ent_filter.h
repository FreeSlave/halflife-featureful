#pragma once
#ifndef ENT_FILTER
#define ENT_FILTER

#include <string>
#include <vector>

#include "optional.h"
#include "tribool.h"

class EntityFilter
{
public:
	enum LifeState
	{
		ANY_LIFESTATE = 0,
		ALIVE = (1<<0),
		DYING = (1<<1),
		DEAD = (1<<2),
		NOT_DEAD = ALIVE | DYING,
		NOT_ALIVE = DYING | DEAD,
		NOT_DYING = ALIVE | DEAD,
		EVERY_LIFESTATE = ALIVE | DYING | DEAD,
	};

	struct BodyFilter
	{
		BodyFilter(int body): _body(body) {}
		BodyFilter(int bodyGroup, int submodel): _bodygroup(bodyGroup), _body(submodel) {}
		bool operator==(const BodyFilter& other) const {
			return _bodygroup == other._bodygroup && _body == other._body;
		}
		bool operator!=(const BodyFilter& other) const {
			return _bodygroup != other._bodygroup || _body != other._body;
		}

		int BodyGroup() const { return _bodygroup; }
		int Submodel() const { return _body; }
		bool IsGroupAndSubmodel() const { return _bodygroup >= 0 && _body >= 0; }
	private:
		int _bodygroup = -1;
		int _body = -1;
	};

	std::vector<std::string> classnames;
	std::vector<std::string> entTemplates;
	std::vector<int> classifications;
	bool sameClassname = false;
	bool sameEntTemplate = false;
	bool sameClassify = false;
	tribool isCombatCharacter;
	LifeState lifeState = ANY_LIFESTATE;
	std::vector<BodyFilter> bodyFilter;
	bool invertBodyCheck = false;
	bool negate = false;
};

#endif
