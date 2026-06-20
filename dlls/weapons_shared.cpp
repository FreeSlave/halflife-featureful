#include "extdll.h"
#include "util.h"
#include "random_utils.h"
#include "cbase.h"
#include "player.h"
#include "player_capabilities.h"
#include "weapons.h"
#include "clamp.h"
#include "weapon_templates.h"

#if !CLIENT_DLL
#include "ammo_amounts.h"
#include "gamerules.h"
#include "game.h"
#include "hornet.h"
#include "ggrenade.h"
#include "spore.h"
#include "skill.h"
#endif

WeaponInfo& AccessWeaponInfo(int id)
{
	// TODO: protect against out of bounds access?
	static WeaponInfo arr[MAX_WEAPONS] = {};
	return arr[id];
}

static void SetDefaultWeaponParameters()
{
	for(int i=1; i<MAX_WEAPONS; ++i)
	{
		WeaponInfo& info = AccessWeaponInfo(i);
		if (info.pWeapon)
			info.params = info.pWeapon->GetDefaultParameters();
	}
}

void SetWeaponParameters()
{
	SetDefaultWeaponParameters();
	g_WeaponTemplateSystem.ReadFromFile("templates/weapons.json");
}

int GetWeaponIdByName(const char* classname)
{
	for (int i=0; i<MAX_WEAPONS; ++i)
	{
		const WeaponInfo& info = AccessWeaponInfo(i);
		if (info.classname && strcmp(classname, info.classname) == 0)
		{
			return info.id;
		}
	}
	return -1;
}

WeaponParameters* AccessWeaponParameters(const char* name)
{
	for (int i=0; i<MAX_WEAPONS; ++i)
	{
		WeaponInfo& info = AccessWeaponInfo(i);
		if (info.classname && strcmp(name, info.classname) == 0)
		{
			return &info.params;
		}
	}
	return nullptr;
}

const WeaponParameters& GetWeaponParameters(int id)
{
	return AccessWeaponInfo(id).params;
}

int CBasePlayer::GetMaxAmmo(int ammoIndex)
{
	if (ammoIndex > 0 && ammoIndex < MAX_AMMO_TYPES)
	{
		if (m_maxAmmoOverride[ammoIndex] > 0)
			return m_maxAmmoOverride[ammoIndex];
		else if (m_maxAmmoOverride[ammoIndex] < 0)
			return 0;
	}
	return g_AmmoRegistry.GetMaxAmmo(ammoIndex);
}

bool ShouldMirrorViewModel(int id)
{
	if (id > 0 && id < MAX_WEAPONS)
	{
		return GetWeaponParameters(id).mirrorViewModel;
	}
	return false;
}

WeaponRegistrator::WeaponRegistrator(const char* classname, CBasePlayerWeapon* pWeapon)
{
	const int id = pWeapon->WeaponId();
	WeaponInfo& info = AccessWeaponInfo(id);
	if (info.classname)
	{
		return;
	}
	info.id = id;
	info.classname = classname;
	info.pWeapon = pWeapon;
}

const WeaponParameters& CBasePlayerWeapon::MyParameters() const
{
	return AccessWeaponInfo(WeaponId()).params;
}

const char* WeaponSoundScript::Wave() const
{
	if (waves.size() > 1)
	{
		return waves[RandomInt(0, waves.size() - 1)];
	}
	else if (waves.size() == 1)
	{
		return waves[0];
	}
	return nullptr;
}

const char* WeaponSoundScript::Wave(int index) const
{
	if (index >= 0 && static_cast<unsigned int>(index) < waves.size())
		return waves[index];
	return nullptr;
}

void CBasePlayerWeapon::PrecacheModelSounds()
{
	const WeaponParameters& params = MyParameters();

	if (params.modelSounds.size() || params.modelSoundsDefined)
	{
		for (const auto& sound : params.modelSounds)
		{
			::PRECACHE_SOUND(sound.c_str());
		}
	}
	else
	{
		PrecacheDefaultModelSounds();
	}
}

void CBasePlayerWeapon::PrecacheDropAmmo()
{
#if !CLIENT_DLL
	const WeaponParameters& params = MyParameters();

	auto precacheDropAmmo = [this](const WeaponParameters::DropAmmoEnt& dropAmmo, int& amount)
	{
		if (dropAmmo.classname.empty())
			return;

		if (FStrEq(STRING(pev->classname), dropAmmo.classname.c_str()))
			return; // avoid recursion

		EntityOverrides entityOverrides;
		entityOverrides.entTemplate = dropAmmo.entTemplate.empty() ? iStringNull : MAKE_STRING(dropAmmo.entTemplate.c_str());

		UTIL_PrecacheAmmoEntity(dropAmmo.classname.c_str(), amount, entityOverrides);
	};

	precacheDropAmmo(params.dropAmmo, m_dropAmmoAmount);
	precacheDropAmmo(params.dropAmmoSecondary, m_dropSecondaryAmmoAmount);
#endif
}

void CBasePlayerWeapon::SendWeaponAnim(int iAnim)
{
	SendWeaponAnim(iAnim, ViewModelBody());
}

bool CBasePlayerWeapon::CanDeploy()
{
	const bool usesAmmo = UsesAmmo();

	if (!usesAmmo)
	{
		// this weapon doesn't use ammo, can always deploy.
		return true;
	}

	bool bHasAmmo = false;

	if( usesAmmo )
	{
		bHasAmmo |= ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 );
	}
	if( UsesSecondaryAmmo() )
	{
		bHasAmmo |= ( m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] > 0 );
	}
	if( m_iClip > 0 )
	{
		bHasAmmo |= true;
	}
	if( !bHasAmmo )
	{
		const WeaponParameters& params = MyParameters();
		return params.IsUsableWithoutAmmo();
	}

	return true;
}

bool CBasePlayerWeapon::DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{
	const bool usesClip = UsesClip();
	if (!usesClip)
		return false;

	const bool usesAmmo = UsesAmmo();

	if (usesAmmo)
	{
		const int j = Q_min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]);
		if (j <= 0)
			return false;
	}
	else
	{
		if (m_iClip >= iClipSize)
			return false;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	if (iAnim >= 0)
		SendWeaponAnim( iAnim, body );

	m_fInReload = true;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	return true;
}

bool CBasePlayerWeapon::DefaultClipReload(int iAnim, float fDelay, int body)
{
	return DefaultReload(m_iMaxClip, iAnim, fDelay, body);
}

void CBasePlayerWeapon::ReloadClipNow(int ammoCountPerReload)
{
	if (!UsesClip())
		return;

	const int maxClip = iMaxClip();

	if (UsesAmmo())
	{
		int j = Q_min(maxClip - m_iClip, m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]);
		if (ammoCountPerReload > 0)
			j = Q_min(j, ammoCountPerReload);

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= j;
	}
	else if (m_iClip < maxClip)
	{
		if (ammoCountPerReload > 0)
		{
			m_iClip += ammoCountPerReload;
			m_iClip = Q_min(maxClip, m_iClip);
		}
		else
		{
			m_iClip = maxClip;
		}
	}
}

void CBasePlayerWeapon::ResetEmptySound()
{
	m_iPlayEmptySound = true;
}

bool CanAttack( float attack_time, float curtime, bool isPredicted )
{
#ifdef CLIENT_DLL
	return attack_time <= 0.0f;
#else
#if CLIENT_WEAPONS
	if( !isPredicted )
#else
	if( 1 )
#endif
	{
		return ( attack_time <= curtime );
	}
	else
	{
		return ( (static_cast<int>(::floor(attack_time * 1000.0f)) * 1000.0f) <= 0.0f);
	}
#endif
}

void CBasePlayerWeapon::ItemPostFrame()
{
	const WeaponParameters& params = MyParameters();
	const bool altMode = InAltMode();
	const bool empty = Emptied();

	if( ( m_fInReload ) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		const int ammoCount = params.reload.ammoCount.Get(altMode, empty);

		// complete the reload.
		ReloadClipNow(ammoCount);

		m_fInReload = false;
	}

#ifndef CLIENT_DLL
	if( !(m_pPlayer->pev->button & IN_ATTACK ) )
	{
		m_flLastFireTime = 0.0f;
	}
#endif

	const bool hasSecondaryFire = params.secondaryFireType != SecondaryFireType::DISABLED;
	if (!hasSecondaryFire)
	{
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}

	// Wait for end reload finish
	if (!params.manualReload && m_fInSpecialReload == 1)
	{
		m_pPlayer->pev->button &= ~(IN_ATTACK|IN_ATTACK2);
	}

	const bool canPrimaryAttackNow = CanAttack( m_flNextPrimaryAttack, gpGlobals->time, UseDecrement() );
	const bool isAttackSuppressed = FBitSet(m_pPlayer->m_suppressedCapabilities, PLAYER_SUPPRESS_ATTACK) || FBitSet(m_pPlayer->pev->flags, FL_FROZEN);

	if (params.primaryFirePrioritized)
	{
		if (canPrimaryAttackNow && (m_pPlayer->pev->button & IN_ATTACK) && (m_pPlayer->pev->button & IN_ATTACK2))
		{
			m_pPlayer->pev->button &= ~IN_ATTACK2;
		}
	}

	if( ( m_pPlayer->pev->button & IN_ATTACK2 ) && CanAttack( m_flNextSecondaryAttack, gpGlobals->time, UseDecrement() ) )
	{
		if( UsesSecondaryAmmo() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = true;
		}

		if (!isAttackSuppressed)
			SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && canPrimaryAttackNow )
	{
		if (UsesAmmo())
		{
			if (UsesClip())
			{
				m_fFireOnEmpty = m_iClip == 0;
			}
			else
			{
				m_fFireOnEmpty = !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()];
			}
		}

		if (!isAttackSuppressed)
			PrimaryAttack();
	}
	else if( m_pPlayer->pev->button & IN_RELOAD && UsesClip() && !m_fInReload  && !FBitSet(m_pPlayer->pev->flags, FL_FROZEN) )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if( !( m_pPlayer->pev->button & ( IN_ATTACK | IN_ATTACK2 ) ) )
	{
		// no fire buttons down
		m_fFireOnEmpty = false;

		UpdateInaccuracy();

#ifndef CLIENT_DLL
		if( !IsUseable() && m_flNextPrimaryAttack < ( UseDecrement() ? 0.0f : gpGlobals->time ) )
		{
			// weapon isn't useable, switch.
			if( g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
			{
				m_flNextPrimaryAttack = ( UseDecrement() ? 0.0f : gpGlobals->time ) + 0.3f;
				return;
			}
		}
		else
#endif
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if( UsesClip() && m_iClip == 0 &&
#ifdef CLIENT_DLL
					m_flNextPrimaryAttack <= 0.0f
#else
					m_flNextPrimaryAttack < ( UseDecrement() ? 0.0f : gpGlobals->time )
#endif
					)
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}
}

void CBasePlayerWeapon::SetInitialAmmoAmount()
{
	const int initialAmmoAmount = RandomizeNumberFromRange(MyParameters().initialAmmoAmount);

	if (m_iDefaultAmmo == 0)
	{
#if !CLIENT_DLL
		const int amount = g_AmmoAmounts.AmountForAmmoEnt(STRING(pev->classname));
		if (amount >= 0)
			m_iDefaultAmmo = amount;
		else
#endif
			m_iDefaultAmmo = initialAmmoAmount;
	}
	else if (m_iDefaultAmmo < 0)
	{
		m_iDefaultAmmo = 0;
	}
	else
	{
		// pass, already initialized
	}
}

void CBasePlayerWeapon::InitMaxClip()
{
	m_iMaxClip = MyParameters().maxClip;
	if (!m_iMaxClip)
	{
		m_iMaxClip = WEAPON_NOCLIP;
		m_iClip = -1;
	}
}

int CBasePlayerWeapon::iMaxClip()
{
	return m_iMaxClip;
}

int CBasePlayerWeapon::iWeight()
{
	const WeaponParameters& params = MyParameters();
	return params.priority;
}

int CBasePlayerWeapon::PrimaryAmmoIndex() const
{
	if (m_iPrimaryAmmoType <= 0 || m_iPrimaryAmmoType >= MAX_AMMO_TYPES)
	{
		ALERT(at_error, "Weapon %d accessing primary ammo with invalid type %d!\n", WeaponId(), m_iPrimaryAmmoType);
		return 0;
	}
	return m_iPrimaryAmmoType;
}

int CBasePlayerWeapon::SecondaryAmmoIndex() const
{
	if (m_iSecondaryAmmoType <= 0 || m_iSecondaryAmmoType >= MAX_AMMO_TYPES)
	{
		ALERT(at_error, "Weapon %d accessing secondary ammo with invalid type %d!\n", WeaponId(), m_iSecondaryAmmoType);
		return 0;
	}
	return m_iSecondaryAmmoType;
}

bool CBasePlayerWeapon::CanReload()
{
	if (!UsesClip())
		return false;

	if (m_iClip >= m_iMaxClip)
		return false;

	const WeaponParameters& params = MyParameters();
	const bool altMode = InAltMode();
	const bool empty = Emptied();

	if (UsesAmmo())
	{
		const int ammoCountMin = params.reload.ammoCountMin.Get(altMode, empty);
		return m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] >= ammoCountMin;
	}
	return true;
}

bool CBasePlayerWeapon::UsesClip()
{
	return m_iMaxClip != WEAPON_NOCLIP;
}

bool CBasePlayerWeapon::HasAmmoToFire(int ammo)
{
	if (UsesClip())
	{
		return m_iClip >= ammo;
	}
	else if (UsesAmmo())
	{
		return m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] >= ammo;
	}
	return true;
}

bool CBasePlayerWeapon::IsOutOfAmmo()
{
	if (!UsesAmmo())
		return false;
	if (UsesClip())
	{
		return m_iClip <= 0 && m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] < 1;
	}
	else
	{
		return m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] < 1;
	}
}

void CBasePlayerWeapon::CheckOutOfAmmo()
{
	if (IsOutOfAmmo())
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
}

void CBasePlayerWeapon::CheckOutOfSecondaryAmmo()
{
	if (m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] < 1)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
}

void CBasePlayerWeapon::SpendAmmo(int ammo)
{
	if (UsesClip())
	{
		m_iClip -= ammo;
		m_iClip = Q_max(0, m_iClip);
	}
	else if (UsesAmmo())
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= ammo;
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = Q_max(0, m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]);
	}
}

bool CBasePlayerWeapon::Emptied()
{
	if (UsesClip())
	{
		return m_iClip == 0;
	}
	else if (UsesAmmo())
	{
		return m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] == 0;
	}
	return false;
}

void CBasePlayerWeapon::PlayWeaponSoundScript(const WeaponSoundScript& soundScript, float volumeFactor)
{
	const char* soundWave = soundScript.Wave();
	if (soundWave)
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), soundScript.channel, soundWave, RandomizeNumberFromRange(soundScript.volume) * volumeFactor, soundScript.attenuation, 0, RandomizeNumberFromRange(soundScript.pitch));
	}
}

void CBasePlayerWeapon::SetWorldModelProps()
{
	const WeaponParameters& params = MyParameters();
	if (params.worldModelAnimated)
	{
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0f;
	}
	if (pev->sequence == 0 && params.worldModelSequence > 0)
	{
		pev->sequence = params.worldModelSequence;
	}
}

static bool PlayerMatchesConditions(CBasePlayer* pPlayer, const PlayerMovementConditions& conditions)
{
	if (!indeterminate(conditions.inAir))
	{
		const bool isOnGround = FBitSet(pPlayer->pev->flags, FL_ONGROUND);
		if (conditions.inAir && isOnGround)
			return false;
		if (!conditions.inAir && !isOnGround)
			return false;
	}
	if (!indeterminate(conditions.ducking))
	{
		const bool isDucking = FBitSet(pPlayer->pev->flags, FL_DUCKING);
		if (conditions.ducking && !isDucking)
			return false;
		if (!conditions.ducking && isDucking)
			return false;
	}
	if (conditions.moving >= 0.0f)
	{
		if (!pPlayer->pev->velocity.IsLength2DGreaterThan(conditions.moving))
		{
			return false;
		}
	}
	return true;
}

void CConfigurableWeapon::Spawn()
{
	const WeaponParameters& params = MyParameters();
	Precache();
	SetMyModel(params.worldModel.c_str());

	SetInitialAmmoAmount();
	InitMaxClip();

	FallInit();// get ready to fall down.

	ResetInaccuracy();
	m_bDelayFire = true;

	if (params.startLaserSpot)
		m_bLaserActive = true;

	SetWorldModelProps();
}

static void PrecacheWeaponSoundScript(const WeaponSoundScript& soundScript)
{
	for (auto& wave : soundScript.waves)
	{
		::PRECACHE_SOUND(wave);
	}
}

static void PrecacheWeaponSoundScript(const optional<WeaponSoundScript> soundScript)
{
	if (soundScript.has_value())
	{
		PrecacheWeaponSoundScript(*soundScript);
	}
}

static void PrecacheWeaponSoundScript(const WeaponModeValue<WeaponSoundScript>& soundScript)
{
	PrecacheWeaponSoundScript(soundScript.main);
	PrecacheWeaponSoundScript(soundScript.alt);
}

static void PrecacheWeaponSoundScript(const WeaponModeValueEmptyAware<WeaponSoundScript>& soundScript)
{
	PrecacheWeaponSoundScript(soundScript.main);
	PrecacheWeaponSoundScript(soundScript.mainEmptied);
	PrecacheWeaponSoundScript(soundScript.alt);
	PrecacheWeaponSoundScript(soundScript.altEmptied);
}

static int PrecacheWeaponParamModel(const char* model)
{
	if (model)
		return PRECACHE_MODEL(model);
	return 0;
}

static int PrecacheWeaponParamModel(optional<const char*> model)
{
	if (model.has_value())
		return PrecacheWeaponParamModel(*model);
	return 0;
}

const char* GetRealProjectileClassname(const char* projectileName, int& variant)
{
#if !CLIENT_DLL
	if (FStrEq(projectileName, "hornet dart"))
	{
		variant = CHornet::DART;
		return "hornet";
	}
	else if (FStrEq(projectileName, "AR grenade"))
	{
		variant = CGrenade::CONTACT;
		return "grenade";
	}
	else if (FStrEq(projectileName, "hand grenade"))
	{
		variant = CGrenade::TIMED;
		return "grenade";
	}
	else if (FStrEq(projectileName, "spore rocket"))
	{
		variant = CSpore::ROCKET;
		return "spore";
	}
	else if (FStrEq(projectileName, "spore bouncy"))
	{
		variant = CSpore::GRENADE_LAUNCHED;
		return "spore";
	}
	else if (FStrEq(projectileName, "crossbow_bolt explosive"))
	{
		variant = 1;
		return "crossbow_bolt";
	}
	else if (FStrEq(projectileName, "rpg_rocket straight"))
	{
		variant = 1;
		return "rpg_rocket";
	}
#endif
	variant = 0;
	return projectileName;
}

void CConfigurableWeapon::Precache()
{
	PrecacheWeaponModels();

	PrecacheCommonEvent();

	const WeaponParameters& params = MyParameters();

	auto precacheIdleAnims = [](const WeaponParameters::IdleAnimArray& idleAnims) {
		for (const auto& idleAnim : idleAnims)
		{
			PrecacheWeaponSoundScript(idleAnim.sound);
		}
	};

	precacheIdleAnims(params.idleAnims.main);
	if (params.idleAnims.mainEmptied.has_value())
	{
		precacheIdleAnims(*params.idleAnims.mainEmptied);
	}
	if (params.idleAnims.alt.has_value())
	{
		precacheIdleAnims(*params.idleAnims.alt);
	}
	if (params.idleAnims.altEmptied.has_value())
	{
		precacheIdleAnims(*params.idleAnims.altEmptied);
	}

	PrecacheWeaponSoundScript(params.fire.sound);
	PrecacheWeaponSoundScript(params.fire.soundAdditional);
	PrecacheWeaponSoundScript(params.fire.hitBodySound);
	PrecacheWeaponSoundScript(params.fire.hitWallSound);
	PrecacheWeaponSoundScript(params.fire.emptySound);
	PrecacheWeaponSoundScript(params.fire.chargeSound);
	PrecacheWeaponSoundScript(params.fire.cooldownSound);
	PrecacheWeaponSoundScript(params.fire.pumpSound);

	shellModel = PrecacheWeaponParamModel(params.fire.shellModel.main);
	shellModel2 = PrecacheWeaponParamModel(params.fire.shellModel.alt);
	shellModelAlternate = PrecacheWeaponParamModel(params.fire.shellModelAlternating.main);
	shellModelAlternate2 = PrecacheWeaponParamModel(params.fire.shellModelAlternating.alt);

	PrecacheWeaponSoundScript(params.deploy.sound);
	PrecacheWeaponSoundScript(params.reload.sound);
	PrecacheWeaponSoundScript(params.endReload.sound);

	PrecacheWeaponSoundScript(params.altMode.zoomSound);
	PrecacheWeaponSoundScript(params.altMode.zoomSound2);
	PrecacheWeaponSoundScript(params.altMode.unzoomSound);

	PrecacheWeaponSoundScript(params.activateLaserSpotSound);
	PrecacheWeaponSoundScript(params.deactivateLaserSpotSound);

	PrecacheWeaponSoundScript(params.recharge.sound);

	PrecacheWeaponSoundScript(params.toolDenySound);

	PrecacheModelSounds();

	if (params.startLaserSpot || params.altMode.toggleLaserSpot)
	{
		UTIL_PrecacheOther("laser_spot");
	}

	auto precacheProjectile = [&params](bool altMode) {
		const auto& projectileName = params.fire.projectileName.Get(altMode);
		if (!projectileName.empty())
		{
			int projectileVariant;
			EntityOverrides entityOverrides;
			const auto& projectileEntTemplate = params.fire.projectileEntTemplate.Get(altMode);
			if (!projectileEntTemplate.empty())
				entityOverrides.entTemplate = MAKE_STRING(projectileEntTemplate.c_str());
			UTIL_PrecacheOther(GetRealProjectileClassname(projectileName.c_str(), projectileVariant), entityOverrides);
		}
	};

	precacheProjectile(false);
	precacheProjectile(true);

	auto precacheSprayVisual = [&params](bool altMode) {
		if (!params.fire.sprayVisual.IsDefined(altMode))
			return;

		const Visual& sprayVisual = params.fire.sprayVisual.Get(altMode);
		if (sprayVisual.HasModel())
		{
			PRECACHE_MODEL(sprayVisual.model);
		}
	};
	precacheSprayVisual(false);
	precacheSprayVisual(true);

	auto precacheViewmodelBeams = [&params](bool altMode) {
		if (!params.fire.viewmodelBeams.IsDefined(altMode))
			return;

		for (const auto& beam : params.fire.viewmodelBeams.Get(altMode))
		{
			if (beam.visual.HasModel())
			{
				PRECACHE_MODEL(beam.visual.model);
			}
		}
	};
	precacheViewmodelBeams(false);
	precacheViewmodelBeams(true);

	PrecacheDropAmmo();
}

bool CConfigurableWeapon::Deploy()
{
	UpdateTape();
	return PerformDeploy();
}

bool CConfigurableWeapon::PerformDeploy()
{
	const WeaponParameters& params = MyParameters();

	const bool altMode = InAltMode();
	const bool emptied = Emptied();

	if (CanRechargeAmmo())
	{
		if (params.recharge.onlyWhenDeployed.Get(altMode))
			m_flRechargeTime = gpGlobals->time + params.recharge.interval.Get(altMode);
	}

	const WeaponParameters::Deploy& deploy = params.deploy;

	const float duration = deploy.duration.Get(altMode, emptied);
	float idleDelay = deploy.idleDelay.Get(altMode, emptied);
	idleDelay = Q_max(idleDelay, duration);

	if (m_switchingBody && m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase())
	{
		m_switchingBody = false;
		SetBody(params.viewModelBody.Get(!m_wasInAltModeBeforeSwitchingBody));
	}

	if (m_switchingMode)
	{
		const int animIndex = params.altMode.endAnimIndex.Get(m_inAltMode);
		if (animIndex >= 0)
		{
			m_switchingMode = false;
			m_inAltMode = !m_inAltMode;
		}
	}

	const bool result = DefaultDeploy(ViewModelToDeploy(params.ViewModel()), params.PlayerModel(), deploy.animIndex.Get(altMode, emptied), params.PlayerAnimExt(), ViewModelBody(), duration, idleDelay);
	if (result)
	{
#if !CLIENT_DLL
		if (params.preventJump)
		{
			m_pPlayer->m_suppressedCapabilities |= PLAYER_SUPPRESS_JUMP_DUE_TO_WEAPON;
		}
#endif
		PlayWeaponSoundScript(deploy.sound.Get(altMode, emptied));

		m_iSwingMode = 0;
		ResetBurst();

		if (params.manualReload)
		{
			if (params.manualReloadRestartOnDeploy)
			{
				if (m_fInSpecialReload)
					m_shouldRestartReloading = true;
				m_fInSpecialReload = 0;
			}
		}
		else
		{
			m_fInSpecialReload = 0;
		}

		m_bDelayFire = true;
		ResetInaccuracy();

		SetChargingAttack(false);
		m_shouldPlayCooldown = false;
		m_chargingAltFire = false;

		if (!m_playedFirstDeploy && params.startInAltMode && !m_inAltMode)
		{
			if (params.altMode.zoomFOV == 0)
				SwitchMode(SwitchModeReason::FirstDeploy);
		}

		m_playedFirstDeploy = true;
	}
	return result;
}

extern void EjectBrass(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype );

void CConfigurableWeapon::EjectBrassLate()
{
#ifndef CLIENT_DLL
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;
	const bool altMode = m_wasInAltModeBeforeEjectLate;
	const int shellId = altMode ? shellModel2 : shellModel;

	if (!shellId)
		return;

	const bool shellLeftSide = fire.shellLeftSide.Get(altMode);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	const Vector vecUp = RandomizeNumberFromRange(fire.shellVelocityUp.Get(altMode)) * gpGlobals->v_up;
	const Vector vecRight = RandomizeNumberFromRange(fire.shellVelocitySide.Get(altMode)) * gpGlobals->v_right;
	const Vector vecForward = RandomizeNumberFromRange(fire.shellVelocityForward.Get(altMode)) * gpGlobals->v_forward;

	const Vector vecShellVelocity = m_pPlayer->pev->velocity + (shellLeftSide ? -vecRight : vecRight) + vecUp + vecForward;
	int soundType = fire.shellSound.Get(altMode);

	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs +
				gpGlobals->v_up * fire.shellOffsetUp.Get(altMode) +
				gpGlobals->v_forward * fire.shellOffsetForward.Get(altMode) +
				gpGlobals->v_right * fire.shellOffsetSide.Get(altMode),
				vecShellVelocity, pev->angles.y, shellId, soundType);
#endif
}

void CConfigurableWeapon::ItemPostFrame()
{
	UpdateSpot();

	const WeaponParameters& params = MyParameters();

	if (!m_fInReload)
	{
		if (UsesClip())
			m_iVisibleClip = m_iClip;
		else if (UsesAmmo())
			m_iVisibleClip = m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()];
	}

#if !CLIENT_DLL
	if (m_toolTriggerTime != 0.0f && m_toolTriggerTime < gpGlobals->time)
	{
		CBaseEntity* triggerEnt = CBaseEntity::OwnInstance(m_pPlayer->m_UseToolTriggers[params.toolIndex]);
		if (triggerEnt)
			triggerEnt->Use(m_pPlayer, m_pPlayer, USE_TOGGLE, 0.0f);
		m_toolTriggerTime = 0.0f;
	}
#endif

	if (m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		PlayWeaponSoundScript(params.fire.pumpSound.Get(m_pumpAltMode));
		m_flPumpTime = 0;
	}

	if (m_pPlayer->m_bResumeZoom && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if (HasAmmoToFire())
		{
			// Don't zoom again if has nothing to fire - the reload unzooms anyway
			// This way we avoid playing the zoom fade if there's any
			SetZoom(m_pPlayer->m_iLastZoom);
		}
		m_pPlayer->m_bResumeZoom = false;
	}

	if (m_pPlayer->m_flEjectBrass != 0.0f && m_pPlayer->m_flEjectBrass <= gpGlobals->time)
	{
		m_pPlayer->m_flEjectBrass = 0.0f;
		EjectBrassLate();
	}

	bool shouldSwitchBodyNow = false;
	bool shouldSwitchModeNow = false;

	if (m_switchingBody)
	{
		if (m_switchingMode)
		{
			const float bodySwitchDelay = params.altMode.bodyDelay.Get(m_wasInAltModeBeforeSwitchingBody);
			const float modeSwitchDelay = params.altMode.modeDelay.Get(m_wasInAltModeBeforeSwitchingBody);

			if (bodySwitchDelay <= modeSwitchDelay)
			{
				m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + modeSwitchDelay - bodySwitchDelay;
				shouldSwitchBodyNow = true;
			}
		}
		else
			shouldSwitchBodyNow = true;
	}

	if (m_switchingMode)
	{
		if (m_switchingBody)
		{
			const float bodySwitchDelay = params.altMode.bodyDelay.Get(m_inAltMode);
			const float modeSwitchDelay = params.altMode.modeDelay.Get(m_inAltMode);

			if (modeSwitchDelay <= bodySwitchDelay)
			{
				m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + bodySwitchDelay - modeSwitchDelay;
				shouldSwitchModeNow = true;
			}
		}
		else
			shouldSwitchModeNow = true;
	}

	if (shouldSwitchBodyNow)
	{
		m_switchingBody = false;
		SetBody(params.viewModelBody.Get(!m_wasInAltModeBeforeSwitchingBody));
	}

	if (shouldSwitchModeNow)
	{
		m_switchingMode = false;

		const int animIndex = params.altMode.endAnimIndex.Get(m_inAltMode);
		if (animIndex >= 0)
		{
			SendWeaponAnim(animIndex);
			const float animDuration = params.altMode.endAnimDuration.Get(m_inAltMode);
			m_pPlayer->m_flNextAttack = Q_max(UTIL_WeaponTimeBase() + animDuration, m_pPlayer->m_flNextAttack);
		}

		m_inAltMode = !m_inAltMode;
	}

	if (m_burstTime != 0.0f)
	{
#if CLIENT_DLL
		// TODO: Sometimes client updates the time slower than the server
		// use decrementing variable instead?
		if (gpGlobals->time + 0.01f > m_burstTime)
#else
		if (gpGlobals->time > m_burstTime)
#endif
		{
			FireRemaining();
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CConfigurableWeapon::UpdateInaccuracy()
{
	if (m_bDelayFire)
	{
		m_bDelayFire = false;

		if (m_iShotsFired > 15)
		{
			m_iShotsFired = 15;
		}
		m_flDecreaseShotsFired = gpGlobals->time + 0.4f;
	}

	const bool altMode = m_lastShotWasInAltMode;

	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;

	const bool semiAuto = altMode ? (fire.semiAuto.alt.has_value() ? *fire.semiAuto.alt : false) : fire.semiAuto.main;
	if (semiAuto)
	{
		m_iShotsFired = 0;
	}
	else
	{
		if (m_iShotsFired > 0 && m_flDecreaseShotsFired < gpGlobals->time)
		{
			m_flDecreaseShotsFired = gpGlobals->time + 0.0225f;
			m_iShotsFired--;

			if (m_iShotsFired == 0)
			{
				m_flInaccuracy = fire.spread.GetDefaultInaccuracy(altMode);
			}
		}
	}
}

void CConfigurableWeapon::SendScreenShake(const PlayerShake& shake)
{
#ifndef CLIENT_DLL
	if (shake.IsDefined())
	{
		if (FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
			UTIL_ScreenShakeToClient( m_pPlayer->edict(), shake.amplitude, shake.frequency, shake.duration );
	}
#endif
}

void CConfigurableWeapon::ApplyMyKickBack(bool altMode)
{
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;

	auto& kickBackRules = fire.kickBack.GetRuleList(altMode);
	if (kickBackRules.size())
	{
		WeaponKickBack kickBack = kickBackRules.back().kickBack;
		if (kickBackRules.size() > 1)
		{
			for (const auto& rule : kickBackRules)
			{
				if (PlayerMatchesConditions(m_pPlayer, rule.conditions))
				{
					kickBack = rule.kickBack;
					break;
				}
			}
		}
		KickBack(kickBack);
	}
}

bool CConfigurableWeapon::SelectAndSendFireAnimation(const WeaponParameters::FireAnimArray &arr)
{
	if (arr.empty())
		return false;

	int anim = -1;
	if (arr.size() == 1)
	{
		anim = arr.front();
	}
	else
	{
		anim = arr[RandomizeNumberFromRange_Shared(m_pPlayer->random_seed, 0, arr.size()-1)];
	}
	SendWeaponAnim(anim);
	return true;
}

bool CConfigurableWeapon::PerformCooldown(bool altMode)
{
	if (m_shouldPlayCooldown)
	{
		m_shouldPlayCooldown = false;
		m_chargingAltFire = false;

		const WeaponParameters& params = MyParameters();

		const WeaponSoundScript& primarySoundScript = params.fire.sound.Get(false);
		const WeaponSoundScript& secondarySoundScript = params.fire.sound.Get(true);

		if (primarySoundScript.looped || secondarySoundScript.looped)
		{
			PLAYBACK_EVENT_FULL(PlaybackFlags(), m_pPlayer->edict(), GetPlaybackEvent(altMode), 0.0, g_vecZero, g_vecZero,
								0.0f, 0.0f, PackIParam1(altMode, Emptied()), PackIParam2(), 1, 0);
		}

		PlayWeaponSoundScript(params.fire.cooldownSound.Get(altMode));
		if (SelectAndSendFireAnimation(params.fire.cooldownAnims.Get(altMode)))
		{
			m_flTimeWeaponIdle = Q_max(UTIL_WeaponTimeBase() + params.fire.cooldownTime.Get(altMode), m_flTimeWeaponIdle);
			return true;
		}
	}
	return false;
}

Vector CConfigurableWeapon::GetSpread(bool altMode)
{
	bool useInaccuracy = false;
	auto getSpread = [this, &useInaccuracy, altMode]()
	{
		const WeaponParameters& params = MyParameters();
		const WeaponParameters::Fire& fire = params.fire;

		const auto& ruleList = fire.spread.GetRuleList(altMode);
		if (fire.spread.UsesDynamicInaccuracy(altMode))
		{
			useInaccuracy = true;
			if (ruleList.size())
			{
				for (const auto& rule : ruleList)
				{
					if (PlayerMatchesConditions(m_pPlayer, rule.Conditions()))
					{
						return rule.GetDynamicSpread(m_flInaccuracy);
					}
				}
				return ruleList.back().GetDynamicSpread(m_flInaccuracy); // pick the last as default option
			}
			else
			{
				const float spread = 0.02 * m_flInaccuracy;
				return Vector(spread, spread, 0.0f);
			}

		}
		else
		{
			if (ruleList.size())
			{
				for (const auto& rule : ruleList)
				{
					if (PlayerMatchesConditions(m_pPlayer, rule.Conditions()))
					{
						return rule.GetStaticSpread();
					}
				}
				return ruleList.back().GetStaticSpread(); // pick the last as default option
			}

		}
		return Vector{};
	};

	const Vector vecSpread = getSpread();
	if (useInaccuracy)
	{
		//ALERT(at_console, "Firing with inaccuracy %g. Spread %g, %g\n", m_flInaccuracy, vecSpread.x, vecSpread.y);
	}
	else
	{
		//ALERT(at_console, "Firing with spread %g, %g. AltMode: %s\n", vecSpread.x, vecSpread.y, altMode ? "yes" : "no");
	}
	return vecSpread;
}

void CConfigurableWeapon::PerformWeaponFire(bool altMode)
{
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;

	if (CanRechargeAmmo())
	{
		Reload();
	}

	if (!params.manualReload && m_fInSpecialReload)
		return;

	const bool semiAuto = altMode ? (fire.semiAuto.alt.has_value() ? *fire.semiAuto.alt : false) : fire.semiAuto.main;
	if (semiAuto)
	{
		if (m_iShotsFired >= 1)
			return;
	}

	const auto fireType = fire.fireType.Get(altMode);

	const float chargeTime = fire.chargeTime.Get(m_chargingAttack ? m_chargingAltFire : altMode);
	const bool chargedAttack = fire.chargedAttack.Get(altMode);

	if ((chargeTime > 0.0f || chargedAttack)
		&& fireType != WeaponParameters::Fire::MELEE) // charged melee attacks are handled differently
	{
		if (!m_chargingAttack)
		{
			m_chargingAltFire = altMode;
			m_shouldPlayCooldown = true;
			SetChargingAttack(true);
			SelectAndSendFireAnimation(fire.chargeAnims.Get(altMode));

			PlayWeaponSoundScript(fire.chargeSound.Get(altMode));

			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + chargeTime;
			m_flNextPrimaryAttack = GetNextAttackDelay(chargeTime);

			if (chargedAttack)
			{
				m_chargeStartTime = gpGlobals->time;
			}

			return;
		}

		if (!params.sharedChargeAndCooldown && m_chargingAltFire != altMode)
		{
			const bool chargingAltFire = m_chargingAltFire;
			if (PerformCooldown(chargingAltFire))
			{
				SetChargingAttack(false);
				const float cooldownDelay = fire.cooldownTime.Get(chargingAltFire);
				if (cooldownDelay > 0.0f)
				{
					m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cooldownDelay;
					m_flNextPrimaryAttack = GetNextAttackDelay(cooldownDelay);
				}
				return;
			}
		}

		if (chargedAttack && m_fInAttack != 2)
		{
			return;
		}
	}

	const bool allowUnderwater = fire.allowUnderwater.Get(altMode);

	if (m_pPlayer->pev->waterlevel == WL_Eyes && !allowUnderwater)
	{
		PlayEmptySound(altMode);
		m_flNextPrimaryAttack = GetNextAttackDelay(fire.delayUnderwater.Get(altMode));
		return;
	}

	bool triggerTool = false;

	if (params.toolIndex >= 0)
	{
		const int toolBit = 1<<params.toolIndex;
		if (FBitSet(m_pPlayer->m_ToolStateBits, toolBit) && !FBitSet(m_pPlayer->m_ToolUnalignedBits, toolBit))
		{
			triggerTool = true;
		}
		else
		{
			PlayWeaponSoundScript(params.toolDenySound);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + params.toolDelayAfterDeny;
			return;
		}
	}

	const int ammoPerFire = fire.ammoPerFire.Get(altMode);
	const bool useSecondaryAmmo = fire.useSecondaryAmmo.Get(altMode);

	bool lastShot = false;

	if (ammoPerFire > 0)
	{
		if (useSecondaryAmmo)
		{
			if (m_iSecondaryAmmoType > 0)
			{
				if (m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] < ammoPerFire)
				{
					PlayEmptySound(altMode);
					PerformCooldown(altMode);
					return;
				}

				if (HandleAttackSubstitution(altMode))
					return;

				m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] -= ammoPerFire;
				m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] = Q_max(0, m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()]);

				if (m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] == 0)
					lastShot = true;
			}
		}
		else
		{
			if (UsesAmmo() || UsesClip())
			{
				if (!HasAmmoToFire(ammoPerFire))
				{
					if (params.reloadAutostart)
					{
						Reload();
						if (!HasAmmoToFire(ammoPerFire))
							PlayEmptySound(altMode);
					}
					else
					{
						if (m_fFireOnEmpty)
						{
							PlayEmptySound(altMode);
							m_flNextPrimaryAttack = GetNextAttackDelay(fire.delayAfterEmpty.Get(altMode));
						}
					}
					PerformCooldown(altMode);
					return;
				}

				if (HandleAttackSubstitution(altMode))
					return;

				SpendAmmo(ammoPerFire);
				UpdateRechargeTime(altMode);
				lastShot = Emptied();
				UpdateTape();
			}
		}
	}
	else
	{
		if (HandleAttackSubstitution(altMode))
			return;
	}

	if (triggerTool)
	{
#if !CLIENT_DLL
		if (params.toolTriggerDelay > 0.0f)
		{
			m_toolTriggerTime = gpGlobals->time + params.toolTriggerDelay;
		}
		else
		{
			CBaseEntity* triggerEnt = CBaseEntity::OwnInstance(m_pPlayer->m_UseToolTriggers[params.toolIndex]);
			if (triggerEnt)
				triggerEnt->Use(m_pPlayer, m_pPlayer, USE_TOGGLE, 0.0f);
		}
#endif
	}

	m_shouldPlayCooldown = true;

	const float flCycleTime = (lastShot && fire.cycleTimeLastShot.Get(altMode) > 0.0f) ? fire.cycleTimeLastShot.Get(altMode) : fire.cycleTime.Get(altMode);

	if (fire.preventMovement.Get(altMode))
	{
		m_pPlayer->m_movementPrevented = true;
		m_pPlayer->m_movementPreventedTime = gpGlobals->time + flCycleTime;
	}

	bool mustResetZoom = false;

	if (params.altMode.zoomFOV > 0 && m_pPlayer->m_iFOV != 0 && params.altMode.resetZoomOnFire)
	{
		if (params.altMode.resumeZoomAfterReset)
		{
			m_pPlayer->m_bResumeZoom = true;
			m_pPlayer->m_iLastZoom = m_pPlayer->m_iFOV;
			SetZoom(0);
		}
		else
			mustResetZoom = true;
	}

	m_bAlternatingEject = !m_bAlternatingEject;

	if (fire.muzzleFlash.Get(altMode))
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	SendScreenShake(fire.shake.Get(altMode));

	if (fireType != WeaponParameters::Fire::MELEE)
	{
		// Melee attacks play animation in the swing functions
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}

	m_pPlayer->m_iWeaponVolume = fire.weaponVolume.Get(altMode);
	m_pPlayer->m_iWeaponFlash = fire.weaponFlash.Get(altMode);

	const int extraSoundTypes = fire.extraSoundTypes.Get(altMode);
	if (extraSoundTypes)
	{
		m_pPlayer->m_iExtraSoundTypes = extraSoundTypes;
		m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + fire.extraSoundTime.Get(altMode);
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	const float autoAimDegree = fire.autoAimDegree.Get(altMode);

	if (autoAimDegree)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(autoAimDegree);
	}
	else
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		vecAiming = gpGlobals->v_forward;
	}

#if !CLIENT_DLL
	const float suspendLaserSpotTime = fire.suspendLaserSpotTime.Get(altMode);
	if (suspendLaserSpotTime)
	{
		if (m_pLaser)
			m_pLaser->Suspend(suspendLaserSpotTime);
	}
#endif

	float spreadX = 0.0f;
	float spreadY = 0.0f;
	Vector vecSpread{};
	if (fireType == WeaponParameters::Fire::BULLETS)
	{
		vecSpread = GetSpread(altMode);

		DamageInfoPatch damageInfo = fire.damageInfo.Get(altMode);
		if (chargedAttack)
		{
			auto damageRange = damageInfo.damage.has_value() ? GetSkillValueRange(*damageInfo.damage) : FloatRange{};
			auto damageFactorRange = fire.damageChargedFactor.Get(altMode);
			if (damageFactorRange == 0.0f)
				damageFactorRange = damageRange;
			auto maxDamageRange = fire.damageChargedMax.Get(altMode);
			if (maxDamageRange == 0.0f)
				maxDamageRange = damageRange * 2.0f;

			auto additionalDamageRange = damageFactorRange * (gpGlobals->time - m_chargeStartTime);
			damageRange = RangeSum(damageRange, additionalDamageRange);

#if !CLIENT_DLL
			const float maxDamage = RandomizeSkillValue(maxDamageRange);
			damageRange.min = Q_min(damageRange.min, maxDamage);
			damageRange.max = Q_min(damageRange.max, maxDamage);
#endif
			damageInfo.damage = damageRange;
		}

		const int bulletCount = fire.bulletCount.Get(altMode);
		const Vector randomizedSpread = m_pPlayer->FireBulletsPlayer(bulletCount, vecSrc, vecAiming, vecSpread, fire.bulletDistance.Get(altMode), damageInfo, fire.rangeModifier.Get(altMode), fire.tracerFreq.Get(altMode), m_pPlayer->pev, m_pPlayer->random_seed);
		if (bulletCount > 1)
		{
			// TODO: properly send spreads for multiple bullet shots to the client?
			spreadX = vecSpread.x;
			spreadY = vecSpread.y;
		}
		else
		{
			spreadX = randomizedSpread.x;
			spreadY = randomizedSpread.y;

			//ALERT(at_console, "Result spread: %g, %g\n", spreadX, spreadY);
		}
	}
	else if (fireType == WeaponParameters::Fire::NATIVE)
	{
		NativeAttack(altMode);
	}
	else if (fireType == WeaponParameters::Fire::PROJECTILE)
	{
		ProjectileAttack(altMode);
	}
	else if (fireType == WeaponParameters::Fire::MELEE)
	{
		if (chargedAttack)
		{
			if (m_iSwingMode != 1)
			{
				SelectAndSendFireAnimation(fire.chargeAnims.Get(altMode));
				m_chargeStartTime = gpGlobals->time;
			}
			m_swingIsAltAttack = altMode;
			m_iSwingMode = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2f;
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		}
		else
		{
			if (!m_iSwingMode)
			{
				m_swingIsAltAttack = altMode;
				if (!Swing(true))
				{
#if !CLIENT_DLL
					SetThink( &CConfigurableWeapon::SwingAgain );
					pev->nextthink = gpGlobals->time + 0.1f;
#endif
				}
			}
		}
		return;
	}

	m_lastShotWasInAltMode = altMode;
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flInaccuracy = fire.spread.GetNewInaccuracy(altMode, m_flInaccuracy, m_iShotsFired, m_flLastFire, gpGlobals->time);
	m_flLastFire = gpGlobals->time;

	//ALERT(at_console, "Punch to send: %g, %g\n", m_pPlayer->pev->punchangle.x, m_pPlayer->pev->punchangle.y);

	PLAYBACK_EVENT_FULL(PlaybackFlags(), m_pPlayer->edict(), GetPlaybackEvent(altMode), 0.0, g_vecZero, g_vecZero,
						spreadX, spreadY,
						PackIParam1(altMode, lastShot), PackIParam2(), 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay( flCycleTime );
	if (params.secondaryFireType == SecondaryFireType::ALTERNATIVE_FIRE)
	{
		m_flNextSecondaryAttack = Q_max(m_flNextPrimaryAttack, m_flNextSecondaryAttack);
	}

	if (altMode)
	{
		m_secondaryFireEndTime = gpGlobals->time + flCycleTime;
	}
	else
	{
		m_primaryFireEndTime = gpGlobals->time + flCycleTime;
	}

	if (ammoPerFire > 0)
	{
		if (useSecondaryAmmo)
			CheckOutOfSecondaryAmmo();
		else
		{
			if (!CanRechargeAmmo())
				CheckOutOfAmmo();
		}
	}

	const float pumpTime = fire.pumpDelay.Get(altMode);
	if (pumpTime)
	{
		m_flPumpTime = gpGlobals->time + pumpTime;
		m_pumpAltMode = altMode;
	}

	const FloatRange weaponIdleDelayRange = fire.idleDelay.Get(altMode, lastShot);
	const float weaponIdleDelay = RandomizeNumberFromRange_Shared(m_pPlayer->random_seed, weaponIdleDelayRange);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + weaponIdleDelay;

	m_fInSpecialReload = 0;

	ApplyMyKickBack(altMode);

	const float shellEjectDelay = fire.shellEjectDelay.Get(altMode);
	if (shellEjectDelay > 0.0f)
	{
		m_pPlayer->m_flEjectBrass = gpGlobals->time + shellEjectDelay;
		m_wasInAltModeBeforeEjectLate = altMode;
	}

	const float pushbackForce = fire.pushbackForce.Get(altMode);
	if (pushbackForce)
	{
		const float currentZVel = m_pPlayer->pev->velocity.z;

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		const Vector vecPush = gpGlobals->v_forward * pushbackForce;

		m_pPlayer->pev->velocity -= vecPush;

		const bool pushbackVertical = fire.pushbackVertical.Get(altMode);
		if (!pushbackVertical)
			m_pPlayer->pev->velocity.z = currentZVel;
	}

	int burstShots = fire.burstShots.Get(altMode);
	if (burstShots > 1)
	{
		m_burstFireIsAlt = altMode;
		m_burstShotsFired++;
		m_burstTime = gpGlobals->time + fire.burstInterval.Get(altMode);
		m_burstSpreadX = vecSpread.x;
		m_burstSpreadY = vecSpread.y;
	}

	if (mustResetZoom)
		ResetZoom(SwitchModeReason::Forced);
}

void CConfigurableWeapon::ProjectileAttack(bool altMode)
{
#if !CLIENT_DLL
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;

	int projectileVariant = 0;
	const auto& projectileName = fire.projectileName.Get(altMode);
	if (!projectileName.empty())
	{
		const char* projectileStr = GetRealProjectileClassname(projectileName.c_str(), projectileVariant);

		Vector vecHead = m_pPlayer->GetGunPosition();
		Vector aimAngles = m_pPlayer->pev->v_angle;
		if (fire.projectileRespectPunchangle.Get(altMode))
			aimAngles += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(aimAngles);
		aimAngles.x = -aimAngles.x;
		const Vector vecUp = gpGlobals->v_up;

		Vector vecDir = gpGlobals->v_forward;
		Vector vecSrc = vecHead +
						gpGlobals->v_forward * fire.projectileOffsetForward.Get(altMode) +
						gpGlobals->v_right * fire.projectileOffsetSide.Get(altMode) +
						gpGlobals->v_up * fire.projectileOffsetUp.Get(altMode);

		Vector vecShift{};

		const auto& firePhases = fire.projectileFirePhases.Get(altMode);
		if (!firePhases.empty())
		{
			m_iFirePhase = Q_min((int)firePhases.size()-1, m_iFirePhase); // just in case

			vecShift += gpGlobals->v_up * firePhases[m_iFirePhase].up;
			vecShift += gpGlobals->v_right * firePhases[m_iFirePhase].side;

			m_iFirePhase++;
			if (m_iFirePhase == firePhases.size())
				m_iFirePhase = 0;
		}

		vecSrc += vecShift;
		vecHead += vecShift;

		Vector vecAngles = aimAngles;
		if (fire.projectileAdjustToCross.Get(altMode))
		{
			TraceResult tr;
			UTIL_TraceLine(vecHead, vecHead + vecDir * 4096, dont_ignore_monsters, edict(), &tr);
			vecDir = (tr.vecEndPos - vecSrc).Normalize();
		}

		const Vector vecSpread = GetSpread(altMode);
		if (vecSpread != g_vecZero)
		{
			float x, y;
			do {
				x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5, 0.5f );
				y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5, 0.5f );
			}
			while( x * x + y * y > 1.0f );

			vecDir += x * vecSpread.x * gpGlobals->v_right;
			vecDir += y * vecSpread.y * gpGlobals->v_up;
		}

		if (fire.projectileAdjustToCross.Get(altMode))
		{
			vecAngles = UTIL_VecToAngles(vecDir);
			//vecAngles.x = -vecAngles.x;
		}

		EntityOverrides entityOverrides;
		if (!fire.projectileEntTemplate.Get(altMode).empty())
		{
			entityOverrides.entTemplate = MAKE_STRING(fire.projectileEntTemplate.Get(altMode).c_str());
		}
		ProjectileParameters projectileParams(projectileStr, vecSrc, vecAngles, vecDir, m_pPlayer, entityOverrides);
		const bool allowInheritance = !altMode || (fire.projectileName.Get(false) == fire.projectileName.Get(true) && fire.projectileEntTemplate.Get(false) == fire.projectileEntTemplate.Get(true));
		const float customSpeed = allowInheritance ? fire.projectileSpeed.Get(altMode) : (altMode ? fire.projectileSpeed.alt : fire.projectileSpeed.main);
		if (customSpeed > 0)
			projectileParams.speedOverride = customSpeed;
		projectileParams.variant = projectileVariant;
		projectileParams.pLauncher = this;
		projectileParams.time = fire.projectileDetonationTime.Get(altMode);
		DamageInfoPatch damageInfo;
		if (allowInheritance)
		{
			damageInfo = fire.damageInfo.Get(altMode);
		}
		else
		{
			if (altMode)
			{
				if (fire.damageInfo.alt.has_value())
				{
					damageInfo = *fire.damageInfo.alt;
				}
			}
			else
			{
				damageInfo = fire.damageInfo.main;
			}
		}
		const float customDamage = damageInfo.damage.has_value() ? GetSkillValue(*damageInfo.damage) : 0.0f;
		if (customDamage > 0)
			projectileParams.damageOverride = customDamage;
		projectileParams.up = vecUp;
		CBaseEntity* pProjectile = CreateAndLaunchAsProjectile(projectileParams);

		if (pProjectile)
		{
			const auto addVelocity = fire.projectileAddCurrentVelocity.Get(altMode);
			switch(addVelocity)
			{
			case WeaponParameters::Fire::ADD_VELOCITY_ABSOLUTE:
				pProjectile->pev->velocity += m_pPlayer->pev->velocity;
				break;
			case WeaponParameters::Fire::ADD_VELOCITY_PROJECTION:
				pProjectile->pev->velocity += vecDir * DotProduct(m_pPlayer->pev->velocity, vecDir);
				break;
			default:
				break;
			}
		}
	}
#endif
}

void CConfigurableWeapon::FireRemaining()
{
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;

	const bool altMode = m_burstFireIsAlt;

	const int ammoPerFire = fire.ammoPerFire.Get(altMode);
	const bool useSecondaryAmmo = fire.useSecondaryAmmo.Get(altMode);

	bool canFireMore = true;
	if (ammoPerFire > 0)
	{
		if (useSecondaryAmmo)
		{
			if (m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] < ammoPerFire)
			{
				canFireMore = false;
			}
			else
			{
				m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] -= ammoPerFire;
				m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] = Q_max(0, m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()]);
			}
		}
		else
		{
			if (UsesAmmo() || UsesClip())
			{
				if (!HasAmmoToFire(ammoPerFire))
				{
					canFireMore = false;
				}
				else
				{
					SpendAmmo(ammoPerFire);
					UpdateTape();
				}
			}
		}
	}

	if (!canFireMore)
	{
		ResetBurst();
		UpdateRechargeTime(altMode);
		return;
	}

	m_bAlternatingEject = !m_bAlternatingEject;

	if (fire.muzzleFlash.Get(altMode))
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecSpread{m_burstSpreadX, m_burstSpreadY, 0.0f};
	float spreadX = 0.0f;
	float spreadY = 0.0f;

	const auto fireType = fire.fireType.Get(altMode);

	if (fireType == WeaponParameters::Fire::BULLETS)
	{
		const int bulletCount = fire.bulletCount.Get(altMode);
		const Vector randomizedSpread = m_pPlayer->FireBulletsPlayer(bulletCount, vecSrc, gpGlobals->v_forward, vecSpread, fire.bulletDistance.Get(altMode), fire.damageInfo.Get(altMode), fire.rangeModifier.Get(altMode), fire.tracerFreq.Get(altMode), m_pPlayer->pev, m_pPlayer->random_seed);
		if (bulletCount > 1)
		{
			spreadX = vecSpread.x;
			spreadY = vecSpread.y;
		}
		else
		{
			spreadX = randomizedSpread.x;
			spreadY = randomizedSpread.y;
		}
	}
	else if (fireType == WeaponParameters::Fire::PROJECTILE)
	{
		ProjectileAttack(altMode);
	}

	const int iParam1Bits = PackIParam1(altMode, Emptied());

	PLAYBACK_EVENT_FULL(PlaybackFlags(), m_pPlayer->edict(), GetPlaybackEvent(altMode), 0.0, g_vecZero, g_vecZero,
						spreadX, spreadY,
						iParam1Bits, PackIParam2(), 0, 0);

	SendScreenShake(fire.shake.Get(altMode));

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (++m_burstShotsFired < fire.burstShots.Get(m_burstFireIsAlt))
	{
		m_burstTime = gpGlobals->time + fire.burstInterval.Get(altMode);
	}
	else
	{
		ResetBurst();
		UpdateRechargeTime(altMode);
	}
}

void CConfigurableWeapon::ResetBurst()
{
	m_burstShotsFired = 0;
	m_burstTime = 0.0f;
}

void CConfigurableWeapon::ResetInaccuracy()
{
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;

	const bool altMode = InAltMode();
	m_iShotsFired = 0;
	m_flInaccuracy = fire.spread.GetDefaultInaccuracy(altMode);
}

void CConfigurableWeapon::PrimaryAttack()
{
	m_shouldRestartReloading = false;
	PerformWeaponFire(InAltMode());
}

void CConfigurableWeapon::SwitchMode(SwitchModeReason reason)
{
	const WeaponParameters& params = MyParameters();

	if (params.altMode.zoomFOV && !m_pPlayer->m_bResumeZoom)
	{
		if (m_pPlayer->m_iFOV != 0)
		{
			if (params.altMode.zoomFOV2 > 0 && m_pPlayer->m_iFOV != params.altMode.zoomFOV2 && reason == SwitchModeReason::Regular)
			{
				SetZoom(params.altMode.zoomFOV2);

				if (params.altMode.zoomSound2.waves.size())
					PlayWeaponSoundScript(params.altMode.zoomSound2);
				else
					PlayWeaponSoundScript(params.altMode.zoomSound);

				const float attackDelay = params.altMode.attackDelay.Get(false);
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + attackDelay;

				return;
			}
			else
			{
				SetZoom(0);
				PlayWeaponSoundScript(params.altMode.unzoomSound);
			}
		}
		else if (m_pPlayer->m_iFOV != params.altMode.zoomFOV && reason == SwitchModeReason::Regular)
		{
			PlayWeaponSoundScript(params.altMode.zoomSound);
			SetZoom(params.altMode.zoomFOV);
		}
	}
	if (params.altMode.toggleLaserSpot)
	{
		ToggleLaserSpot(reason == SwitchModeReason::Regular);
	}

	const int animIndex = params.altMode.animIndex.Get(m_inAltMode);
	if (animIndex >= 0)
	{
		SendWeaponAnim(animIndex);
	}

	const float attackDelay = params.altMode.attackDelay.Get(m_inAltMode);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + attackDelay;
	if (animIndex >= 0)
	{
		m_flTimeWeaponIdle = Q_max(m_flTimeWeaponIdle, m_flNextSecondaryAttack);
		m_flNextPrimaryAttack = GetNextAttackDelay(attackDelay);
	}

	const float bodySwitchDelay = reason == SwitchModeReason::Regular ? params.altMode.bodyDelay.Get(m_inAltMode) : 0.0f;
	const float modeSwitchDelay = reason == SwitchModeReason::Regular ? params.altMode.modeDelay.Get(m_inAltMode) : 0.0f;

	const bool canSwitchBody = params.viewModelBody.Get(false) != params.viewModelBody.Get(true);

	if (canSwitchBody)
	{
		if (!bodySwitchDelay)
		{
			SetBody(params.viewModelBody.Get(!m_inAltMode));
		}
		else
		{
			m_switchingBody = true;
			m_wasInAltModeBeforeSwitchingBody = m_inAltMode;
		}
	}

	if (!modeSwitchDelay)
		m_inAltMode = !m_inAltMode;
	else
		m_switchingMode = true;

	float delay = 0.0f;
	if (bodySwitchDelay && modeSwitchDelay)
		delay = Q_min(bodySwitchDelay, modeSwitchDelay);
	else if (bodySwitchDelay)
		delay = bodySwitchDelay;
	else if (modeSwitchDelay)
		delay = modeSwitchDelay;
	if (delay > 0.0f)
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + delay;

	if (!m_inAltMode)
		m_pPlayer->m_bResumeZoom = false;

	//ALERT(at_console, "New mode: %s\n", m_inAltMode ? "alternative" : "primary");
}

void CConfigurableWeapon::SecondaryAttack()
{
	m_shouldRestartReloading = false;

	const WeaponParameters& params = MyParameters();

	switch (params.secondaryFireType) {
	case SecondaryFireType::DISABLED:
		return;
	case SecondaryFireType::ALTERNATIVE_FIRE:
		PerformWeaponFire(true);
		break;
	case SecondaryFireType::SWITCH_MODE:
		SwitchMode();
		break;
	}
}

bool CConfigurableWeapon::PerformReload()
{
	m_shouldRestartReloading = false;

	if (m_burstTime != 0)
		return false;

	const WeaponParameters& params = MyParameters();
	const bool altMode = InAltMode();

	if (CanRechargeAmmo())
	{
		if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] >= m_pPlayer->GetMaxAmmo(PrimaryAmmoIndex()))
			return false;

		while (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] < m_pPlayer->GetMaxAmmo(PrimaryAmmoIndex()) && m_flRechargeTime < gpGlobals->time)
		{
			PlayWeaponSoundScript(params.recharge.sound.Get(altMode));
			m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]++;
			m_flRechargeTime += params.recharge.interval.Get(altMode);
		}
		return true;
	}

	if (!CanReload())
		return false;

	const WeaponParameters::Reload& reload = params.reload;
	const bool empty = Emptied();

	// don't reload until recoil is done
	if (reload.waitForRecoil.Get(altMode, empty) && m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return false;

	if (PerformCooldown(m_chargingAltFire))
		return false;

	SetChargingAttack(false);
	m_bDelayFire = false;
	ResetInaccuracy();

#if !CLIENT_DLL
	if (m_pLaser)
	{
		float reloadDuration = reload.duration.Get(altMode, empty);
		const float attackDelay = reload.attackDelay.Get(altMode, empty);
		reloadDuration = Q_max(reloadDuration, attackDelay);
		const float suspendLaserTime = reload.suspendLaserSpotTime.Get(altMode, empty);
		const float suspendDuration = Q_max(reloadDuration, suspendLaserTime);
		if (suspendDuration > 0.0f)
		{
			m_pLaser->Suspend(suspendDuration);
		}
	}
#endif

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		const int startAnimIndex = params.startReload.animIndex.Get(altMode, empty);
		if (startAnimIndex >= 0)
		{
			ResetZoom(SwitchModeReason::Reload);

			const float duration = params.startReload.duration.Get(altMode, empty);
			SendWeaponAnim(startAnimIndex);
			m_fInSpecialReload = 1;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + duration;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
			m_flNextPrimaryAttack = GetNextAttackDelay(duration);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + duration;
			return true;
		}
	}
	if (params.manualReload)
	{
		if (m_fInSpecialReload == 0)
		{
			m_fInSpecialReload = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		}

		if (m_fInSpecialReload == 1)
		{
			if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
				return false;
			// was waiting for gun to move to side
			m_fInSpecialReload = 2;

			const int animIndex = reload.animIndex.Get(altMode, empty);

			ResetZoom(SwitchModeReason::Reload);

			PlayWeaponSoundScript(reload.sound.Get(altMode, empty));

			if (animIndex >= 0)
				SendWeaponAnim(animIndex);

			const float attackDelay = reload.duration.Get(altMode, empty);
			if (attackDelay)
				m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + attackDelay;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RandomizeNumberFromRange(reload.idleDelay.Get(altMode, empty));
			return true;
		}
		else
		{
			int ammoCount = reload.ammoCount.Get(altMode, empty);
			if (ammoCount <= 0)
				ammoCount = 1;

			ReloadClipNow(ammoCount);

			m_fInSpecialReload = 1;
			return true;
		}
	}

	const int animIndex = reload.animIndex.Get(altMode, empty);

	const float reloadDuration = reload.duration.Get(altMode, empty);
	bool result = DefaultClipReload(animIndex, reloadDuration, ViewModelBody());
	if (result)
	{
		if (m_fInSpecialReload == 0 && params.endReload.animIndex.Get(altMode, empty) >= 0)
		{
			m_fInSpecialReload = 1;
			m_wasEmptyReload = empty;
		}

		PlayWeaponSoundScript(reload.sound.Get(altMode, empty));

		m_pPlayer->m_bResumeZoom = false;
		ResetZoom(SwitchModeReason::Reload);

		const float attackDelay = reload.attackDelay.Get(altMode, empty);
		if (attackDelay > 0)
		{
			m_flNextPrimaryAttack = GetNextAttackDelay(Q_max(attackDelay, reloadDuration));
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + Q_max(attackDelay, reloadDuration);
		}

		float idleDelay = RandomizeNumberFromRange(reload.idleDelay.Get(altMode, empty));
		idleDelay = Q_max(idleDelay, attackDelay);
		idleDelay = Q_max(idleDelay, reloadDuration);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + idleDelay;

		m_iShotsFired = 0;
		m_bDelayFire = true;
	}
	return result;
}

void CConfigurableWeapon::Reload()
{
	PerformReload();
}

void CConfigurableWeapon::SendIdleAnimation()
{
	const WeaponParameters& params = MyParameters();

	const WeaponParameters::IdleAnimArray& idleAnims = params.idleAnims.Get(InAltMode(), Emptied());
	if (idleAnims.empty())
		return;

	auto sendIdleAnim = [this](const WeaponParameters::IdleAnim anim) {
		SendWeaponAnim(anim.animIndex);
		PlayWeaponSoundScript(anim.sound);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RandomizeNumberFromRange_Shared(m_pPlayer->random_seed, anim.duration);
	};

	if (idleAnims.size() == 1)
	{
		sendIdleAnim(idleAnims.front());
		return;
	}

	float chanceSum = 0.0f;
	for (const WeaponParameters::IdleAnim& anim : idleAnims)
	{
		chanceSum += anim.chance;
	}

	const float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0f, chanceSum);
	float curSum = 0.0f;
	for (const WeaponParameters::IdleAnim& anim : idleAnims)
	{
		curSum += anim.chance;
		if (flRand <= curSum)
		{
			sendIdleAnim(anim);
			return;
		}
	}
}

void CConfigurableWeapon::WeaponIdle()
{
	ResetEmptySound();
	UpdateAutoAim();

	const WeaponParameters& params = MyParameters();
	const bool altMode = InAltMode();

	if (CanRechargeAmmo())
	{
		Reload();
	}

	if (m_iSwingMode == 1)
	{
		if (gpGlobals->time > m_chargeStartTime + params.fire.chargeTime.Get(m_swingIsAltAttack))
		{
			m_iSwingMode = 2;
		}
		return;
	}
	else if (m_iSwingMode == 2)
	{
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + params.fire.cycleTime.Get(m_swingIsAltAttack);
		BigSwing();
		m_iSwingMode = 0;
		return;
	}

	if (m_flTimeWeaponIdle >= UTIL_WeaponTimeBase() && params.manualReload && m_fInSpecialReload)
		return;

	if (m_shouldRestartReloading && UsesClip() && m_iClip < iMaxClip() && (!UsesAmmo() || m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0))
	{
		Reload();
	}
	else if (params.reloadAutostart && UsesClip() && m_iClip == 0 && m_fInSpecialReload == 0 && (!UsesAmmo() || m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0))
	{
		Reload();
	}
	else
	{
		if (m_fInSpecialReload != 0)
		{
			const int ammoCountMin = params.reload.ammoCountMin.Get(altMode, false);
			if (params.manualReload && m_iClip < m_iMaxClip && (!UsesAmmo() || m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] >= ammoCountMin))
			{
				Reload();
				return;
			}

			const bool empty = m_wasEmptyReload;
			m_wasEmptyReload = false;
			const int endReloadAnimIndex = params.endReload.animIndex.Get(altMode, empty);
			if (endReloadAnimIndex >= 0)
			{
				// reload debounce has timed out

				m_iVisibleClip = m_iClip;
				UpdateTape(m_iVisibleClip);

				SendWeaponAnim(endReloadAnimIndex);

				// play cocking sound
				PlayWeaponSoundScript(params.endReload.sound.Get(altMode, empty));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + params.endReload.idleDelay.Get(altMode, empty);
				const float attackDelay = params.endReload.attackDelay.Get(altMode, empty);
				if (attackDelay)
				{
					m_flNextPrimaryAttack = GetNextAttackDelay(attackDelay);
					m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + attackDelay;
				}
				return;
			}
		}
	}

	if (m_chargingAttack)
	{
		if (CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()) || CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
		{
			const bool chargedAttackAlt = m_chargingAltFire;
			const bool chargedAttack = params.fire.chargedAttack.Get(chargedAttackAlt);
			if (chargedAttack)
			{
				const int buttonToCheck = params.secondaryFireType == SecondaryFireType::SWITCH_MODE ? IN_ATTACK : (chargedAttackAlt ? IN_ATTACK2 : IN_ATTACK);
				const bool chargeAttackReleased = FBitSet(m_pPlayer->m_afButtonReleased, buttonToCheck);
				if (chargeAttackReleased)
				{
					m_fInAttack = 2;
					PerformWeaponFire(chargedAttackAlt);
					SetChargingAttack(false);
					m_fInAttack = 0;
				}
			}
			else
				SetChargingAttack(false);
		}
	}

	if (m_chargingAttack)
		return;

	if (PerformCooldown(m_chargingAltFire))
		return;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendIdleAnimation();
}

void CConfigurableWeapon::Holster()
{
	if (m_pPlayer->m_flEjectBrass != 0.0f)
	{
		m_pPlayer->m_flEjectBrass = 0.0f;
		EjectBrassLate();
	}

	ResetBurst();
	m_fInReload = false;
	m_iSwingMode = 0;

	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;

	const WeaponParameters& params = MyParameters();

#if !CLIENT_DLL
	if (m_pPlayer && FBitSet(m_pPlayer->m_suppressedCapabilities, PLAYER_SUPPRESS_JUMP_DUE_TO_WEAPON))
	{
		ClearBits(m_pPlayer->m_suppressedCapabilities, PLAYER_SUPPRESS_JUMP_DUE_TO_WEAPON);
	}
#endif

	if (!params.manualReload || !params.manualReloadContinueOnDeploy)
		m_fInSpecialReload = 0;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + params.holster.attackDelay;
	const float idleDelay = RandomizeNumberFromRange_Shared(m_pPlayer->random_seed, params.holster.idleDelay);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + idleDelay;
	SendWeaponAnim(params.holster.animIndex.Get(m_inAltMode, Emptied()));

	ResetZoom(SwitchModeReason::Holster);
	m_pPlayer->m_bResumeZoom = false;

#if !CLIENT_DLL
	if (m_pLaser)
	{
		m_pLaser->Killed(nullptr, nullptr, GIB_NEVER);
		m_pLaser = nullptr;
	}
#endif

	if (CanRechargeAmmo() && !HasAmmoToFire())
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

int CConfigurableWeapon::ViewModelBody()
{
	const WeaponParameters& params = MyParameters();

	if (params.ammoToBody.empty())
		return pev->body;
	else
		return BodyFromClip();
}

void CConfigurableWeapon::UpdateAutoAim()
{
	const WeaponParameters& params = MyParameters();
	const float autoAimDegree = params.fire.autoAimDegree.Get(InAltMode());
	if (autoAimDegree)
		m_pPlayer->GetAutoaimVector(autoAimDegree);
}

void CConfigurableWeapon::UpdateSpot()
{
#if !CLIENT_DLL
	if (m_bLaserActive)
	{
		if (m_pPlayer->m_hTankControls != 0)
			return;

		const WeaponParameters& params = MyParameters();
		if (!m_pLaser)
		{
			m_pLaser = CLaserSpot::CreateSpot(m_pPlayer->edict());
			if (!params.laserSpotAttractRockets)
				m_pLaser->pev->classname = MAKE_STRING("eagle_laser");
			m_pLaser->pev->scale = params.laserSpotScale;

			PlayWeaponSoundScript(params.activateLaserSpotSound);
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

		UTIL_SetOrigin( m_pLaser->pev, tr.vecEndPos );
	}
#endif
}

void CConfigurableWeapon::ToggleLaserSpot(bool playDeactivationSound)
{
	const bool wasActive = m_bLaserActive;
	m_bLaserActive = !m_bLaserActive;
	if (wasActive)
	{
#if !CLIENT_DLL
		if (m_pLaser)
		{
			if (playDeactivationSound)
			{
				const WeaponParameters& params = MyParameters();
				PlayWeaponSoundScript(params.deactivateLaserSpotSound);
			}
			m_pLaser->Killed(nullptr, nullptr, GIB_NORMAL);
			m_pLaser = nullptr;
		}
#endif
	}
}

void CConfigurableWeapon::SetChargingAttack(bool charging)
{
	m_chargingAttack = charging;

	const WeaponParameters& params = MyParameters();
	if (params.fire.laserSpotOnCharge.Get(m_chargingAltFire))
	{
		if (charging && !m_bLaserActive)
			ToggleLaserSpot(false);
		else if (!charging && m_bLaserActive)
			ToggleLaserSpot(false);
	}
}

void CConfigurableWeapon::SetZoom(int fov)
{
	const WeaponParameters& params = MyParameters();

#if !CLIENT_DLL
	if (fov != m_pPlayer->m_iFOV)
	{
		const WeaponParameters::Fade& fade = params.altMode.zoomFade;
		if (fade.fadeTime > 0 || fade.holdTime > 0)
		{
			UTIL_ScreenFade(m_pPlayer, VectorFromColor(fade.color), fade.fadeTime, fade.holdTime, fade.alpha, fade.flags);
		}
	}
#endif

	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = fov;
}

void CConfigurableWeapon::ResetZoom(SwitchModeReason reason)
{
	const WeaponParameters& params = MyParameters();
	if (params.altMode.zoomFOV && m_inAltMode && params.secondaryFireType == SecondaryFireType::SWITCH_MODE)
	{
		SwitchMode(reason);
	}
}

#if CLIENT_DLL
void CConfigurableWeapon::KickBack(const WeaponKickBack&) {}
#endif

enum
{
	WEAPONDATA_ALTMODE = (1<<0),
	WEAPONDATA_LASERSPOT = (1<<1),
	WEAPONDATA_BURST_IS_ALT = (1<<2),
	WEAPONDATA_BURSTING = (1<<3),
	WEAPONDATA_SWITCHING_BODY = (1<<4),
	WEAPONDATA_WAS_IN_ALT_MODE_BEFORE_SWITCHING_BODY = (1<<5),
	WEAPONDATA_SWITCHING_MODE = (1<<6),
	WEAPONDATA_SWING_MODE = (1<<7),
	WEAPONDATA_SWING_MODE2 = (1<<8),
};

enum
{
	WEAPONCHARGE_CHARGING = (1<<0),
	WEAPONCHARGE_ALTMODE = (1<<1),
	WEAPONCHARGE_SHOULD_COOLDOWN = (1<<2),
};

void CConfigurableWeapon::GetWeaponData(weapon_data_t& data)
{
	data.iuser1 = 0;
	if (m_inAltMode)
		data.iuser1 |= WEAPONDATA_ALTMODE;
	if (m_bLaserActive)
		data.iuser1 |= WEAPONDATA_LASERSPOT;
	if (m_burstFireIsAlt)
		data.iuser1 |= WEAPONDATA_BURST_IS_ALT;

	if (m_switchingBody)
		data.iuser1 |= WEAPONDATA_SWITCHING_BODY;
	if (m_wasInAltModeBeforeSwitchingBody)
		data.iuser1 |= WEAPONDATA_WAS_IN_ALT_MODE_BEFORE_SWITCHING_BODY;
	if (m_switchingMode)
		data.iuser1 |= WEAPONDATA_SWITCHING_MODE;
	if (m_iSwingMode == 2)
		data.iuser1 |= WEAPONDATA_SWING_MODE2;
	else if (m_iSwingMode == 1)
		data.iuser1 |= WEAPONDATA_SWING_MODE;

	data.iuser2 = pev->body & 0xF;

	int chargeFlags = 0;
	if (m_chargingAttack)
		chargeFlags |= WEAPONCHARGE_CHARGING;
	if (m_chargingAltFire)
		chargeFlags |= WEAPONCHARGE_ALTMODE;
	if (m_shouldPlayCooldown)
		chargeFlags |= WEAPONCHARGE_SHOULD_COOLDOWN;

	data.iuser2 |= chargeFlags << 4;

	data.iuser3 = m_iShotsFired & 0xFF;

	if (m_burstTime != 0.0f)
	{
		data.iuser1 |= WEAPONDATA_BURSTING;
		data.iuser3 |= (m_burstShotsFired & 0xF) << 8;
		data.fuser2 = gpGlobals->time - m_burstTime;
	}

	data.fuser3 = m_flInaccuracy;
}

void CConfigurableWeapon::SetWeaponData(const weapon_data_t& data)
{
	m_inAltMode = FBitSet(data.iuser1, WEAPONDATA_ALTMODE);
	m_bLaserActive = FBitSet(data.iuser1, WEAPONDATA_LASERSPOT);
	m_burstFireIsAlt = FBitSet(data.iuser1, WEAPONDATA_BURST_IS_ALT);
	m_switchingBody = FBitSet(data.iuser1, WEAPONDATA_SWITCHING_BODY);
	m_wasInAltModeBeforeSwitchingBody = FBitSet(data.iuser1, WEAPONDATA_WAS_IN_ALT_MODE_BEFORE_SWITCHING_BODY);
	m_switchingMode = FBitSet(data.iuser1, WEAPONDATA_SWITCHING_MODE);

	if (FBitSet(data.iuser1, WEAPONDATA_SWING_MODE2))
		m_iSwingMode = 2;
	else if (FBitSet(data.iuser1, WEAPONDATA_SWING_MODE))
		m_iSwingMode = 1;
	else
		m_iSwingMode = 0;

	pev->body = data.iuser2 & 0xF;

	int chargeFlags = data.iuser2 >> 4;
	m_chargingAttack = FBitSet(chargeFlags, WEAPONCHARGE_CHARGING);
	m_chargingAltFire = FBitSet(chargeFlags, WEAPONCHARGE_ALTMODE);
	m_shouldPlayCooldown = FBitSet(chargeFlags, WEAPONCHARGE_SHOULD_COOLDOWN);

	m_iShotsFired = data.iuser3 & 0xFF;

	if (FBitSet(data.iuser1, WEAPONDATA_BURSTING))
	{
		if (m_burstTime == 0.0f)
		{
			// in case of save-restore, set these on client
			m_burstShotsFired = (data.iuser3 >> 8) & 0xF;
			m_burstTime = gpGlobals->time - data.fuser2;
		}
	}
	else
	{
		ResetBurst();
	}

	m_flInaccuracy = data.fuser3;
}

void CConfigurableWeapon::ResetWeaponData()
{
	// Note: this still has problems on changelevel. One of the shots is getting lost.
	ResetBurst();
}

void CConfigurableWeapon::Smack()
{
#if !CLIENT_DLL
	const WeaponParameters& params = MyParameters();
	if (params.fire.hitDecal.Get(m_swingIsAltAttack))
		DecalSmack(m_trHit);
#endif
}

void CConfigurableWeapon::HitShake()
{
#if !CLIENT_DLL
	const WeaponParameters& params = MyParameters();
	SendScreenShake(params.fire.hitShake.Get(m_swingIsAltAttack));
#endif
}

void CConfigurableWeapon::SwingAgain()
{
	Swing(false);
}

bool CConfigurableWeapon::Swing(bool fFirst)
{
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;
	const bool altMode = m_swingIsAltAttack;
	const float cycleTime = fire.cycleTime.Get(altMode);
	const float definedHitCycleTime = fire.hitCycleTime.Get(altMode);
	const float hitCycleTime = definedHitCycleTime <= 0.0f ? Q_max(cycleTime * 0.5f, cycleTime - 0.25f) : definedHitCycleTime;
	const FloatRange idleDelay = fire.idleDelay.Get(altMode, false);

	bool fDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
		if (!fire.kickBackOnHitOnly.Get(altMode))
			ApplyMyKickBack(altMode);
	}
	else
	{
		ApplyMyKickBack(altMode);
	}
#endif
	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), GetPlaybackEvent(altMode),
							0.0f, g_vecZero, g_vecZero, 0, 0, PackIParam1(altMode, Emptied()), PackIParam2(), 0, 0 );
	}

	if( tr.flFraction >= 1.0f )
	{
		if( fFirst )
		{
			// miss
			m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + cycleTime;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RandomizeNumberFromRange_Shared( m_pPlayer->random_seed, idleDelay );
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		const WeaponParameters::FireAnimArray& arr = fire.hitAnims.Get(altMode);
		if (arr.size())
		{
			const int count = static_cast<int>(arr.size());
			if (count)
			{
				SendWeaponAnim(arr[(m_iSwing++) % count]);
			}
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL
		// hit
		fDidHit = true;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// play thwack, smack, or dong sound
		float flVol = 1.0f;
		bool fHitWorld = true;

		HitShake();

		if( pEntity )
		{
			// If building with the clientside weapon prediction system,
			// UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
			// m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
			DamageInfo damageInfo{0.0f, DMG_CLUB};
			ApplyDamageInfoPatch(damageInfo, fire.damageInfo.Get(altMode));
#if CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1.0f == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1.0f < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
			}
			else
			{
				// subsequent swings do half
				damageInfo.damage *= fire.subsequentSwingFactor.Get(altMode);
			}
			pEntity->ApplyTraceAttack( m_pPlayer->pev, m_pPlayer->pev, damageInfo, gpGlobals->v_forward, &tr );

			if( pEntity->HasFlesh() )
			{
				// play thwack or smack sound
				PlayWeaponSoundScript(params.fire.hitBodySound.Get(altMode));
				m_pPlayer->m_iWeaponVolume = fire.bodyHitVolume.Get(altMode);

				if( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = GetNextAttackDelay(hitCycleTime);
					m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + hitCycleTime;
					return true;
				}
				else
					flVol = 0.1f;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound(tr, vecSrc, vecSrc + (vecEnd - vecSrc)*2.0f, true);

			if( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1.0f;
			}

			// also play crowbar strike
			PlayWeaponSoundScript(params.fire.hitWallSound.Get(altMode), fvolbar);

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * fire.wallHitVolume.Get(altMode) );

		SetThink( &CConfigurableWeapon::Smack );
		pev->nextthink = gpGlobals->time + fire.smackDelay.Get(altMode);
#endif
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + hitCycleTime;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RandomizeNumberFromRange_Shared( m_pPlayer->random_seed, idleDelay );
	return fDidHit;
}

void CConfigurableWeapon::BigSwing()
{
	const WeaponParameters& params = MyParameters();
	const WeaponParameters::Fire& fire = params.fire;
	const bool altMode = m_swingIsAltAttack;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc	= m_pPlayer->GetGunPosition();
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if ( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
		if (!fire.kickBackOnHitOnly.Get(altMode))
			ApplyMyKickBack(altMode);
	}
	else
	{
		ApplyMyKickBack(altMode);
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), GetPlaybackEvent(altMode),
						0.0f, g_vecZero, g_vecZero, 0, 0, PackIParam1(altMode, Emptied()), PackIParam2(), 0, 0 );

	if ( tr.flFraction >= 1.0 )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{
		const WeaponParameters::FireAnimArray& arr = fire.hitAnims.Get(altMode);
		if (arr.size())
		{
			const int count = static_cast<int>(arr.size());
			if (count)
			{
				SendWeaponAnim(arr[(m_iSwing++) % count]);
			}
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL
		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity)
		{
			DamageInfo damageInfo{0.0f, DMG_CLUB};
			ApplyDamageInfoPatch(damageInfo, fire.damageInfo.Get(altMode));

			auto damageFactor = fire.damageChargedFactor.Get(altMode);
			if (damageFactor == 0.0f)
				damageFactor = damageInfo.damage;
			auto maxDamageRange = fire.damageChargedMax.Get(altMode);
			if (maxDamageRange == 0.0f)
				maxDamageRange = damageInfo.damage * 2.0f;

			damageInfo.damage = damageInfo.damage + (gpGlobals->time - m_chargeStartTime) * RandomizeSkillValue(damageFactor);
			const float maxDamage = RandomizeSkillValue(maxDamageRange);
			if (damageInfo.damage > maxDamage) {
				damageInfo.damage = maxDamage;
			}

			pEntity->ApplyTraceAttack(m_pPlayer->pev, m_pPlayer->pev, damageInfo, gpGlobals->v_forward, &tr);
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool fHitWorld = true;

		if (pEntity)
		{
			if (pEntity->HasFlesh())
			{
				// play thwack or smack sound
				PlayWeaponSoundScript(params.fire.hitBodySound.Get(altMode));
				m_pPlayer->m_iWeaponVolume = fire.bodyHitVolume.Get(altMode);

				if ( !pEntity->IsAlive() )
					return;
				else
					flVol = 0.1f;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound(tr, vecSrc, vecSrc + (vecEnd - vecSrc)*2.0f, true);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1.0f;
			}

			PlayWeaponSoundScript(params.fire.hitWallSound.Get(altMode), fvolbar);

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * fire.wallHitVolume.Get(altMode) );

		SetThink( &CConfigurableWeapon::Smack );
		pev->nextthink = gpGlobals->time + fire.smackDelay.Get(altMode);
#endif
	}
}

bool CConfigurableWeapon::CanRechargeAmmo()
{
	const WeaponParameters& params = MyParameters();
	return params.recharge.interval.Get(InAltMode()) && UsesAmmo() && !UsesClip();
}

void CConfigurableWeapon::UpdateRechargeTime(bool altMode)
{
	if (CanRechargeAmmo())
	{
		const WeaponParameters& params = MyParameters();
		const float rechargeInterval = params.recharge.interval.Get(altMode);
		const float rechargeDelayAfterFire = params.recharge.delayAfterFire.Get(altMode);
		const float rechargeDelay = Q_max(rechargeInterval, rechargeDelayAfterFire);

		m_flRechargeTime = gpGlobals->time + rechargeDelay;
	}
}

float CConfigurableWeapon::GetMaxSpeed()
{
	float result = 0.0f;
#if !CLIENT_DLL
	auto CalcSpeed = [this](const PlayerSpeed& playerSpeed) {
		if (playerSpeed.isFactor)
		{
			return m_pPlayer->GetBaseMaxSpeed() * playerSpeed.value;
		}
		else
		{
			return playerSpeed.value;
		}
	};

	const WeaponParameters& params = MyParameters();

	if (m_chargingAttack)
	{
		const PlayerSpeed& speedOnCharge = params.fire.playerMaxSpeedOnCharge.Get(m_chargingAltFire);
		result = CalcSpeed(speedOnCharge);
	}

	const bool primaryFiring = m_primaryFireEndTime > gpGlobals->time;
	const bool secondaryFiring = m_secondaryFireEndTime > gpGlobals->time;

	if (result == 0.0f && (primaryFiring || secondaryFiring))
	{
		float weaponPrimaryFireSpeed = 0.0f;
		float weaponSecondaryFireSpeed = 0.0f;

		if (primaryFiring)
		{
			const PlayerSpeed& primaryFirePlayerSpeed = params.fire.playerMaxSpeed.Get(false);
			if (primaryFirePlayerSpeed.IsDefined())
				result = weaponPrimaryFireSpeed = CalcSpeed(primaryFirePlayerSpeed);
		}
		if (secondaryFiring)
		{
			const PlayerSpeed& secondaryFirePlayerSpeed = params.fire.playerMaxSpeed.Get(true);
			if (secondaryFirePlayerSpeed.IsDefined())
				result = weaponSecondaryFireSpeed = CalcSpeed(secondaryFirePlayerSpeed);
		}
		if (weaponPrimaryFireSpeed > 0.0f && weaponSecondaryFireSpeed > 0.0f)
		{
			result = Q_min(weaponPrimaryFireSpeed, weaponSecondaryFireSpeed);
		}
	}

	if (result == 0.0f)
	{
		const PlayerSpeed playerSpeed = params.playerMaxSpeed.Get(InAltMode());
		if (playerSpeed.IsDefined())
			result = CalcSpeed(playerSpeed);
	}
#endif
	return result;
}

void CConfigurableWeapon::OnPlayerAttackCapabilityChanged(bool enabled)
{
	if (!enabled)
	{
		if (m_pPlayer->m_iFOV != 0)
			ResetZoom(SwitchModeReason::Forced);
	}
}

void CConfigurableWeapon::ResetOnRemoveAsActive()
{
	if (m_pPlayer->m_iFOV != 0)
		ResetZoom(SwitchModeReason::Forced);
}

void CConfigurableWeapon::UpdateTape()
{
	int visibleClip = UsesClip() ? m_iClip : (UsesAmmo() ? m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] : 0);
	UpdateTape(visibleClip);
	m_iVisibleClip = visibleClip;
}

void CConfigurableWeapon::UpdateTape(int clip)
{
	const WeaponParameters& params = MyParameters();

	if (!params.ammoToBody.empty() && (UsesClip() || UsesAmmo()))
		pev->body = BodyFromClip(clip);
}

int CConfigurableWeapon::BodyFromClip()
{
	return BodyFromClip(m_iVisibleClip);
}

int CConfigurableWeapon::BodyFromClip(int clip)
{
	const WeaponParameters& params = MyParameters();

	for (const auto& p : params.ammoToBody)
	{
		if (clip == p.first)
		{
			return p.second;
		}
	}
	return params.viewModelBody.Get(InAltMode());
}

static int PackPunchAngleComponent(float f)
{
	int i = static_cast<int>(std::round(f * 8));
	i = clamp(i, -255, 255);
	return i;
}

int CConfigurableWeapon::PackIParam1(bool altMode, bool emptied)
{
	int packed = 0;
	if (altMode)
	{
		packed |= (int)WeaponEventFlags::ALTMODE;
	}
	if (emptied)
	{
		packed |= (int)WeaponEventFlags::EMPTIED;
	}
	if (m_bAlternatingEject)
	{
		packed |= (int)WeaponEventFlags::ALTERNATING_EJECT;
	}

	int body = ViewModelBody();
	body = clamp(body, 0, 15);

	packed |= (body << 3);

	if (m_pPlayer)
	{
		int punchAngleCoded = PackPunchAngleComponent(m_pPlayer->pev->punchangle.x);

		if (punchAngleCoded >= 0)
		{
			packed |= (punchAngleCoded << 7);
		}
		else
		{
			packed |= ((-punchAngleCoded) << 7);
			packed = -packed;
		}
	}

	return packed;
}

int CConfigurableWeapon::PackIParam2()
{
	int packed = WeaponId();

	int punchAngleCoded = PackPunchAngleComponent(m_pPlayer->pev->punchangle.y);

	if (punchAngleCoded >= 0)
	{
		packed |= (punchAngleCoded << 7);
	}
	else
	{
		packed |= ((-punchAngleCoded) << 7);
		packed = -packed;
	}

	return packed;
}

void CConfigurableWeapon::PrecacheCommonEvent()
{
	m_usFire = PRECACHE_EVENT(1, "events/glock1.sc");
}

class CGenericConfigurableWeapon : public CConfigurableWeapon {};

enum melee_e
{
	MELEE_IDLE = 0,
	MELEE_DRAW,
	MELEE_HOLSTER,
	MELEE_ATTACK1HIT,
	MELEE_ATTACK1MISS,
	MELEE_ATTACK2MISS,
	MELEE_ATTACK2HIT,
	MELEE_ATTACK3MISS,
	MELEE_ATTACK3HIT,
};

class CMelee : public CGenericConfigurableWeapon
{
public:
	int WeaponId() const override {
		return WEAPON_MELEE;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 0;
		p->iPosition = 5;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override
	{
		WeaponParameters params;

		params.maxClip = WEAPON_NOCLIP;

		params.worldModel = "models/w_crowbar.mdl";
		params.viewModel = "models/v_crowbar.mdl";
		params.playerModel = "models/p_crowbar.mdl";
		params.playerAnimExt = "crowbar";
		params.priority = 0;

		params.deploy.animIndex = MELEE_DRAW;

		params.fire.fireType = WeaponParameters::Fire::MELEE;
		params.fire.damageInfo.main.damage = 10;
		params.fire.anims = {MELEE_ATTACK1MISS, MELEE_ATTACK2MISS, MELEE_ATTACK3MISS};
		params.fire.hitAnims = {MELEE_ATTACK2HIT, MELEE_ATTACK3HIT};
		params.fire.sound = {
			CHAN_WEAPON,
			{"weapons/cbar_miss1.wav"},
			1.0f,
			ATTN_NORM,
			PITCH_NORM
		};
		params.fire.cycleTime = 0.5f;
		params.fire.hitBodySound = {
			CHAN_ITEM,
			{"weapons/cbar_hitbod1.wav", "weapons/cbar_hitbod2.wav", "weapons/cbar_hitbod3.wav"},
			1.0f,
			ATTN_NORM,
			PITCH_NORM
		};
		params.fire.hitWallSound = {
			CHAN_ITEM,
			{"weapons/cbar_hit1.wav", "weapons/cbar_hit2.wav"},
			1.0f,
			ATTN_NORM,
			IntRange(98, 101)
		};

		return params;
	}
};

enum pistol_e
{
	PISTOL_IDLE1 = 0,
	PISTOL_IDLE2,
	PISTOL_IDLE3,
	PISTOL_SHOOT,
	PISTOL_SHOOT_EMPTY,
	PISTOL_RELOAD,
	PISTOL_RELOAD_NOT_EMPTY,
	PISTOL_DRAW
};

class CPistol : public CGenericConfigurableWeapon
{
public:
	int WeaponId() const override {
		return WEAPON_PISTOL;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 1;
		p->iPosition = 4;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override
	{
		WeaponParameters params;

		params.initialAmmoAmount = 15;
		params.maxClip = 15;
		params.ammoName = "9mm";

		params.worldModel = "models/w_9mmhandgun.mdl";
		params.viewModel = "models/v_9mmhandgun.mdl";
		params.playerModel = "models/p_9mmhandgun.mdl";
		params.playerAnimExt = "onehanded";
		params.priority = 10;

		params.deploy.animIndex = PISTOL_DRAW;

		params.idleAnims.main = WeaponParameters::IdleAnimArray{
			WeaponParameters::IdleAnim{PISTOL_IDLE3, 0.3f, 49.0f / 16.0f},
			WeaponParameters::IdleAnim{PISTOL_IDLE1, 0.3f, 60.0f / 16.0f},
			WeaponParameters::IdleAnim{PISTOL_IDLE2, 0.4f, 40.0f / 16.0f}
		};

		params.fire.fireType = WeaponParameters::Fire::BULLETS;
		params.fire.damageInfo.main.damage = 8;
		params.fire.anims.main = {PISTOL_SHOOT};

		params.fire.sound = {
			CHAN_WEAPON,
			{"weapons/pl_gun3.wav"},
			FloatRange(0.92f, 1.0f),
			ATTN_NORM,
			IntRange(98, 101)
		};

		params.fire.spread.SetStaticSpread(false, 0.01f);
		params.fire.cycleTime = 0.3f;
		params.fire.allowUnderwater = false;

		params.fire.muzzleFlash = true;
		params.fire.weaponVolume = NORMAL_GUN_VOLUME;
		params.fire.weaponFlash = NORMAL_GUN_FLASH;

		params.fire.clientPunchPitch = -2.0f;
		params.fire.shellOffsetForward = 20;
		params.fire.shellOffsetUp = -12;
		params.fire.shellOffsetSide = 4;
		params.fire.shellModel = "models/shell.mdl";
		params.fire.shellSound = TE_BOUNCE_SHELL;

		params.reload.animIndex = PISTOL_RELOAD_NOT_EMPTY;
		params.reload.duration = 1.5f;
		params.reload.idleDelay = FloatRange(10.0f, 15.0f);
		params.reload.animIndex.mainEmptied = PISTOL_RELOAD;

		return params;
	}
};

class CPistol2 : public CPistol
{
public:
	int WeaponId() const override {
		return WEAPON_PISTOL2;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 1;
		p->iPosition = 5;
		return true;
	}
};

enum smg_e
{
	SMG_LONGIDLE = 0,
	SMG_IDLE1,
	SMG_LAUNCH,
	SMG_RELOAD,
	SMG_DEPLOY,
	SMG_FIRE1,
	SMG_FIRE2,
	SMG_FIRE3
};

class CSMG : public CGenericConfigurableWeapon
{
public:
	int WeaponId() const override {
		return WEAPON_SMG;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 1;
		p->iPosition = 6;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override
	{
		WeaponParameters params;

		params.initialAmmoAmount = 30;
		params.maxClip = 30;
		params.ammoName = "9mm";

		params.worldModel = "models/w_9mmAR.mdl";
		params.viewModel = "models/v_9mmAR.mdl";
		params.playerModel = "models/p_9mmAR.mdl";
		params.playerAnimExt = "mp5";
		params.priority = 15;

		params.deploy.animIndex = SMG_DEPLOY;

		params.idleAnims.main = WeaponParameters::IdleAnimArray{
			WeaponParameters::IdleAnim{SMG_LONGIDLE, 0.5f, 41.0f / 8.0f},
			WeaponParameters::IdleAnim{SMG_IDLE1, 0.5f, 111.0f / 35.0f},
		};

		params.fire.fireType = WeaponParameters::Fire::BULLETS;
		params.fire.damageInfo.main.damage = 8;
		params.fire.anims.main = {SMG_FIRE1, SMG_FIRE2, SMG_FIRE3};

		params.fire.sound = {
			CHAN_WEAPON,
			{"weapons/hks1.wav", "weapons/hks2.wav"},
			FloatRange(0.92f, 1.0f),
			ATTN_NORM,
			IntRange(94, 109)
		};
		params.fire.spread.SetStaticSpread(false, VECTOR_CONE_3DEGREES);
		params.fire.cycleTime = 0.1f;
		params.fire.allowUnderwater = false;

		params.fire.muzzleFlash = true;
		params.fire.weaponVolume = NORMAL_GUN_VOLUME;
		params.fire.weaponFlash = NORMAL_GUN_FLASH;

		params.fire.clientPunchPitch = FloatRange(-2.0f, 2.0f);
		params.fire.shellOffsetForward = 20;
		params.fire.shellOffsetUp = -12;
		params.fire.shellOffsetSide = 4;
		params.fire.shellModel = "models/shell.mdl";
		params.fire.shellSound = TE_BOUNCE_SHELL;

		params.reload.animIndex = SMG_RELOAD;
		params.reload.duration = 1.5f;

		return params;
	}
};

class CSMG2 : public CSMG
{
public:
	int WeaponId() const override {
		return WEAPON_SMG2;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 1;
		p->iPosition = 7;
		return true;
	}
};

class CRifle : public CSMG
{
public:
	int WeaponId() const override {
		return WEAPON_RIFLE;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 2;
		p->iPosition = 6;
		return true;
	}
};

class CRifle2 : public CSMG
{
public:
	int WeaponId() const override {
		return WEAPON_RIFLE2;
	}
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 2;
		p->iPosition = 7;
		return true;
	}
};

enum shotgun2_e
{
	SHOTGUN2_IDLE = 0,
	SHOTGUN2_FIRE,
	SHOTGUN2_FIRE2,
	SHOTGUN2_RELOAD,
	SHOTGUN2_PUMP,
	SHOTGUN2_START_RELOAD,
	SHOTGUN2_DRAW,
	SHOTGUN2_HOLSTER,
	SHOTGUN2_IDLE4,
	SHOTGUN2_IDLE_DEEP
};

class CShotgun2 : public CGenericConfigurableWeapon
{
public:
	int WeaponId() const override {
		return WEAPON_SHOTGUN2;
	}
	bool GetItemInfo(ItemInfo *p) override {
		p->iSlot = 2;
		p->iPosition = 8;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override
	{
		WeaponParameters params;

		params.initialAmmoAmount = 8;
		params.maxClip = 8;
		params.ammoName = "buckshot";

		params.worldModel = "models/w_shotgun.mdl";
		params.viewModel = "models/v_shotgun.mdl";
		params.playerModel = "models/p_shotgun.mdl";
		params.playerAnimExt = "shotgun";
		params.priority = 15;

		params.deploy.animIndex = SHOTGUN2_DRAW;

		params.idleAnims.main = WeaponParameters::IdleAnimArray{
			WeaponParameters::IdleAnim{SHOTGUN2_IDLE_DEEP, 0.8f, 60.0f/12.0f},
			WeaponParameters::IdleAnim{SHOTGUN2_IDLE, 0.15f, 20.0f/9.0f},
			WeaponParameters::IdleAnim{SHOTGUN2_IDLE4, 0.05f, 20.0f/9.0f}
		};

		// Primary fire
		params.fire.fireType = WeaponParameters::Fire::BULLETS;
		params.fire.damageInfo.main.damage = 5;
		params.fire.anims.main = {SHOTGUN2_FIRE};

		params.fire.sound = {
			CHAN_WEAPON,
			{"weapons/sbarrel1.wav"},
			FloatRange(0.95f, 1.0f),
			ATTN_NORM,
			IntRange(93, 124)
		};

		params.fire.spread.SetStaticSpread(false, VECTOR_CONE_10DEGREES);
		params.fire.cycleTime = 0.75f;
		params.fire.idleDelay.main = 5.0f;
		params.fire.allowUnderwater = false;
		params.fire.bulletCount = 6;

		params.fire.muzzleFlash = true;
		params.fire.weaponVolume = LOUD_GUN_VOLUME;
		params.fire.weaponFlash = NORMAL_GUN_FLASH;

		params.fire.bulletDistance = 2048;

		params.fire.clientPunchPitch = -5.0f;
		params.fire.shellOffsetForward = 32;
		params.fire.shellOffsetUp = -12;
		params.fire.shellOffsetSide = 6;
		params.fire.shellModel = "models/shotgunshell.mdl";
		params.fire.shellSound = TE_BOUNCE_SHOTSHELL;
		//

		params.startReload.animIndex = SHOTGUN2_START_RELOAD;
		params.startReload.duration = 0.7f;

		params.reloadAutostart = true;
		params.manualReload = true;

		params.reload.animIndex = SHOTGUN2_RELOAD;
		params.reload.idleDelay = 0.5f;
		params.reload.duration = 0.0f;
		params.reload.sound = {
			CHAN_ITEM,
			{"weapons/reload1.wav", "weapons/reload3.wav"},
			1.0f,
			ATTN_NORM,
			IntRange(85, 114)
		};
		params.reload.waitForRecoil = true;

		params.endReload.animIndex = SHOTGUN2_PUMP;
		params.endReload.idleDelay = 1.5f;
		params.endReload.attackDelay = 0.0f;

		return params;
	}
};

enum sniper2_e
{
	SNIPER2_DRAW = 0,
	SNIPER2_SLOWIDLE1,
	SNIPER2_FIRE,
	SNIPER2_FIRELASTROUND,
	SNIPER2_RELOAD1,
	SNIPER2_RELOAD2,
	SNIPER2_RELOAD3,
	SNIPER2_SLOWIDLE2,
	SNIPER2_HOLSTER
};

class CSniperRifle2 : public CGenericConfigurableWeapon
{
public:
	int WeaponId() const override {
		return WEAPON_SNIPERRIFLE2;
	}
	bool GetItemInfo(ItemInfo *p) override {
		p->iSlot = 2;
		p->iPosition = 9;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override
	{
		WeaponParameters params;

		params.initialAmmoAmount = 5;
		params.maxClip = 5;
		params.ammoName = "762";

		params.worldModel = "models/w_m40a1.mdl";
		params.viewModel = "models/v_m40a1.mdl";
		params.playerModel = "models/p_m40a1.mdl";
		params.playerAnimExt = "bow";
		params.priority = 10;

		params.deploy.animIndex = SNIPER2_DRAW;

		params.idleAnims.main = WeaponParameters::IdleAnimArray{
			WeaponParameters::IdleAnim{SNIPER2_SLOWIDLE1, 1.0f, 67.5f / 16.0f}
		};

		// Primary fire
		params.fire.fireType = WeaponParameters::Fire::BULLETS;
		params.fire.damageInfo.main.damage = 40;
		params.fire.anims.main = {SNIPER2_FIRE};

		params.fire.sound = {
			CHAN_WEAPON,
			{"weapons/sniper_fire.wav"},
			FloatRange(0.9f, 1.0f),
			ATTN_NORM,
			IntRange(98, 101)
		};

		params.fire.spread.SetStaticSpread(false, 0.001f);
		params.fire.cycleTime = 1.75f;
		params.fire.idleDelay = 68.0f / 38.0f;
		params.fire.allowUnderwater = false;

		params.fire.muzzleFlash = true;
		params.fire.weaponVolume = LOUD_GUN_VOLUME;
		params.fire.weaponFlash = BRIGHT_GUN_FLASH;

		params.fire.clientPunchPitch = -5.0f;
		//

		// Alt fire
		params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
		params.altMode.zoomFOV = 18;
		params.altMode.attackDelay = 0.5f;
		params.altMode.zoomSound.waves = {"weapons/sniper_zoom.wav"};
		//

		params.reload.animIndex = SNIPER2_RELOAD3;
		params.reload.duration = 80.0f / 34.0f;

		return params;
	}
};

LINK_WEAPON_TO_CLASS(weapon_melee, CMelee)
LINK_WEAPON_TO_CLASS(weapon_pistol, CPistol)
LINK_WEAPON_TO_CLASS(weapon_pistol2, CPistol2)
LINK_WEAPON_TO_CLASS(weapon_smg, CSMG)
LINK_WEAPON_TO_CLASS(weapon_smg2, CSMG2)
LINK_WEAPON_TO_CLASS(weapon_rifle, CRifle)
LINK_WEAPON_TO_CLASS(weapon_rifle2, CRifle2)
LINK_WEAPON_TO_CLASS(weapon_shotgun2, CShotgun2)
LINK_WEAPON_TO_CLASS(weapon_sniperrifle2, CSniperRifle2)
