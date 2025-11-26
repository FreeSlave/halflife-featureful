---
bookHidden: true
bookToC: false
---

# monster_osprey

[TWHL](https://twhl.info/wiki/page/monster_osprey)

### New parameters

* `Rotor volume` - configure rotor volume.
* `Sound attenuation` parameter to configure rotor sound radius.
* ![](/images/svencoop.png) New parameter `Grunt Type` to allow deploying Opposing Force grunts instead of Half-Life ones.
* ![](/images/svencoop.png) `Number of grunts` parameter to override the maximum number of grunts deployed at once.

### New spawnflags

* `No deploy, just fly` - make osprey not care about grunt deploying and just fly on specified path. Such Osprey does not skip path corners with low speed like the default one does to find the deploy point.

### Skill variables

* **sk_osprey** - monster's health.

### Default classification

`Machine`

### Soundscripts

* **Osprey.Rotor** - rotor sound during the flight. Must be looped.
* **Osprey.Crash** - death sound. Derived from **NPC.Crash**

### Visuals

* **Osprey.FallingSmoke** - smokes produced while Osprey is falling.
* **Osprey.DamageSmoke** - smoke indication of being damaged.
* **Osprey.Fireball** - big fireball played when Osprey crashes.
* **Osprey.BlastCircle** - wave played when Osprey crashes.
* **NPC.Rope** - the rope sprite for the rappelling grunts.
