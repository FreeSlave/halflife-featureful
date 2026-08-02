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

`Human Military`, or `Blackops` if `blackops_classify` server feature is enabled.

### Default display name

`Female Assassin`

### Soundscripts

* **HAssassin.Shot** - silenced handgun shot.
* **HAssassin.Footstep** - footstep sound.
* **HAssassin.Cloak** - going invisible.
* **HAssassin.Pain** - pain sounds. Empty by default.
* **HAssassin.Die** - death sounds. Empty by default.
* **HAssassin.Use** - start following the ally player. By default it uses the `HA_OK` sentence group.
* **HAssassin.UnUse** - stop following the ally player. By default it uses the `HA_WAIT` sentence group.

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
    "monster_human_assassin": {
        "squad_capability": {
            "can_recruit": true
        }
    }
}
```
{{% /tab %}}

{{% tab "Voice lines" %}}
Reusing Gina's voice lines from Decay for human assassin.
```json
{
    "monster_human_assassin": {
        "soundscripts": {
            "HAssassin.Pain": {
                "waves": ["gina/gina_pain0.wav", "gina/gina_pain1.wav", "gina/gina_pain2.wav", "gina/gina_pain3.wav"],
                "volume": 0.6,
                "pitch": 95
            },
            "HAssassin.Die": {
                "waves": ["gina/gina_die0.wav", "gina/gina_die1.wav", "gina/gina_die2.wav"],
                "volume": 0.6,
                "pitch": 95
            },
            "HAssassin.Use": {
                "waves": ["gina/gina_use0.wav", "gina/gina_use1.wav", "gina/gina_use2.wav", "gina/gina_use3.wav", "gina/gina_use4.wav", "gina/gina_use5.wav"],
                "volume": 0.6,
                "pitch": 95
            },
            "HAssassin.UnUse": {
                "waves": ["gina/gina_unuse0.wav", "gina/gina_unuse1.wav", "gina/gina_unuse2.wav", "gina/gina_unuse3.wav", "gina/gina_unuse4.wav"],
                "volume": 0.6,
                "pitch": 95
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
