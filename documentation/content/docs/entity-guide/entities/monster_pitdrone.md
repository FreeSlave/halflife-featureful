---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_pitdrone

### Changes

* Restores health when eating.
* Fixed melee attack damage - in Opposing Force primary and secondary melee attack damage was mixed up.
* When spawning via monstermaker, pitdrone always spawns with spikes loaded. In Opposing Force pitdrones spawned without spikes and unable to reload.

### Skill variables

* **sk_pitdrone_health** - monster's health.
* **sk_pitdrone_dmg_bite** - single claw attack damage.
* **sk_pitdrone_dmg_whip** - double claw attack damage.
* **sk_pitdrone_dmg_spit** - spike damage.

Note: the skill variable names might be confusing as pitdrones don't bite and don't have a tail to whip, but these are the names that are used in Opposing Force.

### Default classification

`Race X Predator`

### Soundscripts

* **PitDrone.Idle** - idle sounds.
* **PitDrone.Alert** - alert sounds.
* **PitDrone.Pain** - pain sounds.
* **PitDrone.Die** - death sounds.
* **PitDrone.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **PitDrone.AttackHit** - hitting something when attacking with both claws.
* **PitDrone.SpikeHitWorld** - when spike hits the world. Emitted from the spike projectile.
* **PitDrone.SpikeHitBody** - when spike hits someone. Emitted from the spike projectile.

Note: the pitdrone uses a lot of sounds in the sequence events. These sounds can't be changed via soundscripts.

### Visuals

* **Pitdrone.Spike** - spike projectile.
* **Pitdrone.SpikeTrail** - a trail beam following the projectile.
* **Pitdrone.TinySpit** - sprites sprayed out of pitdrone head on spike attack.

### Entity template examples

{{% tabs %}}

{{% tab "Let pitdrones open doors" %}}
*In Half-Life: Opposing Force pitdrones can't open doors.*
```json
{
    "monster_pitdrone": {
        "open_door_capability": true
    }
}
```
{{% /tab %}}

{{% /tabs %}}
