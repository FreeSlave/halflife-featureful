#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

enum camera_e
{
	CAMERA_IDLE = 0,
	CAMERA_USE,
	CAMERA_DRAW,
	CAMERA_HOLSTER
};

class CCameraTool : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_CAMERA; }
	bool GetItemInfo(ItemInfo* p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_camera, CCameraTool )

bool CCameraTool::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 4;
	p->iPosition = 5;
	return true;
}

WeaponParameters CCameraTool::GetDefaultParameters() const
{
	WeaponParameters params;

	params.worldModel = "models/w_camera.mdl";
	params.viewModel = "models/v_camera.mdl";
	params.playerModel = "models/p_camera.mdl";

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{CAMERA_IDLE, 1.0f, 1.0f}
	};

	params.deploy.animIndex = CAMERA_DRAW;
	params.deploy.duration = 1.0f;

	params.fire.anims = {CAMERA_USE};
	params.fire.cycleTime = 1.82f;
	params.fire.preventMovement = true;

	params.holster.animIndex = CAMERA_HOLSTER;
	params.holster.attackDelay = 0.73f;

	params.modelSounds = {"weapons/camera-1.wav", "weapons/camera_sliderelease.wav"};

	params.mirrorViewModel = true;

	params.toolIcon = "icon_camera";
	params.toolTriggerDelay = 1.8f;

	return std::move(params);
}

enum radio_e
{
	RADIO_IDLE = 0,
	RADIO_DRAW,
	RADIO_HOLSTER,
	RADIO_USE
};

class CRadioTool : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_RADIO; }
	bool GetItemInfo(ItemInfo* p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_radio, CRadioTool )

bool CRadioTool::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 4;
	p->iPosition = 6;
	return true;
}

WeaponParameters CRadioTool::GetDefaultParameters() const
{
	WeaponParameters params;

	params.worldModel = "models/w_radio.mdl";
	params.viewModel = "models/v_radio.mdl";
	params.playerModel = "models/p_radio.mdl";

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{RADIO_IDLE, 1.0f, 1.0f}
	};

	params.deploy.animIndex = RADIO_DRAW;
	params.deploy.duration = 1.0f;

	params.fire.anims = {RADIO_USE};
	params.fire.cycleTime = 2.75f;
	params.fire.preventMovement = true;

	params.holster.animIndex = RADIO_HOLSTER;
	params.holster.attackDelay = 1.3f;

	params.modelSounds = {"weapons/radio_draw.wav", "weapons/radio_use.wav"};

	params.mirrorViewModel = true;

	params.toolIcon = "icon_radio";

	return std::move(params);
}

enum btorch_e
{
	BTORCH_IDLE = 0,
	BTORCH_USE,
	BTORCH_DRAW
};

#define BLOWTORCH_FLARE_SPRITE "sprites/flare1.spr"
#define BLOWTORCH_FIRE_SOUND "weapons/blowtorch-1.wav"

class CBlowTorchTool : public CConfigurableWeapon
{
public:
	void Precache() override;
	bool Deploy() override;
	void Holster() override;
	int WeaponId() const override { return WEAPON_BTORCH; }
	bool GetItemInfo(ItemInfo* p) override;
	WeaponParameters GetDefaultParameters() const override;
	void NativeAttack(bool altMode) override;
	void WeaponIdle() override;
	void StopFireSound();
	void SetTorchState(int state);

	bool m_playingFireSound;

	int m_usBlowTorch;
};

LINK_WEAPON_TO_CLASS( weapon_blowtorch, CBlowTorchTool )

void CBlowTorchTool::Precache()
{
	CConfigurableWeapon::Precache();
	PRECACHE_MODEL(BLOWTORCH_FLARE_SPRITE);
	PRECACHE_SOUND(BLOWTORCH_FIRE_SOUND);
	m_usBlowTorch = PRECACHE_EVENT(1, "events/crowbar.sc"); //re-use crowbar event as all fully configurable weapons use the glock1.sc anyway
}

bool CBlowTorchTool::Deploy()
{
	if (CConfigurableWeapon::Deploy())
	{
		SetTorchState(1);
		return true;
	}
	return false;
}

void CBlowTorchTool::Holster()
{
	CConfigurableWeapon::Holster();
	SetTorchState(0);
	StopFireSound();
}

bool CBlowTorchTool::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 4;
	p->iPosition = 7;
	return true;
}

WeaponParameters CBlowTorchTool::GetDefaultParameters() const
{
	WeaponParameters params;

	params.worldModel = "models/w_btorch.mdl";
	params.viewModel = "models/v_btorch.mdl";
	params.playerModel = "models/p_btorch.mdl";

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{BTORCH_IDLE, 1.0f, 1.0f}
	};

	params.deploy.animIndex = BTORCH_DRAW;
	params.deploy.duration = 1.0f;

	params.fire.anims = {BTORCH_USE};
	params.fire.damageInfo.main.damage = 1.0f;
	params.fire.damageInfo.main.type = DMG_BURN;
	params.fire.cycleTime = 0.1f;
	params.fire.preventMovement = true;
	params.fire.fireType = WeaponParameters::Fire::NATIVE;

	params.mirrorViewModel = true;

	params.toolIcon = "icon_blowtorch";

	return params;
}

void CBlowTorchTool::NativeAttack(bool altMode)
{
	const WeaponParameters& params = MyParameters();

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	const Vector vecDir = gpGlobals->v_forward;
	const Vector vecSrc = m_pPlayer->GetGunPosition();
	const Vector vecDest = vecSrc + vecDir * 64.0f;

	SetTorchState(2);

	if (!m_playingFireSound)
	{
		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, BLOWTORCH_FIRE_SOUND, 1.0f, ATTN_NORM, 0, 100);
		m_playingFireSound = true;
	}

	edict_t *pentIgnore = m_pPlayer->edict();
	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

	if (tr.fAllSolid)
		return;

#if !CLIENT_DLL
	CBaseEntity *pEntity = CBaseEntity::OwnInstance(tr.pHit);
	if (!pEntity)
		return;

	if (pev->dmgtime < gpGlobals->time)
	{
		if (pEntity->pev->takedamage)
		{
			DamageInfo damageInfo;
			ApplyDamageInfoPatch(damageInfo, params.fire.damageInfo.Get(altMode));
			pEntity->ApplyTraceAttack(m_pPlayer->pev, m_pPlayer->pev, damageInfo, vecDir, &tr);
		}

		pev->dmgtime = gpGlobals->time + 0.1f;
	}
#endif
}

void CBlowTorchTool::WeaponIdle()
{
	CConfigurableWeapon::WeaponIdle();
	SetTorchState(1);
	StopFireSound();
}

void CBlowTorchTool::StopFireSound()
{
	if (m_playingFireSound)
	{
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, BLOWTORCH_FIRE_SOUND);
		m_playingFireSound = false;
	}
}

void CBlowTorchTool::SetTorchState(int state)
{
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usBlowTorch, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, state, 0, 0, 0);
}

enum satchel_radio_e
{
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

class CWeaponTool : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_TOOL; }
	bool GetItemInfo(ItemInfo* p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_tool, CWeaponTool )

bool CWeaponTool::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 4;
	p->iPosition = 8;
	return true;
}

WeaponParameters CWeaponTool::GetDefaultParameters() const
{
	WeaponParameters params;

	params.worldModel = "models/w_weaponbox.mdl";
	params.viewModel = "models/v_satchel_radio.mdl";
	params.playerModel = "models/p_satchel_radio.mdl";
	params.playerAnimExt = "hive";

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SATCHEL_RADIO_IDLE1, 0.5f, 2.7f},
		WeaponParameters::IdleAnim{SATCHEL_RADIO_FIDGET1, 0.5f, 3.7f}
	};

	params.deploy.animIndex = SATCHEL_RADIO_DRAW;
	params.deploy.duration = 0.63f;

	params.fire.anims = {SATCHEL_RADIO_FIRE};
	params.fire.cycleTime = 1.0f;
	params.fire.preventMovement = true;

	params.holster.animIndex = SATCHEL_RADIO_HOLSTER;
	params.holster.attackDelay = 0.5f;

	params.toolIcon = "icon_radio";

	return std::move(params);
}
