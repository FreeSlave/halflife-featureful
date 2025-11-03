#include "const_sound.h"
#include "common_soundscripts.h"

const NamedSoundScript sparkBaseSoundScript = {
	CHAN_VOICE,
	{
		"buttons/spark1.wav", "buttons/spark2.wav", "buttons/spark3.wav",
		"buttons/spark4.wav", "buttons/spark5.wav", "buttons/spark6.wav"
	},
	"DoSparkBase"
};

const NamedSoundScript materialSparkSoundScript = {
	CHAN_STATIC,
	{ "buttons/spark5.wav", "buttons/spark6.wav" },
	FloatRange(0.7f, 1.0f),
	ATTN_NORM,
	"Material.Spark"
};

namespace NPC {

const NamedSoundScript bodySplatSoundScript = {
	CHAN_WEAPON,
	{"common/bodysplat.wav"},
	"NPC.Gibbed"
};

const NamedSoundScript bodyDropHeavySoundScript = {
	CHAN_BODY,
	{"common/bodydrop3.wav", "common/bodydrop4.wav"},
	90,
	"NPC.BodyDrop_Heavy"
};

const NamedSoundScript bodyDropLightSoundScript = {
	CHAN_BODY,
	{"common/bodydrop3.wav", "common/bodydrop4.wav"},
	"NPC.BodyDrop_Light"
};

const NamedSoundScript swishSoundScript = {
	CHAN_BODY,
	{"zombie/claw_miss2.wav"},
	"NPC.SwishSound"
};

const NamedSoundScript attackHitSoundScript = {
	CHAN_WEAPON,
	{"zombie/claw_strike1.wav", "zombie/claw_strike2.wav", "zombie/claw_strike3.wav"},
	IntRange(95, 105),
	"NPC.AttackHit"
};

const NamedSoundScript attackMissSoundScript = {
	CHAN_WEAPON,
	{"zombie/claw_miss1.wav", "zombie/claw_miss2.wav"},
	IntRange(95, 105),
	"NPC.AttackMiss"
};

const NamedSoundScript reloadSoundScript = {
	CHAN_WEAPON,
	{"hgrunt/gr_reload1.wav"},
	"NPC.Reload"
};

const NamedSoundScript single9mmSoundScript = {
	CHAN_WEAPON,
	{"weapons/hks1.wav", "weapons/hks2.wav", "weapons/hks3.wav"},
	"NPC.9MMSingle"
};

const NamedSoundScript burst9mmSoundScript = {
	CHAN_WEAPON,
	{"hgrunt/gr_mgun1.wav", "hgrunt/gr_mgun2.wav"},
	"NPC.9MM"
};

const NamedSoundScript grenadeLaunchSoundScript = {
	CHAN_WEAPON,
	{"weapons/glauncher.wav"},
	0.8f,
	ATTN_NORM,
	"NPC.GrenadeLaunch"
};

const NamedSoundScript shotgunSoundScript = {
	CHAN_WEAPON,
	{"weapons/sbarrel1.wav"},
	"NPC.Shotgun"
};

const NamedSoundScript sniperSoundScript = {
	CHAN_WEAPON,
	{"weapons/sniper_fire.wav"},
	1.0f,
	0.6f,
	"NPC.Sniper"
};

const NamedSoundScript handgunSoundScript = {
	CHAN_WEAPON,
	{"weapons/pl_gun3.wav"},
	"NPC.Handgun"
};

const NamedSoundScript pythonSoundScript = {
	CHAN_WEAPON,
	{"weapons/357_shot1.wav", "weapons/357_shot2.wav"},
	"NPC.Python"
};

const NamedSoundScript desertEagleSoundScript = {
	CHAN_WEAPON,
	{"weapons/desert_eagle_fire.wav"},
	"NPC.DesertEagle"
};

const NamedSoundScript desertEagleReloadSoundScript = {
	CHAN_WEAPON,
	{"weapons/desert_eagle_reload.wav"},
	"NPC.ReloadDesertEagle"
};

const NamedSoundScript m249SoundScript = {
	CHAN_WEAPON,
	{"weapons/saw_fire1.wav", "weapons/saw_fire2.wav", "weapons/saw_fire3.wav"},
	IntRange(95, 109),
	"NPC.M249"
};

const NamedSoundScript spitTouchSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_acid1.wav"},
	IntRange(90, 110),
	"NPC.SpitTouch"
};

const NamedSoundScript spitHitSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_spithit1.wav", "bullchicken/bc_spithit2.wav"},
	IntRange(90, 110),
	"NPC.SpitHit"
};

const NamedSoundScript crashSoundScript = {
	CHAN_STATIC,
	{"weapons/mortarhit.wav"},
	1.0f,
	0.3f,
	"NPC.Crash"
};

}

namespace Items
{

const NamedSoundScript pickupSoundScript = {
	CHAN_ITEM,
	{"items/gunpickup2.wav"},
	"Item.Pickup"
};

const NamedSoundScript materializeSoundScript = {
	CHAN_WEAPON,
	{"items/suitchargeok1.wav"},
	150,
	"Item.Materialize"
};

const NamedSoundScript ammoPickupSoundScript = {
	CHAN_ITEM,
	{"items/9mmclip1.wav"},
	"Ammo.Pickup"
};

const char* const weaponPickupSoundScript = "Weapon.Pickup";

const NamedSoundScript weaponDropSoundScript = {
	CHAN_VOICE,
	{"items/weapondrop1.wav"},
	IntRange(95, 119),
	"Weapon.Drop"
};

const NamedSoundScript weaponEmptySoundScript = {
	CHAN_WEAPON,
	{"weapons/357_cock1.wav"},
	0.8f,
	ATTN_NORM,
	"Weapon.Empty"
};

const NamedSoundScript inventoryPickupSoundScript = {
	CHAN_ITEM,
	{},
	"Inventory.Pickup"
};

}

namespace Player
{

const NamedSoundScript sprayPaintSoundScript = {
	CHAN_VOICE,
	{"player/sprayer.wav"},
	"SprayCan.Paint"
};

const NamedSoundScript fallBodySplatSoundScript = {
	CHAN_ITEM,
	{"common/bodysplat.wav"},
	"Player.FallBodySplat"
};

const NamedSoundScript fallPainSoundScript = {
	CHAN_VOICE,
	{"player/pl_fallpain3.wav"},
	"Player.FallPain"
};

const NamedSoundScript jumpSoundScript = {
	CHAN_BODY,
	{},
	0.5f,
	ATTN_NORM,
	"Player.Jump"
};

const NamedSoundScript trainUseSoundScript = {
	CHAN_ITEM,
	{"plats/train_use1.wav"},
	0.8f,
	ATTN_NORM,
	"Player.TrainUse"
};

const NamedSoundScript vehicleIgnitionSoundScript = {
	CHAN_ITEM,
	{"plats/vehicle_ignition.wav"},
	0.8f,
	ATTN_NORM,
	"Player.VehicleUse"
};

const NamedSoundScript wadeSoundScript = {
	CHAN_BODY,
	{"player/pl_wade1.wav", "player/pl_wade2.wav", "player/pl_wade3.wav", "player/pl_wade4.wav"},
	1.0f,
	ATTN_NORM,
	"Player.Wade"
};

const NamedSoundScript underwaterExhaleSoundScript = {
	CHAN_BODY,
	{"player/pl_swim1.wav", "player/pl_swim2.wav", "player/pl_swim3.wav", "player/pl_swim4.wav"},
	0.8f,
	ATTN_NORM,
	"Player.UnderwaterExhale"
};

const NamedSoundScript undrownSoundScript = {
	CHAN_VOICE,
	{"player/pl_wade1.wav"},
	"Player.Undrown"
};

const NamedSoundScript emergeInhaleSoundScript = {
	CHAN_VOICE,
	{"player/pl_wade2.wav"},
	"Player.EmergeInhale"
};

const NamedSoundScript flashlightOnSoundScript = {
	CHAN_WEAPON,
	{"items/flashlight1.wav"},
	"Player.FlashlightOn"
};

const NamedSoundScript flashlightOffSoundScript = {
	CHAN_WEAPON,
	{"items/flashlight1.wav"},
	"Player.FlashLightOff"
};

const NamedSoundScript nvgOnSoundScript = {
	CHAN_WEAPON,
	{},
	"Player.NVGOn"
};

const NamedSoundScript nvgOffSoundScript = {
	CHAN_WEAPON,
	{},
	"Player.NVGOff"
};

const NamedSoundScript deathSoundScript = {
	CHAN_VOICE,
	{},
	"Player.Death"
};

const NamedSoundScript deathUnderwaterSoundScript = {
	CHAN_VOICE,
	{},
	"Player.DeathUnderwater"
};

const NamedSoundScript geigerSoundScript = {
	CHAN_STATIC,
	{"player/geiger1.wav", "player/geiger2.wav", "player/geiger3.wav"},
	FloatRange(0.25f, 0.5f),
	ATTN_NORM,
	"Player.Geiger"
};

}
