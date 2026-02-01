---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_human_medic_ally

{{% hint info %}}
Despite the `ally` part in the entity name, the monster can be made enemy to the player via custom classification in the entity parameters or entity template.
{{% /hint %}}

### Changes

* Besides healing ally soldiers medics can also heal ally scientists and security guards.

### Skill variables

* **sk_medic_ally_health** - monster's health. If not defined, **sk_hgrunt_health** is used.
* **sk_medic_ally_kick** - kick damage. If not defined, **sk_hgrunt_kick** is used.
* **sk_medic_ally_gspeed** - the grenade speed when it's thrown. If not defined, **sk_hgrunt_ally_gspeed** is used.
* **sk_9mm_bullet** - handgun damage.
* **sk_357_bullet** - Desert Eagle bullet damage. Default value is 34.
* **sk_medic_ally_heal** - the amount of health charge.

### Default classification

`Player Ally Military`

### Soundscripts

* **MedicGrunt.Heal** - healing sound. Must be looped.
* **MedicGrunt.Pain** - pain sound. Derived from **HGruntAlly.Pain**
* **MedicGrunt.Die** - death sound. Derived from **HGruntAlly.Die**
* **MedicGrunt.CallMedic** - call another medic when low on health. Derived from **HGruntAlly.CallMedic**
* **MedicGrunt.Handgun** - handgun fire. Derived from **NPC.Handgun**
* **MedicGrunt.Reload** - handgun reload. Derived from **NPC.Reload**
* **MedicGrunt.DesertEagle** - Desert Eagle fire. Derived from **NPC.DesertEagle**
* **MedicGrunt.ReloadDesertEagle** - Desert Eagle reload. Derived from **NPC.ReloadDesertEagle**

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack rules" %}}
Uses same rules as [monster_human_grunt_ally]({{< ref monster_human_grunt_ally >}}).
{{% /tab %}}

{{% /tabs %}}
