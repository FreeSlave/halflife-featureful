---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_gonome

### Changes

* Restores health when eating.

### Skill variables

* **sk_gonome_health** - monster's health.
* **sk_gonome_dmg_one_slash** - melee damage.
* **sk_gonome_dmg_guts** - guts projectile damage.
* **sk_gonome_dmg_one_bite** - mouth bite damage.

### Default classification

`Alien Monster`

### Soundscripts

* **Gonome.Idle** - idle sounds.
* **Gonome.Alert** - alert sounds.
* **Gonome.Pain** - pain sounds.
* **Gonome.Die** - death sounds.
* **Gonome.Bite** - bite sounds on mouth attack.
* **Gonome.Melee1** - right arm melee attack.
* **Gonome.Melee2** - left arm melee attack.
* **Gonome.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Gonome.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **Gonome.SpitTouch** - one of the sounds played when the guts projectile hits something. Emitted from the guts projectile. Derived from **NPC.SpitTouch**
* **Gonome.SpitHit** - one of the sounds played when the guts projectile hits something. Emitted from the guts projectile. Derived from **NPC.SpitHit**

### Visuals

* **Gonome.Guts** - guts projectile visual.

### Entity template examples

{{% tabs %}}

{{% tab "Precache step sounds" %}}
The gonome model refers to nonexistent step sounds in animation events. Even if you provide the needed sounds in the mod resources, they still won't play because they're not precached in the code. Use this template to force precaching of step sounds.
```json
{
    "monster_gonome": {
        "precached_sounds": ["gonome/gonome_step1.wav", "gonome/gonome_step2.wav", "gonome/gonome_step3.wav", "gonome/gonome_step4.wav"]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
