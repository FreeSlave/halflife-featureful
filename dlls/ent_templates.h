#pragma once
#ifndef ENT_TEMPLATES_H
#define ENT_TEMPLATES_H

#include "vector.h"
#include "visuals.h"
#include "soundscripts.h"
#include "json_config.h"
#include "fixed_string.h"
#include "optional.h"
#include "tribool.h"
#include "ent_filter.h"

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct ObjectSize
{
	ObjectSize() = default;
	ObjectSize(const Vector& minVec, const Vector& maxVec): mins(minVec), maxs(maxVec) {}
	Vector mins{0.0f, 0.0f, 0.0f};
	Vector maxs{0.0f, 0.0f, 0.0f};
};

struct SquadCapabilities
{
	tribool canRecruit;
	tribool denyRecruiting;
	tribool allowDifferentClassification;
	tribool requireSameClassname;
	tribool requireSameEntTemplate;
};

struct PainSoundRule
{
	FloatRange delay = 0.0f;
	float chance = 1.0f;
	float lowerBound = 0.0f;
	bool allowWhenDying = false;
};

enum class DamageTypeMatch
{
	INVALID = -1,
	ONE = 0,
	ALL,
	NONE,
	EXACT
};

enum class ValueComparison
{
	UNKNOWN,
	LESS,
	LESS_OR_EQUAL,
	GREATER,
	GREATER_OR_EQUAL
};

enum class ValueModifier
{
	UNKNOWN,
	SET,
	FACTOR,
	SUBSTRUCT,
	ADD
};

struct DropItemInfo
{
	std::string classname;
	std::string entTemplate;
	std::string pickupName;
	float chance = 1.0f;
	float weight = 1.0f;
};

struct DropItemInfoHandle
{
	DropItemInfoHandle(const DropItemInfo& item): chance(item.chance), weight(item.weight) {
		classname = item.classname.empty() ? nullptr : item.classname.c_str();
		entTemplate = item.entTemplate.empty() ? nullptr : item.entTemplate.c_str();
		pickupName = item.pickupName.empty() ? nullptr : item.pickupName.c_str();
	}

	const char* classname;
	const char* entTemplate;
	const char* pickupName;
	float chance;
	float weight;
};

struct DropItemSet
{
	std::vector<DropItemInfo> items;
	float maxWeight = 0.0f;

	static DropItemSet FromJSON(const rapidjson::Value& value);
};

struct ChildVariant
{
	std::string classname;
	std::map<std::string, std::string> parameters;
	float chance{1.0f};

	static ChildVariant FromJSON(const rapidjson::Value& value);
};

struct ChildrenInfo
{
	std::vector<ChildVariant> variants;
};

struct SkillReplacement
{
	enum
	{
		STRING,
		COMMON,
		DIFFICULTIES,
		MULTIPLIER
	};

	std::string replacement;
	FloatRange easy{0.0f};
	FloatRange medium{0.0f};
	FloatRange hard{0.0f};
	int type{STRING};
};

struct EntTemplate
{
public:
	struct PainSoundRule
	{
		optional<FloatRange> delay;
		optional<float> chance;
		optional<float> lowerBound;
		tribool allowWhenDying;
	};

	struct DamageInfo
	{
		enum
		{
			REPLACE_DAMAGE_TYPE,
			ADD_DAMAGE_TYPE,
		};

		optional<float> damage;
		optional<int> type;
		int typePolicy = ADD_DAMAGE_TYPE;
		tribool nonLethal;
		tribool ignoreArmor;
		optional<int> gibPolicy;
	};

	struct DamageConditions
	{
		optional<int> dmgType;
		DamageTypeMatch dmgTypeMatch = DamageTypeMatch::ONE;
		float dmg = 0.0f;
		ValueComparison dmgComparison = ValueComparison::UNKNOWN;
		optional<EntityFilter> inflictorFilter;
		optional<EntityFilter> attackerFilter;
		optional<EntityFilter> selfFilter;

		enum AttackAffinity
		{
			ANY_SOURCE = 0,
			ENEMY = (1<<0),
			FRIENDLY = (1<<1),
			SELF = (1<<2),
			NEUTRAL = (1<<3)
		};
		int attackAffinity = ANY_SOURCE;
		optional<int> gibPolicy;

		void UpdateFromJSON(const rapidjson::Value& value);
	};

	struct DamageInfoModifier
	{
		float dmg = 0.0f;
		ValueModifier dmgModifier = ValueModifier::UNKNOWN;
		float dmgMinThreshold = 0.0f;
		bool useHealthAsDmg = false;
		bool skip = false;
		tribool noBlood;
		optional<int> gibPolicy;

		void UpdateFromJSON(const rapidjson::Value& value);
	};

	static int DamageTypeFromJSON(const rapidjson::Value& value);
	static bool UpdateDamageInfoFromJSON(const rapidjson::Value& value, DamageInfo& damageInfo);
	static EntityFilter EntityFilterFromJSON(const rapidjson::Value& value);
	static int HitgroupFromJSON(const rapidjson::Value& value);
	static std::vector<int> HitgroupSetFromJSON(const rapidjson::Value& value);

	struct CheckMeleeAttack
	{
		optional<float> distance;
		optional<float> dot;
	};

	struct TraceHullAttack
	{
		struct PunchAngle
		{
			optional<float> pitch;
			optional<float> yaw;
			optional<float> roll;
		};

		struct Knock
		{
			optional<float> forward;
			optional<float> right;
			optional<float> up;
			tribool playerOnly;
		};

		optional<float> distance;
		optional<float> height;
		bool heightIsFactor = false;
		PunchAngle punchAngle;
		Knock knock;
		DamageInfo damageInfo;
		tribool spawnBlood;

		std::string hitSoundScript;
		std::string missSoundScript;
	};

	struct TouchAttack
	{
		DamageInfo damageInfo;
	};

	struct TraceAttackRule
	{
		struct Conditions : public DamageConditions
		{
			std::vector<int> hitgroups;
			bool invertHitgroupCheck = false;

			void UpdateFromJSON(const rapidjson::Value& value);
		};
		struct Modifier : public DamageInfoModifier
		{
			int hitgroup = -1;

			void UpdateFromJSON(const rapidjson::Value& value);
		};
		struct Effects
		{
			struct Ricochet
			{
				float chance = 0.0f;
				FloatRange scale = 1.0f;
				bool certainOnNewFrame = true;
			};
			struct Tracer
			{
				float chance = 1.0f;
				float variance = 0.3f;
				bool certainOnNewFrame = false;
			};

			optional<Ricochet> ricochet;
			optional<Tracer> tracer;
		};

		Conditions conditions;
		Modifier modifier;
		Effects effects;
		Effects thresholdEffects;

		static TraceAttackRule FromJSON(const rapidjson::Value& value);
	};

	struct TakeDamageRule
	{
		struct Conditions : public DamageConditions {};
		struct Modifier : public DamageInfoModifier {};

		Conditions conditions;
		Modifier modifier;

		static TakeDamageRule FromJSON(const rapidjson::Value& value);
	};

	const char* OwnVisualName() const;
	void SetOwnVisualName(const std::string& name) {
		_ownVisual = name;
	}

	const char* GibVisualName() const;
	void SetGibVisualName(const std::string& name) {
		_gibVisual = name;
	}

	const char* GetSoundScriptNameOverride(const char* name) const;
	void SetSoundScriptReplacement(const char* soundScript, const std::string& replacement);

	const char* GetVisualNameOverride(const char* name) const;
	void SetVisualReplacement(const char* visual, const std::string& replacement);

	const char* GetSoundReplacement(const char* originalSample) const;
	void SetSoundReplacement(const char* originalSample, const char* replacementSample);

	inline std::vector<std::string>::const_iterator PrecachedSoundsBegin() const {
		return _precachedSounds.begin();
	}
	inline std::vector<std::string>::const_iterator PrecachedSoundsEnd() const {
		return _precachedSounds.end();
	}
	void SetPrecachedSounds(std::vector<std::string>&& sounds);

	inline std::vector<std::string>::const_iterator PrecachedSoundScriptsBegin() const {
		return _precachedSoundScripts.begin();
	}
	inline std::vector<std::string>::const_iterator PrecachedSoundScriptsEnd() const {
		return _precachedSoundScripts.end();
	}
	void SetPrecachedSoundScripts(std::vector<std::string>&& soundScripts);
	void AddPrecachedSoundScript(const std::string& soundScript);

	inline bool AutoPrecacheSounds() const {
		return _autoprecachedSounds;
	}
	inline void SetAutoPrecacheSounds(bool b) {
		_autoprecachedSounds = b;
	}
	inline bool AutoPrecacheSoundScripts() const {
		return _autoprecachedSoundScripts;
	}
	inline void SetAutoPrecacheSoundScripts(bool b) {
		_autoprecachedSoundScripts = b;
	}

	bool IsClassifyDefined() const {
		return _classify.has_value();
	}
	int Classify() const {
		return _classify.value_or(0);
	}
	void SetClassify(int classify) {
		_classify = classify;
	}

	bool IsBloodDefined() const {
		return _bloodColor.has_value();
	}
	int BloodColor() const {
		return _bloodColor.value_or(-1);
	}
	void SetBloodColor(int bloodColor) {
		_bloodColor = bloodColor;
	}

	bool IsHealthDefined() const {
		return _health.has_value();
	}
	float Health() const {
		return _health.value_or(0.0f);
	}
	void SetHealth(float health) {
		_health = health;
	}

	bool IsFielfOfViewDefined() const {
		return _fieldOfView.has_value();
	}
	float FieldOfView() const {
		return _fieldOfView.value_or(0.0f);
	}
	void SetFieldOfView(float fieldOfView) {
		_fieldOfView = fieldOfView;
	}

	bool IsSizeDefined() const {
		return _size.has_value();
	}
	Vector MinSize() const {
		return _size.value_or(ObjectSize()).mins;
	}
	Vector MaxSize() const {
		return _size.value_or(ObjectSize()).maxs;
	}
	void SetSize(const Vector& minSize, const Vector& maxSize) {
		_size = ObjectSize(minSize, maxSize);
	}

	bool IsCollisionBoxDefined() const {
		return _collisionBox.has_value();
	}
	Vector CollisionBoxMin() const {
		return _collisionBox.value_or(ObjectSize()).mins;
	}
	Vector CollisionBoxMax() const {
		return _collisionBox.value_or(ObjectSize()).maxs;
	}
	void SetCollisionBox(const Vector& minSize, const Vector& maxSize)
	{
		_collisionBox = ObjectSize(minSize, maxSize);
	}

	bool IsSizeForGrappleDefined() const {
		return _sizeForGrapple.has_value();
	}
	int SizeForGrapple() const {
		return _sizeForGrapple.value_or(0);
	}
	void SetSizeForGrapple(int sizeForGrapple) {
		_sizeForGrapple = sizeForGrapple;
	}

	const char* SpeechPrefix() const;
	void SetSpeechPrefix(const std::string& speechPrefix) {
		_speechPrefix = speechPrefix;
	}

	SquadCapabilities GetSquadCapabilities() const {
		return _squadCapabilities;
	}
	void SetSquadCapabilities(const SquadCapabilities& caps) {
		_squadCapabilities = caps;
	}

	bool IsOpenDoorCapabilityDefined() const {
		return !indeterminate(_openDoorCapability);
	}
	bool CanOpenDoors() const {
		return (bool)_openDoorCapability;
	}
	void SetCanOpenDoors(bool enable) {
		_openDoorCapability = enable;
	}

	CheckMeleeAttack GetCheckMeleeAttack1() const {
		return _checkMeleeAttack1;
	}
	void SetCheckMeleeAttack1(const CheckMeleeAttack& check) {
		_checkMeleeAttack1 = check;
	}

	CheckMeleeAttack GetCheckMeleeAttack2() const {
		return _checkMeleeAttack2;
	}
	void SetCheckMeleeAttack2(const CheckMeleeAttack& check) {
		_checkMeleeAttack2 = check;
	}

	const TraceHullAttack* GetTraceHullAttackForEvent(int eventIndex) const {
		auto it = _traceHullAttacks.find(eventIndex);
		if (it == _traceHullAttacks.end())
			return nullptr;
		return &it->second;
	}

	void SetTraceHullAttackForEvent(int eventIndex, const TraceHullAttack& attack) {
		_traceHullAttacks[eventIndex] = attack;
	}

	TouchAttack GetTouchAttack() const {
		return _touchAttack;
	}
	void SetTouchAttack(const TouchAttack& touchAttack) {
		_touchAttack = touchAttack;
	}

	std::pair<std::vector<TraceAttackRule>::const_iterator, std::vector<TraceAttackRule>::const_iterator> TraceAttackRulesRange() const;
	void SetTraceAttackRules(std::vector<TraceAttackRule>&& traceAttackRules);
	bool HasCustomTraceAttackRules() const {
		return _traceAttackRulesDefined;
	}

	std::pair<std::vector<TakeDamageRule>::const_iterator, std::vector<TakeDamageRule>::const_iterator> TakeDamageRulesRange() const;
	void SetTakeDamageRules(std::vector<TakeDamageRule>&& takeDamageRules);
	bool HasCustomTakeDamageRules() const {
		return _takeDamageRulesDefined;
	}

	const ChildrenInfo& GetChildrenInfo() const {
		return _childrenInfo;
	}
	void SetChildrenInfo(ChildrenInfo&& childrenInfo) {
		_childrenInfo = childrenInfo;
	}

	const DropItemSet& GetLootDrop() const {
		return _lootDrop;
	}
	void SetLootDrop(DropItemSet&& dropItemSet) {
		_lootDrop = dropItemSet;
	}

	PainSoundRule GetPainSoundRule() const {
		return _painSoundRule;
	}
	void SetPainSoundRule(const PainSoundRule& rule) {
		_painSoundRule = rule;
	}
	void UpdatePainSoundRule(::PainSoundRule& rule) const;

	void SetSkillReplacement(const char* name, const SkillReplacement& replacement);
	const SkillReplacement* GetSkillReplacement(const char* name) const;

	void SetDisplayName(std::string&& name);
	void SetDisplayName(const char* name);
	const char* GetDisplayName() const;
private:
	static int ParseDamageType(const char* type);
	static int ParseGibPolicy(const char* gibPolicyName);

	mutable std::string tempString;
	std::map<std::string, std::string> _soundScripts;
	std::map<std::string, std::string> _visuals;
	std::string _ownVisual;
	std::string _gibVisual;

	std::map<std::string, std::string> _soundReplacements;
	std::vector<std::string> _precachedSounds;
	std::vector<std::string> _precachedSoundScripts;
	bool _autoprecachedSounds = false;
	bool _autoprecachedSoundScripts = false;

	optional<int> _classify;
	optional<int> _bloodColor;
	optional<float> _health;
	optional<float> _fieldOfView;
	optional<ObjectSize> _size;
	optional<ObjectSize> _collisionBox;
	optional<short> _sizeForGrapple;

	std::string _speechPrefix;
	SquadCapabilities _squadCapabilities;
	tribool _openDoorCapability;

	CheckMeleeAttack _checkMeleeAttack1;
	CheckMeleeAttack _checkMeleeAttack2;
	std::map<int, TraceHullAttack> _traceHullAttacks;
	TouchAttack _touchAttack;

	std::vector<TraceAttackRule> _traceAttackRules;
	bool _traceAttackRulesDefined = false;

	std::vector<TakeDamageRule> _takeDamageRules;
	bool _takeDamageRulesDefined = false;

	ChildrenInfo _childrenInfo;

	DropItemSet _lootDrop;

	PainSoundRule _painSoundRule;

	std::unordered_map<std::string, SkillReplacement> _skillReplacements;

	std::string _displayName;
};

class EntTemplateSystem : public JSONConfig
{
public:
	void SetSoundScriptSystem(SoundScriptSystem* soundScriptSystem) {
		_soundScriptSystem = soundScriptSystem;
	}
	void SetVisualSystem(VisualSystem* visualSystem) {
		_visualSystem = visualSystem;
	}
	bool AddTemplateFromJsonValue(const rapidjson::Value& allTemplatesJsonValue, const char* name, const rapidjson::Value& value, const char* fileName, std::vector<std::string> inheritanceChain = std::vector<std::string>());
	void AddTemplateFromJsonValue(const char* name, const rapidjson::Value& value, const char* fileName);
	const EntTemplate* GetTemplate(const char* name);
	void EnsureVisualReplacementForTemplate(const char* templateName, const char* visualName);
	void EnsureSoundScriptReplacementForTemplate(const char* templateName, const char* soundScriptName);
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
private:
	void AddTemplateFromJsonValueImpl(const std::string& templateName, const rapidjson::Value& value, EntTemplate& entTemplate);
	std::map<std::string, EntTemplate, CaseInsensitiveCompare> _entTemplates;
	std::string _temp;

	SoundScriptSystem* _soundScriptSystem = nullptr;
	VisualSystem* _visualSystem = nullptr;
};

extern EntTemplateSystem g_EntTemplateSystem;

#endif
