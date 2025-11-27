---
bookHidden: true
bookToC: false
---

# monster_osprey

[TWHL](https://twhl.info/wiki/page/monster_osprey)

### Changes

* Can be correctly spawned via [monstermaker]({{< ref monstermaker >}}) (notifies the monstermaker about its death). Use a custom `#target` keyvalue parameter so the spawned Osprey has the path to follow. Set the `#maxlivechildren` keyvalue parameter to ensure the resupply won't depend on the number of grunts alive on the map at the moment when the Osprey spawns. See [Configuring the spawned monster]({{< ref "monstermaker/#configuring-the-spawned-monster" >}}) and the **flyers_demo** map in the sample mod.
* `Death` and `50% Health Remaining` trigger conditions now work for Osprey.

### New parameters

* `Rotor volume` - configure rotor volume. This can also be configured via **Osprey.Rotor** soundscript.
* `Rotor sound attenuation` - configure rotor sound attenuation. The less the value the larger the sound radius. The default attenuation is 0.15. This can also be configured via **Osprey.Rotor** soundscript.
* ![](/images/svencoop.png) `Grunt Type` - deploy Opposing Force grunts instead of Half-Life ones.
* ![](/images/svencoop.png) `Number of grunts per deploy` - override the maximum number of grunts deployed at once. If defined this must be a number from 1 to 4.
* `Custom number of grunts to resupply` - override the maximum number of alive grunts the Osprey can resupply. After spawning this number of grunts the Osprey will stop deploying new ones until someone has died or been removed from the map.

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
