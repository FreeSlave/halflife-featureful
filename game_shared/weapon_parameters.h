#pragma once
#ifndef WEAPON_PARAMETERS_H
#define WEAPON_PARAMATERS_H

#include <array>
#include <string>
#include <type_traits>
#include <vector>

#include "common_types.h"
#include "fixed_string.h"
#include "fixed_vector.h"
#include "optional.h"
#include "template_property_types.h"
#include "tribool.h"
#include "vector.h"

#include "const_sound.h"
#include "cone_degrees.h"
#include "damageinfo.h"
#include "shake.h"
#include "shell_bounce.h"
#include "visual_object.h"

#define LOUD_GUN_VOLUME			1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME		200

#define	BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define	DIM_GUN_FLASH			128

enum class SecondaryFireType : byte
{
	DISABLED = 0,
	ALTERNATIVE_FIRE = 1,
	SWITCH_MODE = 2,
};

enum class WeaponEventFlags : int
{
	NOTHING = 0,
	ALTMODE = (1<<0),
	EMPTIED = (1<<1),
	ALTERNATING_EJECT = (1<<2)
};

template<typename T>
struct WeaponModeValue
{
	WeaponModeValue(): main{} {}
	explicit WeaponModeValue(const T& v): main(v) {}
	explicit WeaponModeValue(const T& v, const T& va): main(v), alt(va) {}
	WeaponModeValue& operator=(const T& v) {
		main = v;
		return *this;
	}
	WeaponModeValue& operator=(T&& v) {
		main = v;
		return *this;
	}

	T main;
	optional<T> alt;

	const T& Get(bool altMode) const {
		if (altMode && alt.has_value())
			return *alt;
		return main;
	}
	T& Materialize(bool altMode) {
		if (altMode)
		{
			if (!alt.has_value())
				alt = T{};
			return *alt;
		}
		return main;
	}
	bool IsDefined(bool altMode) const {
		if (altMode)
			return alt.has_value();
		else
			return true;
	}
};

template<typename T>
struct WeaponModeValueNonNegative
{
	static_assert((std::is_integral<T>::value || std::is_floating_point<T>::value) && std::is_signed<T>::value, "requires a signed number");

	WeaponModeValueNonNegative() {}
	explicit WeaponModeValueNonNegative(const T& v): main(v) {}
	explicit WeaponModeValueNonNegative(const T& v, const T& va): main(v), alt(va) {}
	WeaponModeValueNonNegative& operator=(const T& v) {
		main = v;
		return *this;
	}

	T main{-1};
	T alt{-1};

	const T& Get(bool altMode) const {
		return altMode && alt >= 0 ? alt : main;
	}
	T& Materialize(bool altMode) {
		return altMode ? alt : main;
	}
	bool IsDefined(bool altMode) const {
		if (altMode)
			return alt >= 0;
		else
			return true;
	}
};

template<>
struct WeaponModeValueNonNegative<FloatRange>
{
	WeaponModeValueNonNegative() {}
	explicit WeaponModeValueNonNegative(const FloatRange& v): main(v) {}
	explicit WeaponModeValueNonNegative(const FloatRange& v, const FloatRange& va): main(v), alt(va) {}
	WeaponModeValueNonNegative& operator=(const FloatRange& v) {
		main = v;
		return *this;
	}

	FloatRange main{-1};
	FloatRange alt{-1};

	const FloatRange& Get(bool altMode) const {
		return altMode && alt.min >= 0 ? alt : main;
	}
	FloatRange& Materialize(bool altMode) {
		return altMode ? alt : main;
	}
};

template<typename T>
struct WeaponModeValueEmptyAware
{
	WeaponModeValueEmptyAware(): main{} {}
	explicit WeaponModeValueEmptyAware(const T& v): main(v) {}
	explicit WeaponModeValueEmptyAware(const T& v, const T& va): main(v), alt(va) {}
	WeaponModeValueEmptyAware& operator=(const T& v) {
		main = v;
		return *this;
	}
	WeaponModeValueEmptyAware& operator=(T&& v) {
		main = v;
		return *this;
	}

	T main;
	optional<T> mainEmptied;
	optional<T> alt;
	optional<T> altEmptied;

	const T& Get(bool altMode, bool emptied) const {
		if (altMode)
		{
			if (emptied && altEmptied.has_value())
				return *altEmptied;
			if (alt.has_value())
				return *alt;
		}
		if (emptied && mainEmptied.has_value())
			return *mainEmptied;
		return main;
	}

	T& Materialize(bool altMode, bool emptied) {
		if (altMode)
		{
			if (emptied)
			{
				if (!altEmptied.has_value())
					altEmptied = T{};
				return *altEmptied;
			}
			else
			{
				if (!alt.has_value())
					alt = T{};
				return *alt;
			}
		}
		if (emptied)
		{
			if (!mainEmptied.has_value())
				mainEmptied = T{};
			return *mainEmptied;
		}
		return main;
	}
	void Reset(bool altMode, bool emptied) {
		if (altMode && emptied)
			altEmptied.reset();
		else if (altMode)
			alt.reset();
		else if (emptied)
			mainEmptied.reset();
	}
};

template<typename T>
struct WeaponModeValueEmptyAwareNonNegative
{
	static_assert((std::is_integral<T>::value || std::is_floating_point<T>::value) && std::is_signed<T>::value, "requires a signed number");

	WeaponModeValueEmptyAwareNonNegative() {}
	explicit WeaponModeValueEmptyAwareNonNegative(const T& v): main(v) {}
	explicit WeaponModeValueEmptyAwareNonNegative(const T& v, const T& va): main(v), alt(va) {}
	WeaponModeValueEmptyAwareNonNegative& operator=(const T& v) {
		main = v;
		return *this;
	}

	T main{-1};
	T mainEmptied{-1};
	T alt{-1};
	T altEmptied{-1};

	const T& Get(bool altMode, bool emptied) const {
		if (altMode)
		{
			if (emptied && altEmptied >= 0)
				return altEmptied;
			if (alt >= 0)
				return alt;
		}
		if (emptied && mainEmptied >= 0)
			return mainEmptied;
		return main;
	}

	T& Materialize(bool altMode, bool emptied) {
		if (altMode)
		{
			return emptied ? altEmptied : alt;
		}
		return emptied ? mainEmptied : main;
	}
	void Reset(bool altMode, bool emptied) {
		if (altMode && emptied)
			altEmptied = -1;
		else if (altMode)
			alt = -1;
		else if (emptied)
			mainEmptied = -1;
	}
};

template<>
struct WeaponModeValueEmptyAwareNonNegative<FloatRange>
{
	WeaponModeValueEmptyAwareNonNegative() {}
	explicit WeaponModeValueEmptyAwareNonNegative(const FloatRange& v): main(v) {}
	explicit WeaponModeValueEmptyAwareNonNegative(const FloatRange& v, const FloatRange& va): main(v), alt(va) {}
	WeaponModeValueEmptyAwareNonNegative& operator=(const FloatRange& v) {
		main = v;
		return *this;
	}

	FloatRange main{-1};
	FloatRange mainEmptied{-1};
	FloatRange alt{-1};
	FloatRange altEmptied{-1};

	const FloatRange& Get(bool altMode, bool emptied) const {
		if (altMode)
		{
			if (emptied && altEmptied.min >= 0)
				return altEmptied;
			if (alt.min >= 0)
				return alt;
		}
		if (emptied && mainEmptied.min >= 0)
			return mainEmptied;
		return main;
	}

	FloatRange& Materialize(bool altMode, bool emptied) {
		if (altMode)
		{
			return emptied ? altEmptied : alt;
		}
		return emptied ? mainEmptied : main;
	}
	void Reset(bool altMode, bool emptied) {
		if (altMode && emptied)
			altEmptied.min = -1;
		else if (altMode)
			alt.min = -1;
		else if (emptied)
			mainEmptied.min = -1;
	}
};

struct WeaponSoundScript
{
	WeaponSoundScript() {}
	WeaponSoundScript(int soundChannel): channel(soundChannel), volume(1.0f), attenuation(ATTN_NORM), pitch(PITCH_NORM) {}
	WeaponSoundScript(int soundChannel, std::initializer_list<const char*> sounds, FloatRange soundVolume, float soundAttenuation, IntRange soundPitch)
		: waves(sounds), channel(soundChannel), volume(soundVolume), attenuation(soundAttenuation), pitch(soundPitch)
	{}

	int channel{CHAN_WEAPON};
	fixed_vector<const char*, 4> waves;
	FloatRange volume{VOL_NORM};
	float attenuation{ATTN_NORM};
	IntRange pitch{PITCH_NORM};
	bool looped{false};

	const char* Wave() const;
	const char* Wave(int index) const;
};

struct PlayerMovementConditions
{
	static PlayerMovementConditions InAir() {
		PlayerMovementConditions c;
		c.inAir = true;
		return c;
	}
	static PlayerMovementConditions Ducking() {
		PlayerMovementConditions c;
		c.ducking = true;
		return c;
	}
	static PlayerMovementConditions Moving(float speed = 0.0f) {
		PlayerMovementConditions c;
		c.moving = speed;
		return c;
	}

	tribool inAir;
	tribool ducking;
	float moving{-1.0f};
};

struct WeaponSpreadRule
{
	static WeaponSpreadRule Static(float spreadX, float spreadY, const PlayerMovementConditions& conditions) {
		WeaponSpreadRule set;
		set._spreadBaseOrX = spreadX;
		set._spreadFactorOrY = spreadY;
		set._conditions = conditions;
		return set;
	}
	static WeaponSpreadRule Static(float spread, const PlayerMovementConditions& conditions) {
		return Static(spread, spread, conditions);
	}
	static WeaponSpreadRule Static(const Vector& cone, const PlayerMovementConditions& conditions) {
		return Static(cone.x, cone.y, conditions);
	}
	static WeaponSpreadRule Dynamic(float base, float factor, const PlayerMovementConditions& conditions)
	{
		WeaponSpreadRule set;
		set._spreadBaseOrX = base;
		set._spreadFactorOrY = factor;
		set._conditions = conditions;
		return set;
	}
	Vector GetStaticSpread() const {
		return Vector(_spreadBaseOrX, _spreadFactorOrY, 0.0f);
	}
	Vector GetDynamicSpread(float inaccuracy) const {
		float spread = _spreadBaseOrX + inaccuracy * _spreadFactorOrY;
		return Vector(spread, spread, 0.0f);
	}
	const PlayerMovementConditions& Conditions() const {
		return _conditions;
	}
private:
	float _spreadBaseOrX;
	float _spreadFactorOrY;
	PlayerMovementConditions _conditions;
};

struct WeaponSpread
{
public:
	enum Type : unsigned char
	{
		STATIC,
		QUBIC,
		QUADRATIC,
		RECOVERING,
	};

	typedef fixed_vector<WeaponSpreadRule, 5> RuleList;

	// HL static spread
	void SetStaticSpread(bool altMode, float spread);
	void SetStaticSpread(bool altMode, float spreadX, float spreadY);
	void SetStaticSpread(bool altMode, const Vector& cone);
	void SetStaticSpread(bool altMode, const RuleList& ruleList);
	// CS pistols
	void SetInaccuracyRecovering(bool altMode, float inaccuracy, float recoverTime, float factor, float maxInaccuracy, const RuleList& ruleList);
	// CS combat rifles/submachine guns
	void SetInaccuracyAuto(bool altMode, float inaccuracy, float factor, float inaccuracyShift, bool quadratic, float maxInaccuracy, const RuleList& ruleList);

	bool UsesDynamicInaccuracy(bool altMode) const;
	Type SpreadType(bool altMode) const {
		return _type.Get(altMode);
	}
	float GetDefaultInaccuracy(bool altMode) const;
	float GetNewInaccuracy(bool altMode, float currentInaccuracy, int shotsFired, float lastFireTime, float currentTime) const;

	const RuleList& GetRuleList(bool altMode) const;
private:
	enum Variable
	{
		Inaccuracy = 0,
		InaccuracyShift = 1,
		RecoverTime = 1,
		InaccuracyFactor = 2,
		MaxInaccuracy = 3,
	};

	WeaponModeValue<std::array<float, 4>> _variables;
	WeaponModeValue<RuleList> _ruleList;

	WeaponModeValue<Type> _type{STATIC};
};

// Kickback from CS
struct WeaponKickBack
{
	WeaponKickBack() {}
	WeaponKickBack(float vertical_base, float lateral_base, float vertical_modifier, float lateral_modifier, float vertical_max, float lateral_max, int direction_change_vertical, int direction_change_lateral):
		verticalBase(vertical_base), lateralBase(lateral_base),
		verticalModifier(vertical_modifier), lateralModifier(lateral_modifier),
		verticalMax(vertical_max), lateralMax(lateral_max),
		directionChangeVertical(direction_change_vertical),
		directionChangeLateral(direction_change_lateral)
	{}
	float verticalBase{0.0f};
	float lateralBase{0.0f};
	float verticalModifier{0.0f};
	float lateralModifier{0.0f};
	float verticalMax{0.0f};
	float lateralMax{0.0f};
	int directionChangeVertical{0};
	int directionChangeLateral{0};
};

struct WeaponKickBackRule
{
	PlayerMovementConditions conditions;
	WeaponKickBack kickBack;
};

struct WeaponKickBackProfile
{
public:
	typedef fixed_vector<WeaponKickBackRule, 4> RuleList;

	void SetKickBack(bool altMode, float punch);
	void SetKickBack(bool altMode, float punchX, float punchY);
	void SetKickBack(bool altMode, const WeaponKickBack& kickBack);
	void SetKickBack(bool altMode, const RuleList& rules);

	const RuleList& GetRuleList(bool altMode) const;
private:
	WeaponModeValue<RuleList> _rules;
};

struct PlayerSpeed
{
	float value{0.0f};
	bool isFactor{false};
	bool IsDefined() const {
		return value > 0.0f;
	}
};

struct WeaponParameters
{
	struct IdleAnim
	{
		IdleAnim() {}
		IdleAnim(int index, float probability, const FloatRange& dur):
			animIndex(index), chance(probability), duration(dur) {}
		IdleAnim(int index, float probability, const FloatRange& dur, const WeaponSoundScript& soundScript):
			animIndex(index), chance(probability), duration(dur), sound(soundScript) {}
		int animIndex;
		float chance;
		FloatRange duration;
		WeaponSoundScript sound{CHAN_STATIC};
	};
	typedef fixed_vector<IdleAnim, 5> IdleAnimArray;
	typedef fixed_vector<int, 4> FireAnimArray;

	struct Deploy
	{
		WeaponModeValueEmptyAwareNonNegative<short> animIndex;
		WeaponModeValueEmptyAwareNonNegative<float> duration{0.5f};
		WeaponModeValueEmptyAwareNonNegative<float> idleDelay{1.0f};
		WeaponModeValueEmptyAware<WeaponSoundScript> sound{CHAN_AUTO};
	};

	struct Reload
	{
		WeaponModeValueEmptyAwareNonNegative<short> animIndex{-1};
		WeaponModeValueEmptyAwareNonNegative<float> duration{1.5f};
		WeaponModeValueEmptyAwareNonNegative<float> attackDelay{0.0f};
		WeaponModeValueEmptyAwareNonNegative<FloatRange> idleDelay{3.0f};
		WeaponModeValueEmptyAware<WeaponSoundScript> sound{CHAN_ITEM};
		WeaponModeValueEmptyAware<bool> waitForRecoil{false};
		WeaponModeValueEmptyAwareNonNegative<float> suspendLaserSpotTime{0.0f};
		WeaponModeValueEmptyAwareNonNegative<int> ammoCount;
		WeaponModeValueEmptyAwareNonNegative<int> ammoCountMin{1};
	};

	struct StartReload
	{
		WeaponModeValueEmptyAwareNonNegative<short> animIndex{-1};
		WeaponModeValueEmptyAwareNonNegative<float> duration{0.0f};
	};

	struct EndReload
	{
		WeaponModeValueEmptyAwareNonNegative<short> animIndex{-1};
		WeaponModeValueEmptyAwareNonNegative<float> attackDelay{0.0f};
		WeaponModeValueEmptyAwareNonNegative<float> idleDelay{1.5f};
		WeaponModeValueEmptyAware<WeaponSoundScript> sound{CHAN_ITEM};
	};

	struct Holster
	{
		WeaponModeValueEmptyAwareNonNegative<int> animIndex{-1};
		float attackDelay{0.0f};
		FloatRange idleDelay{0.0f};
	};

	struct FirePhase
	{
		FirePhase(float r, float u) : side(r), up(u) {}
		FirePhase() {}
		float side{0.0f};
		float up{0.0f};
	};
	typedef std::vector<FirePhase> FirePhaseArray;

	struct ViewmodelBeam
	{
		Visual visual;
		short startAttachment{0};
		short endAttachment{0};
	};

	typedef std::vector<ViewmodelBeam> ViewmodelBeamArray;

	struct Fire
	{
		enum Type
		{
			NATIVE,
			BULLETS,
			MELEE,
			PROJECTILE
		};

		enum ADD_VELOCITY
		{
			DONT_ADD_VELOCITY,
			ADD_VELOCITY_ABSOLUTE,
			ADD_VELOCITY_PROJECTION
		};

		enum
		{
			GUNSMOKE_AUTO,
			GUNSMOKE_ALLOWED,
			GUNSMOKE_FORCED,
			GUNSMOKE_DISALLOWED,
		};

		enum
		{
			GRENADEPHYS_NO,
			GRENADEPHYS_AUTO,
			GRENADEPHYS_CLASSIC,
			GRENADEPHYS_ANNIVERSARY
		};

		WeaponModeValue<Type> fireType{NATIVE};
		WeaponModeValue<DamageInfoPatch> damageInfo;
		WeaponModeValueNonNegative<FloatRange> damageChargedFactor{0.0f};
		WeaponModeValueNonNegative<FloatRange> damageChargedMax{0.0f};
		WeaponModeValueNonNegative<float> subsequentSwingFactor{1.0f};

		WeaponModeValueEmptyAware<FireAnimArray> anims;
		WeaponModeValue<FireAnimArray> hitAnims;
		WeaponModeValue<FireAnimArray> chargeAnims;
		WeaponModeValueNonNegative<float> chargeTime{0.0f};
		WeaponModeValue<bool> chargeAmmoCheck{true};
		WeaponModeValue<bool> chargeUnderwaterCheck{true};
		WeaponModeValue<bool> chargeEachFire{false};
		WeaponModeValue<bool> chargeDamage{false};
		WeaponModeValue<bool> allowHolsterDuringCharge{true};
		WeaponModeValue<bool> laserSpotOnCharge{false};
		WeaponModeValue<FireAnimArray> cooldownAnims;
		WeaponModeValueNonNegative<float> cooldownTime{0.5f};
		WeaponModeValue<WeaponSoundScript> sound{CHAN_WEAPON};
		WeaponModeValue<WeaponSoundScript> soundAdditional{CHAN_ITEM};
		WeaponModeValue<WeaponSoundScript> hitBodySound{CHAN_ITEM};
		WeaponModeValue<WeaponSoundScript> hitWallSound{CHAN_ITEM};
		WeaponModeValue<WeaponSoundScript> emptySound{CHAN_WEAPON};
		WeaponModeValue<WeaponSoundScript> chargeSound{CHAN_WEAPON};
		WeaponModeValue<WeaponSoundScript> cooldownSound{CHAN_WEAPON};
		WeaponModeValue<bool> useStandardEmptySound{true};

		WeaponSpread spread{};
		WeaponModeValueNonNegative<float> cycleTime{0.2f};
		WeaponModeValueNonNegative<float> cycleTimeLastShot{0.0f};
		WeaponModeValueNonNegative<float> hitCycleTime{0.0f};
		WeaponModeValueEmptyAwareNonNegative<FloatRange> idleDelay{(FloatRange{10.0f, 15.0f})};
		WeaponModeValueNonNegative<short> ammoPerFire{1};
		WeaponModeValue<bool> allowUnderwater{true};
		WeaponModeValueNonNegative<short> bulletCount{1};
		WeaponModeValueNonNegative<short> tracerFreq{0};
		WeaponModeValueNonNegative<short> burstShots{0};
		WeaponModeValueNonNegative<float> burstInterval{0.1f};
		WeaponModeValue<bool> semiAuto{false};
		WeaponModeValue<bool> useSecondaryAmmo{false};

		WeaponModeValueNonNegative<float> autoAimDegree{0.0f};
		WeaponModeValue<bool> muzzleFlash{false};
		WeaponModeValueNonNegative<short> weaponVolume{0};
		WeaponModeValueNonNegative<short> bodyHitVolume{128};
		WeaponModeValueNonNegative<short> wallHitVolume{512};
		WeaponModeValueNonNegative<short> weaponFlash{0};
		WeaponModeValueNonNegative<int> extraSoundTypes{0};
		WeaponModeValueNonNegative<float> extraSoundTime{0.2f};

		WeaponModeValueNonNegative<float> delayAfterEmpty{0.15f};
		WeaponModeValueNonNegative<float> delayUnderwater{0.15f};
		WeaponModeValueNonNegative<float> pumpDelay{0.0f};
		WeaponModeValue<WeaponSoundScript> pumpSound{CHAN_ITEM};

		WeaponModeValueNonNegative<short> bulletDistance{8192};
		WeaponModeValueNonNegative<float> rangeModifier{1.0f};

		WeaponModeValue<FloatRange> clientPunchPitch;
		WeaponModeValue<FloatRange> clientPunchYaw;
		WeaponModeValueNonNegative<short> shellCount{1};
		WeaponModeValue<float> shellOffsetUp{0.0f};
		WeaponModeValue<float> shellOffsetSide{0.0f};
		WeaponModeValue<float> shellOffsetForward{0.0f};
		WeaponModeValue<short> shellAttachment{0};
		WeaponModeValue<const char*> shellModel{nullptr};
		WeaponModeValue<const char*> shellModelAlternating{nullptr};
		WeaponModeValueNonNegative<short> shellSound{TE_BOUNCE_SHELL};
		WeaponModeValue<bool> shellLeftSide;
		WeaponModeValue<FloatRange> shellVelocityUp{FloatRange{100, 150}};
		WeaponModeValue<FloatRange> shellVelocitySide{FloatRange{50, 70}};
		WeaponModeValue<FloatRange> shellVelocityForward{25};
		WeaponModeValueNonNegative<float> shellEjectDelay{0.0f};

		WeaponModeValueNonNegative<float> suspendLaserSpotTime{0.0f};

		WeaponKickBackProfile kickBack;
		WeaponModeValue<bool> kickBackOnHitOnly{false};

		WeaponModeValueNonNegative<float> pushbackForce{0.0f};
		WeaponModeValue<bool> pushbackVertical{false};

		WeaponModeValue<PlayerShake> shake;
		WeaponModeValue<PlayerShake> hitShake;

		WeaponModeValueNonNegative<float> smackDelay{0.2f};
		WeaponModeValue<bool> hitDecal{true};

		WeaponModeValue<float> sprayOffsetUp{-24.0f};
		WeaponModeValue<float> sprayOffsetSide{0.0f};
		WeaponModeValue<float> sprayOffsetForward{0.0f};
		WeaponModeValue<Visual> sprayVisual;
		WeaponModeValue<int> sprayCount{8};
		WeaponModeValue<int> spraySpeed{210};
		WeaponModeValue<float> spraySpread{0.25f};
		WeaponModeValue<int> sprayFlags{0};

		WeaponModeValue<bool> preventMovement{false};
		WeaponModeValue<PlayerSpeed> playerMaxSpeed;
		WeaponModeValue<PlayerSpeed> playerMaxSpeedOnCharge;

		WeaponModeValue<std::string> projectileName;
		WeaponModeValue<std::string> projectileEntTemplate;
		WeaponModeValue<float> projectileOffsetUp{0.0f};
		WeaponModeValue<float> projectileOffsetSide{0.0f};
		WeaponModeValue<float> projectileOffsetForward{0.0f};
		WeaponModeValue<bool> projectileRespectPunchangle{true};
		WeaponModeValue<bool> projectileAdjustToCross{true};
		WeaponModeValueNonNegative<float> projectileSpeed{0.0f};
		WeaponModeValue<ADD_VELOCITY> projectileAddCurrentVelocity{DONT_ADD_VELOCITY};
		WeaponModeValue<float> projectileDetonationTime{0.0f};
		WeaponModeValue<bool> projectileDetonationTimeSet{false};
		WeaponModeValue<bool> projectileDetonationCooked{false};
		WeaponModeValue<float> projectileDetonationTimeMin{0.0f};
		WeaponModeValue<FirePhaseArray> projectileFirePhases;
		WeaponModeValueNonNegative<short> projectileGrenadePhysics{GRENADEPHYS_NO};
		WeaponModeValue<FireAnimArray> projectileFarThrowAnims;
		WeaponModeValue<FireAnimArray> projectileFarthestThrowAnims;

		WeaponModeValue<ViewmodelBeamArray> viewmodelBeams;

		WeaponModeValue<std::vector<Visual>> gunSmokeVisuals;
		WeaponModeValueNonNegative<short> gunSmokePolicy{GUNSMOKE_ALLOWED};
		WeaponModeValueNonNegative<short> gunSmokeAttachment{0};
		WeaponModeValueNonNegative<short> gunSmokeForwardSpeed{20};
		WeaponModeValueNonNegative<float> gunSmokeRisingAcceleration{100.0f};
		WeaponModeValueNonNegative<float> gunSmokeStartRisingFrame{0.25f};
		WeaponModeValueNonNegative<float> gunSmokeInterval{0.1f};
	};

	struct Fade
	{
		Color3 color{};
		float fadeTime{0};
		float holdTime{0};
		int alpha{255};
		int flags{FFADE_IN};
	};

	struct Mode
	{
		int zoomFOV = 0;
		int zoomFOV2 = 0;
		bool resetZoomOnFire = false;
		bool resumeZoomAfterReset = false;
		bool hideViewModelOnZoom = false;
		bool toggleLaserSpot = false;

		WeaponSoundScript zoomSound{CHAN_ITEM};
		WeaponSoundScript zoomSound2{CHAN_ITEM};
		WeaponSoundScript unzoomSound{CHAN_ITEM};

		Fade zoomFade;

		WeaponModeValueNonNegative<float> attackDelay{0.0f};
		WeaponModeValueNonNegative<short> animIndex{-1};

		WeaponModeValueNonNegative<float> bodyDelay{0.0f};
		WeaponModeValueNonNegative<float> modeDelay{0.0f};
		WeaponModeValueNonNegative<short> endAnimIndex{-1};
		WeaponModeValueNonNegative<float> endAnimDuration{0.0f};
	};

	struct Recharge
	{
		WeaponModeValueNonNegative<float> interval{0};
		WeaponModeValueNonNegative<float> delayAfterFire{0};
		WeaponModeValue<bool> onlyWhenDeployed{false};
		WeaponModeValue<WeaponSoundScript> sound{CHAN_WEAPON};
	};

	struct DropAmmoEnt
	{
		std::string classname;
		std::string entTemplate;
		int amount{0};
	};

	Fire fire;
	Mode altMode;

	WeaponModeValueEmptyAware<IdleAnimArray> idleAnims;
	WeaponModeValueNonNegative<int> viewModelBody{0};
	std::vector<std::pair<int, int>> ammoToBody;
	bool reloadAutostart = false;
	bool manualReload = false;
	bool manualReloadContinueOnDeploy = true;
	bool manualReloadRestartOnDeploy = false;
	bool startInAltMode = false;
	bool primaryFirePrioritized = false;
	bool sharedChargeAndCooldown = false;

	Deploy deploy;

	Reload reload;
	StartReload startReload;
	EndReload endReload;

	Holster holster;

	SecondaryFireType secondaryFireType = SecondaryFireType::DISABLED;
	bool mirrorViewModel = false;

	bool startLaserSpot = false;
	bool laserSpotAttractRockets = false;
	bool laserSpotCheckActiveRockets = false;
	float laserSpotScale = 1.0f;
	WeaponSoundScript activateLaserSpotSound{CHAN_WEAPON};
	WeaponSoundScript deactivateLaserSpotSound{CHAN_WEAPON};

	Recharge recharge;

	IntRange initialAmmoAmount{0};
	int maxClip{0};
	fixed_string<32> ammoName;
	fixed_string<32> secondaryAmmoName;

	std::vector<std::string> modelSounds;
	bool modelSoundsDefined = false;
	std::string viewModel;
	std::string viewModelDetonator;
	std::string worldModel;
	std::string playerModel;
	std::string playerModelDetonator;
	fixed_string<32> playerAnimExt{"onehanded"};
	fixed_string<32> playerAnimExtDetonator{};
	int priority{0};
	bool worldModelAnimated{false};
	int worldModelSequence{0};

	fixed_string<32> toolIcon;
	float toolTriggerDelay{0.0f};
	WeaponSoundScript toolDenySound{CHAN_WEAPON};
	float toolDelayAfterDeny{0.1f};
	int toolIndex{-1};

	WeaponModeValue<PlayerSpeed> playerMaxSpeed;
	bool preventJump{false};
	bool exhausitble{false};

	DropAmmoEnt dropAmmo;
	DropAmmoEnt dropAmmoSecondary;

	const char* ViewModel() const {
		return viewModel.c_str();
	}
	const char* PlayerModel() const {
		if (playerModel.empty())
			return nullptr;
		return playerModel.c_str();
	}
	const char* DetonatorViewModel() const {
		if (viewModelDetonator.empty())
			return nullptr;
		return viewModelDetonator.c_str();
	}
	const char* DetonatorPlayerModel() const {
		if (playerModelDetonator.empty())
			return nullptr;
		return playerModelDetonator.c_str();
	}
	const char* PlayerAnimExt() const {
		return playerAnimExt.c_str();
	}
	const char* DetonatorPlayerAnimExt() const {
		return playerAnimExtDetonator.c_str();
	}

	bool IsUsableWithoutAmmo() const;
};

#endif
