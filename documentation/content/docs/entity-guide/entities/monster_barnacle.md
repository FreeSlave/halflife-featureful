---
bookHidden: true
bookToC: false
---

# monster_barnacle

[TWHL](https://twhl.info/wiki/page/monster_barnacle)

### Changes

* Barnacles won't grab monsters in Non-interruptible scripts.

### Skill variables

* **sk_barnacle_health** - monster's health.

{{% hint info %}}
When attacked with player's melee weapon a barnacle will die with one hit no matter the current health. This is the original Half-Life behavior.
To change that you can create an [entity template]({{< ref "entity-templates/#take_damage" >}}) with a custom `take_damage` rules.
{{% /hint %}}

### Default classification

`Alien Monster`

### Soundscripts

* **Barnacle.Bite** - biting the prey when it's lifted.
* **Barnacle.Chew** - random chewing sound when the prey is lifted and when the gib is spewed out.
* **Barnacle.Alert** - when starting lifting the prey.
* **Barnacle.Die** - death sound.
* **Barnacle.Pain** - pain sound.

### Entity template examples

{{% tabs %}}

{{% tab "Disallow grappling to barnacles" %}}
```json
{
    "monster_barnacle": {
        "size_for_grapple": "no"
    }
}
```
{{% /tab %}}

{{% tab "Take Damage rules" %}}
The [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_barnacle": {
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["club"]
                },
                "modifier": {
                    "dmg": "=health"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
