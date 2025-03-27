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

{{% /tabs %}}
