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

class CCameraRadio : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_CAMERA; }
	bool GetItemInfo(ItemInfo* p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_camera, CCameraRadio )

bool CCameraRadio::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 4;
	p->iPosition = 5;
	return true;
}

WeaponParameters CCameraRadio::GetDefaultParameters() const
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

class CToolRadio : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_RADIO; }
	bool GetItemInfo(ItemInfo* p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_radio, CToolRadio )

bool CToolRadio::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 4;
	p->iPosition = 6;
	return true;
}

WeaponParameters CToolRadio::GetDefaultParameters() const
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
	p->iPosition = 7;
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
