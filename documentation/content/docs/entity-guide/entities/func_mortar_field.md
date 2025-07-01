---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_mortar_field

[TWHL](https://twhl.info/wiki/page/func_mortar_field)

### New parameters

* `Custom delay before first strike` - delay in seconds before the first mortar strike in sequence of strikes. If omitted or 0, the default value 2.5 is used.

### New spawnflags

* `Don't set activator as attacker` - the activator of the `func_mortar_field` won't be used as an attacker for mortar strikes. By default the activator is sent as an attacker. E.g. even if player activated the `func_mortar_field` via [trigger_once]({{< ref trigger_once >}}), the mortar strikes will be attributed to the player, thus making monsters who were damaged by the strike think that they have been attacked by the player. Use this flag to prevent such behavior if it's undesired. As an alternative you can also put the [trigger_relay]({{< ref trigger_relay >}}) between the calling trigger and the `func_mortar_field` to ensure the original activator (usually the player) is not used.

### Skill variables

* **sk_mortar** - explosion damage of the mortar hit.

### Soundscripts

* **Mortar.Launch** - when `func_mortar_field` fires. The length of the audio should roughly match the delay before the strike.

### Visuals

* **Mortar.Beam** - the beam visual representing the trail effect of the mortar strike (from top to bottom).
* **Mortar.Circle** - the cicle beam visual representing the shock wave from the impact. The sprite for this visual is not defined by default, thus the wave doesn't appear unless the sprite is defined. The rest of parameters use the values left by Valve in the code that was disabled (they might have been thinking the effect doesn't fit, but you can give it a try).
