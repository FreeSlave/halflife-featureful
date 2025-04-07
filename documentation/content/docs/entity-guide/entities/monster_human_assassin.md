---
bookHidden: true
bookToC: false
---

# monster_human_assassin

[TWHL](https://twhl.info/wiki/page/monster_human_assassin)

### Skill variables

* **sk_hassassin_health** - monster's health.
* **sk_hassassin_cloaking** - whether the cloaking is enabled. By default it's set to 0 on easy and normal, and to 1 on hard difficulty (replicating the behavior from Half-Life).
* **sk_9mm_bullet** - handgun bullet damage.

### Default classification

`Human Military` or `Blackops` if `blackops_classify` server feature is enabled.

### Soundscripts

* **HAssassin.Shot** - silenced handgun shot.
* **HAssassin.Footstep** - footstep sound.
* **HAssassin.Cloak** - going invisible.
* **HAssassin.Pain** - pain sounds. Empty by default.
* **HAssassin.Die** - death sounds. Empty by default.

### Attacks

* *Melee Attack 1* - jump.
* *Range Attack 1* - pistol attack.
* *Range Attack 2* - throw grenade.

### Animation events

* `1` - fire a pistol. Play **HAssassin.Shot** soundscript.
* `2` - throw a timed grenade.
* `3` - jump.

### Entity template examples

{{% tabs %}}

{{% tab "Allow squad leaders" %}}
Allow female assassins to be squad leaders (in original Half-Life they can't).
```json
{
    "monster_alien_slave": {
        "squad_capability": {
            "can_recruit": true
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
