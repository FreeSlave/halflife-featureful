---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_hwgrunt

Heavy Weapons Grunt from Sven Co-op. He drops [weapon_minigun]({{< ref weapon_minigun >}}) on death (if the weapon is enabled).

{{% hint info %}}
This monster was implemented with the **hwgrunt.mdl** form Sven Co-op 3 in mind (the model is distributed with the sample mod).

The model from Sven Co-op 5 has some changes to the model activities and the hwgrunt code in Featureful SDK has been adjusted to be compatible with this model as well. Still, Featureful SDK doesn't implement the Sven Co-op 5 behavioral trait when heavy weapons grunt involuntary drops minigun on taking high damage and draws the pistol.
{{% /hint %}}

### Skill variables

* **sk_hwgrunt_health** - monster's health.
* **sk_556_bullet** - M249 damage. Default value is 15.

### Default classification

`Human Military`

### Default display name

`Heavy Weapons Grunt`

### Soundscripts

* **HWGrunt.Pain** - pain sounds. Derived from **HGrunt.Pain**
* **HWGrunt.Die** - death sounds. Derived from **HGrunt.Die**
* **HWGrunt.Shoot** - minigun fire.
* **HWGrunt.Spindown** - minigun spindown.
* **HWGrunt.Spinup** - minigun spinup.
* **HWGrunt.Use** - start following the ally player. Derived from **HGrunt.Use**
* **HWGrunt.UnUse** - stop following the ally player. Derived from **HGrunt.UnUse**
