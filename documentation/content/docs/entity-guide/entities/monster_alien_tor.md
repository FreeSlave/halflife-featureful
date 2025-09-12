---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_alien_tor

Alien commander from Sven Co-op capable of spawning up to 3 alien grunts.

{{% hint warning %}}
The Tor's energy beam and slam attacks can launch enemies upwards, so make sure the player won't access any unintended area with the help of these.
{{% /hint %}}

### Skill variables

* **sk_tor_health** - monster's health.
* **sk_tor_punch** - melee damage.
* **sk_tor_energybeam** - damage per energy beam tick.
* **sk_tor_sonicblast** - maximum slam attack damage.

### Default classification

`Alien Military`

### Soundscripts

* **Tor.Idle** - idle sounds.
* **Tor.Alert** - alert sounds.
* **Tor.Pain** - pain sounds.
* **Tor.Die** - death sounds.
* **Tor.Attack** - sound to play on melee attack.
* **Tor.LeftFoot** - left foot step sound (event 11).
* **Tor.RightFoot** - left foot step sound (event 10). This is the same as **Tor.LeftFoot** by default.
* **Tor.Slam** - slam attack sounds.
* **Tor.Shoot** - energy beam attack sound.
* **Tor.Summon** - sound to play when Tor starts summoning.
* **Tor.SummonPortal** - sound to play when the portal is opened. This is played at the summon point.
* **Tor.SummonSpawn** - sound to play when the summon is finished. This is played at the summon point.
* **Tor.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Tor.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **Tor.Use** - start following the ally player. Derived from **Tor.Idle**
* **Tor.UnUse** - stop following the ally player. Derived from **Tor.Alert**

### Visuals

* **Tor.Slam** - slam attack beam.
* **Tor.Beam** - energy beam.
* **Tor.Beam2** - a spiral energy beam played along with the main one.
* **Tor.SummonBeam** - the temporary beam linking the Tor and the portal he creates.
* **Tor.SummonSprite** - the portal sprite.

### Entity template examples

{{% tabs %}}

{{% tab "Friendly Tor" %}}
```json
{
    "tor_friendly": {
        "inherits": "monster_alien_tor",
        "own_visual": {
            "model": "models/Torf.mdl"
        },
        "classify": "Player Ally",
        "visuals": {
            "Tor.Beam": {
                "color": [16, 255, 255]
            },
            "Tor.Beam2": {
                "color": [128, 255, 255]
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
