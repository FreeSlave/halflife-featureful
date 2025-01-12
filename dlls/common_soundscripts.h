#pragma once
#ifndef COMMON_SOUNDSCRIPTS_H
#define COMMON_SOUNDSCRIPTS_H

#include "soundscripts.h"

extern const NamedSoundScript sparkBaseSoundScript;
extern const NamedSoundScript materialSparkSoundScript;

namespace NPC
{

extern const NamedSoundScript bodySplatSoundScript;

extern const NamedSoundScript bodyDropHeavySoundScript;
extern const NamedSoundScript bodyDropLightSoundScript;
extern const NamedSoundScript swishSoundScript;

extern const NamedSoundScript attackHitSoundScript;
extern const NamedSoundScript attackMissSoundScript;

extern const NamedSoundScript reloadSoundScript;
extern const NamedSoundScript single9mmSoundScript;
extern const NamedSoundScript burst9mmSoundScript;
extern const NamedSoundScript grenadeLaunchSoundScript;
extern const NamedSoundScript shotgunSoundScript;
extern const NamedSoundScript sniperSoundScript;
extern const NamedSoundScript handgunSoundScript;
extern const NamedSoundScript pythonSoundScript;
extern const NamedSoundScript desertEagleSoundScript;
extern const NamedSoundScript desertEagleReloadSoundScript;
extern const NamedSoundScript m249SoundScript;

extern const NamedSoundScript spitTouchSoundScript;
extern const NamedSoundScript spitHitSoundScript;

extern const NamedSoundScript crashSoundScript;

}

namespace Items
{
extern const NamedSoundScript pickupSoundScript;
extern const NamedSoundScript materializeSoundScript;
extern const NamedSoundScript ammoPickupSoundScript;

extern const char* const weaponPickupSoundScript;
extern const NamedSoundScript weaponDropSoundScript;
extern const NamedSoundScript weaponEmptySoundScript;
}

namespace Player
{
extern const NamedSoundScript sprayPaintSoundScript;
extern const NamedSoundScript fallBodySplatSoundScript;
extern const NamedSoundScript trainUseSoundScript;
extern const NamedSoundScript vehicleIgnitionSoundScript;
extern const NamedSoundScript underwaterExhaleSoundScript;
extern const NamedSoundScript undrownSoundScript;
extern const NamedSoundScript emergeInhaleSoundScript;
extern const NamedSoundScript flashlightOnSoundScript;
extern const NamedSoundScript flashlightOffSoundScript;
extern const NamedSoundScript nvgOnSoundScript;
extern const NamedSoundScript nvgOffSoundScript;
extern const NamedSoundScript deathSoundScript;
extern const NamedSoundScript deathUnderwaterSoundScript;
}

#endif
