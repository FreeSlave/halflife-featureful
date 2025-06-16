---
title: "Soundscripts"
---

# {{% param "title" %}}

Originally in Half-Life the sound names used by NPCs and items are explicitly written in the code as file paths, along with the properties these sounds should be played with like volume, attenuation and pitch. This leaves the mod author no other choice than changing the code or replacing the .wav files (the latter only changes the sound files, not the audio properties. It also doesn't allow to change the number of sounds to choose from when playing a random sound).

This has been changed in Source where [soundscripts](https://developer.valvesoftware.com/wiki/Soundscripts) were introduced to ease the customization of the played sounds. Featureful SDK implements the similar concept. Now NPCs and items refer to the *soundscripts* that can be defined in the outer file - **sound/soundscripts.json**. Each soundscript has a name and each monster or item uses a predefined set of soundscripts and refer to them by names.

Example:

```json
{
    "Garg.Footstep": {
        "channel": "body",
        "waves": ["garg/gar_step1.wav", "garg/gar_step2.wav"],
        "volume": 1.0,
        "attenuation": "norm",
        "pitch": [90, 110],
    },
    "Garg.Pain": {
        "channel": "voice",
        "waves": ["garg/gar_pain1.wav", "garg/gar_pain2.wav", "garg/gar_pain3.wav"],
        "volume": 1.0,
        "attenuation": "norm",
        "pitch": 100
    }
}
```

Unlike Source the default soundscript definitions are still present in the game code to keep the outer file definitions optional. Moreover when you provide a customized soundscript but omit some properties (like channel, pitch, etc.) the ones defined by default will be used. This way you can redefine only certain set of properties leaving the rest intact. The example above (with Garg sounds) is actually redundant as for demonstration purposes it defines sounds and properties the same way as they're written in the game code. Let's look at the example that actually makes a change:

```json
{
    "Barney.Pain": {
        "waves": ["fgrunt/gr_pain1.wav", "fgrunt/gr_pain2.wav"],
        "pitch": 110
    }
}
```

**Barney.Pain** will use the custom set of waves and the custom pitch, but will retain default values for other properties like channel, volume, attenuation.

## Format of soudscripts.json

Each entry in the file may have the following properties:

* `"channel"` - a channel to play the sound on. Possible values: `"auto"`, `"weapon"`, `"voice"`, `"item"`, `"body"`, `"static"`. Note: generally it's not recommended to redefine the sound channel as entities are designed to play certain sounds on certain channels for a reason.
* `"waves"` - an array of sound samples. A sample can be a path to .wav file (e.g. `"barney/yup.wav"`), the specific sentence (starts with '!', e.g. `"!BA_OK1"`) or a sentence group (e.g. `"BA_OK"`). When the soundscript is played the wave is choosen randomly from the array. The maximum number of samples in the list is 10.
* `"volume"` - a numeric value or [range]({{< ref "JSON/#range" >}}) between 0.0 and 1.0 to define a sound volume. If it's a range the volume value will be randomized on each play. Example of range: `[0.7, 0.9]`.
* `"attenuation"` - a numeric value to define a sound attenuation (i.e. how fast the volume will decrease with the distance between the sound origin and the listener). The normal value is 0.8. The value 0.0 means no atenuation (play everywhere). The following string aliases are supported as well instead of numeric values: `"norm"` (large radius), `"static"` (medium radius), `"idle"` (small radius), `"none"` (play everywhere).
* `"pitch"` - an integer value or [integer range]({{< ref "JSON/#range_int" >}}) to define a sound pitch, where 100 is a normal pitch. The values higher than 100 mean high pitch, and the values below 100 mean lowered pitch. If it's a range the pitch value will be randomized on each play. Example of range: `[95, 105]`

## Debugging

If you're unsure about applied soundscript properties, you can check them by the `dump_soundscripts` command in the console (the `developer` mode must be enabled and some map should be loaded).

* Type `dump_soundscripts` to get all currently registered soundscripts and their properties (so e.g. if there were no Garg on a map, his soundscripts won't be shown).
* Provide arguments to get only the soundscripts you're interested in, e.g. `dump_soundscripts Barney.Pain Barney.Die`.
* If the provided argument ends with `.` the command will show all soundscripts which names start with this string. E.g. `dump_soundscripts Barney. Otis.` prints all registered soundscripts which names start with `Barney.` and `Otis.`.

## Disabling a soundscript

If you want some soundscript to not play at all, redefine it with an empty `waves` property:

```json
{
    "Barney.Pain": {
        "waves": []
    }
}
```

## Derived soundscripts

Some soundscripts are based on other ones. That means that they inherit the properties of the base soundscript and optionally replace some of them (e.g. pitch).

For example Otis doesn't have his own pain and death sounds and the ones from Barney are used by default. **Otis.Pain** is derived from (or based on) **Barney.Pain** so by default they're equal (same thing with **Otis.Die** and **Barney.Die**). However you can explicitly define the custom soundscripts so Otis will have unique set of pain and death sounds different from Barney's. The omitted properties will still be derived from Barney's soundscripts.

Example:

```json
{
    "Barney.Pain": {
        "pitch": 110
    },
    "Otis.Pain": {
        "waves": ["otis/ot_pain1.wav", "otis/ot_pain2.wav"]
    }
}
```

This soundscript replaces the sound set for **Otis.Pain**. The other properties like channel, volume, attenuation and pitch will be taken from **Barney.Pain**.

Note: if **Barney.Pain** is redefined and **Otis.Pain** is not explicitly defined Otis will use the redefined version of **Barney.Pain** soundscript for pain sounds.

## Sequence events

NPC play some sounds on animation events. Events are properties attached to the specific frames of the sequence. Some events explicitly define what sound should be played, i.e. the sound name is stored in the model. As such sounds don't go through the soundscript system, you can't affect them by customizing the soundscripts. But you can change the model to use the soundscript event. See [Model animation events]({{< ref "model-animation-events" >}}).

## List of entity soundcripts

{{% hint info %}}
Notes:
* All names are case-insensitive. For example **HoundEye.Idle** and **houndeye.idle** refer to the same soundscript.
* `Empty by default` means that the wave list is not defined for the soundscript in the game code by default. The developer must provide a customized soundscript to enable it. For example [male assassins]({{< ref monster_male_assassin >}}) are silent by default, but if **Massn.Die** is provided the monster will play it when they die.
{{% /hint %}}

Soundscripts used by a certain entity are listed on the page for the entity. See [entities]({{< ref "entity-guide" >}}). Here's the list of soundscripts common to multiple entities:

### Effect soundscripts

* **DoSpark** - sounds played by [env_spark]({{< ref env_spark >}}) and sparking [func_button]({{< ref func_button >}}).
* **Material.Spark** - played when the player hits a texture of material that produces sparks (e.g. the computer material).

### Common Item soundscripts

* **Ammo.Pickup** - ammo item is getting picked up.
* **Item.Pickup** - item is getting picked up.
* **Item.Materialize** - item or weapon respawns (in multiplayer).
* **Weapon.Pickup** - weapon is getting picked up. Derived from **Item.Pickup**
* **Weapon.Empty** - player tries to shoot, but the weapon is out of ammo.
* **Weapon.Drop** - the weapon dropped by a monster falls onto the ground.

### Common NPC soundscripts

* **NPC.Gibbed** - played when monster (or a corpse) is gibbed.
* **NPC.BodyDrop_Heavy** - played on the event 2002 in the death sequence.
* **NPC.BodyDrop_Light** - played on the event 2001 in the death sequence.
* **NPC.SwishSound** - played on the event 2010 in the melee attack sequence.
* **NPC.AttackHit** - melee attack hit.
* **NPC.AttackMiss** - melee attack miss.
* **NPC.Reload** - generic reload sound.
* **NPC.9MMSingle** - single MP5 shot. Used by miniturrets and sentry turrets.
* **NPC.9MM** - burst MP5 shot. Used by human grunts, blackops male assassins and robogrunts.
* **NPC.GrenadeLaunch** - MP5 grenade launch.
* **NPC.Shotgun** - shotgun fire.
* **NPC.Sniper** - sniperrifle fire.
* **NPC.Handgun** - handgun fire.
* **NPC.Python** - Python fire.
* **NPC.DesertEagle** - Desert Eagle fire.
* **NPC.ReloadDesertEagle** - Desert Eagle reload.
* **NPC.M249** - M249 fire.
* **NPC.SpitTouch** - one of the sounds played when the spit projectile hits something. Used by bullsquid, big momma and gonome.
* **NPC.SpitHit** - one of the sounds played when the spit projectile hits something. Used by bullsquid, big momma and gonome.
* **NPC.Crash** - aircraft death sound. Used by apache and osprey.

### Player soundscripts

See [player soundscripts]({{< ref "player/#soundscripts" >}})
