---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_human_torch_ally

{{% hint info %}}
Despite the `ally` part in the entity name, the monster can be made enemy to the player via custom classification in the entity parameters or entity template.
{{% /hint %}}

### Skill variables

* **sk_torch_ally_health** - monster's health. If not defined, **sk_hgrunt_health** is used.
* **sk_torch_ally_kick** - kick damage. If not defined, **sk_hgrunt_kick** is used.
* **sk_torch_ally_gspeed** - the grenade speed when it's thrown. If not defined, **sk_hgrunt_ally_gspeed** is used.
* **sk_357_bullet** - Desert Eagle bullet damage. Default value is 34.

### Default classification

`Player Ally Military`

### Soundscripts

* **TorchGrunt.Pain** - pain sound. Derived from **HGruntAlly.Pain**
* **TorchGrunt.Die** - death sound. Derived from **HGruntAlly.Die**
* **TorchGrunt.CallMedic** - call medic when low on health. Derived from **HGruntAlly.CallMedic**
* **TorchGrunt.DesertEagle** - Desert Eagle fire. Derived from **NPC.DesertEagle**
* **TorchGrunt.ReloadDesertEagle** - Desert Eagle reload. Derived from **NPC.ReloadDesertEagle**

### Visuals

* **TorchGrunt.Beam** - the torch beam.
* **TorchGrunt.DynLight** - the dynamic light played during the cutting sequence.
* **TorchGrunt.EntLight** - entity light played during the cutting sequence.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack rules" %}}
Uses same rules as [monster_human_grunt_ally]({{< ref monster_human_grunt_ally >}}), but also has a special rule for hitgroup 8 (the tank) which is currently not configurable.
{{% /tab %}}

{{% /tabs %}}
