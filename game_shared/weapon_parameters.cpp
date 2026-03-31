#include "weapon_parameters.h"
#include "clamp.h"

template<>
WeaponSoundScript& WeaponModeValue<WeaponSoundScript>::Materialize(bool altMode)
{
	if (altMode)
	{
		if (!alt.has_value())
		{
			alt = WeaponSoundScript{main.channel};
		}
		return *alt;
	}
	return main;
}

void WeaponSpread::SetStaticSpread(bool altMode, float spread)
{
	SetStaticSpread(altMode, spread, spread);
}

void WeaponSpread::SetStaticSpread(bool altMode, float spreadX, float spreadY)
{
	auto& list = _ruleList.Materialize(altMode);
	list.clear();
	list.push_back(WeaponSpreadRule::Static(spreadX, spreadY, PlayerMovementConditions()));
	_type.Materialize(altMode) = STATIC;
}

void WeaponSpread::SetStaticSpread(bool altMode, const Vector& cone)
{
	SetStaticSpread(altMode, cone.x, cone.y);
}

void WeaponSpread::SetStaticSpread(bool altMode, const RuleList& ruleList)
{
	_ruleList.Materialize(altMode) = ruleList;
	_type.Materialize(altMode) = STATIC;
}

void WeaponSpread::SetInaccuracyRecovering(bool altMode, float inaccuracy, float recoverTime, float factor, float maxInaccuracy, const RuleList& ruleList)
{
	assert(ruleList.size());
	auto& vars = _variables.Materialize(altMode);
	vars[Inaccuracy] = inaccuracy;
	vars[RecoverTime] = recoverTime;
	vars[InaccuracyFactor] = factor;
	vars[MaxInaccuracy] = maxInaccuracy;
	_ruleList.Materialize(altMode) = ruleList;
	_type.Materialize(altMode) = RECOVERING;
}

void WeaponSpread::SetInaccuracyAuto(bool altMode, float inaccuracy, float factor, float inaccuracyShift, bool quadratic, float maxInaccuracy, const RuleList& ruleList)
{
	assert(ruleList.size());
	auto& vars = _variables.Materialize(altMode);
	vars[Inaccuracy] = inaccuracy;
	vars[InaccuracyShift] = inaccuracyShift;
	vars[InaccuracyFactor] = factor;
	vars[MaxInaccuracy] = maxInaccuracy;
	_ruleList.Materialize(altMode) = ruleList;
	_type.Materialize(altMode) = quadratic ? QUADRATIC : QUBIC;
}

bool WeaponSpread::UsesDynamicInaccuracy(bool altMode) const
{
	return _type.Get(altMode) != STATIC;
}

float WeaponSpread::GetDefaultInaccuracy(bool altMode) const
{
	return _variables.Get(altMode)[Inaccuracy];
}

float WeaponSpread::GetNewInaccuracy(bool altMode, float currentInaccuracy, int shotsFired, float lastFireTime, float currentTime) const
{
	auto& vars = _variables.Get(altMode);
	auto getInaccuracy = [&]() {
		switch (_type.Get(altMode)) {
		case QUBIC:
			return (shotsFired * shotsFired * shotsFired) * vars[InaccuracyFactor] + vars[Inaccuracy] + vars[InaccuracyShift];
		case QUADRATIC:
			return (shotsFired * shotsFired) * vars[InaccuracyFactor] + vars[Inaccuracy] + vars[InaccuracyShift];
		case RECOVERING:
			return lastFireTime == 0.0f ? currentInaccuracy : currentInaccuracy + (vars[RecoverTime] - (currentTime - lastFireTime)) * vars[InaccuracyFactor];
		default:
			return currentInaccuracy;
		}
	};
	return clamp(getInaccuracy(), vars[Inaccuracy], vars[MaxInaccuracy]);
}

const WeaponSpread::RuleList& WeaponSpread::GetRuleList(bool altMode) const
{
	return _ruleList.Get(altMode);
}

void WeaponKickBackProfile::SetKickBack(bool altMode, float punch)
{
	SetKickBack(altMode, punch, punch);
}

void WeaponKickBackProfile::SetKickBack(bool altMode, float punchX, float punchY)
{
	WeaponKickBack kickBack;
	kickBack.verticalBase = punchX;
	kickBack.lateralBase = punchY;
	SetKickBack(altMode, kickBack);
}

void WeaponKickBackProfile::SetKickBack(bool altMode, const WeaponKickBack &kickBack)
{
	auto& rules = _rules.Materialize(altMode);
	rules.clear();
	rules.push_back(WeaponKickBackRule{PlayerMovementConditions(), kickBack});
}

void WeaponKickBackProfile::SetKickBack(bool altMode, const RuleList &rules)
{
	_rules.Materialize(altMode) = rules;
}

const WeaponKickBackProfile::RuleList& WeaponKickBackProfile::GetRuleList(bool altMode) const
{
	return _rules.Get(altMode);
}

bool WeaponParameters::IsUsableWithoutAmmo() const
{
	if (fire.ammoPerFire.Get(false) <= 0)
		return true;

	if (secondaryFireType != SecondaryFireType::DISABLED && fire.ammoPerFire.Get(true) <= 0)
		return true;

	if ((fire.useSecondaryAmmo.Get(false) || fire.useSecondaryAmmo.Get(true)) && secondaryAmmoName.empty())
		return true;

	return false;
}
