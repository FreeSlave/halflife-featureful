---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_shocktrooper

### Skill variables

* **sk_shocktrooper_health** - monster's health. The number is multiplied by 2.5 to get the actual health value, replicating the behavior from Opposing Force.
* **sk_shocktrooper_kick** - melee damage.
* **sk_shocktrooper_gspeed** - the spore speed when it's thrown.
* **sk_shocktrooper_maxcharge** - the shockrifle clip size.
* **sk_shocktrooper_rchgspeed** - the time in seconds between recharging one point of shock ammo.
* [shock_beam skill variables]({{< ref "shock_beam/#skill-variables" >}}).

### Default classification

`Race X Shock`

### Soundscripts

* **ShockTrooper.Pain** - pain sounds.
* **ShockTrooper.Die** - death sounds.
* **ShockTrooper.Fire** - firing a shock beam projectile.
* [shock_beam soundscripts]({{< ref "shock_beam/#soundscripts" >}}).
* [spore soundscripts]({{< ref "spore/#soundscripts" >}}).

### Visuals

* **ShockTrooper.MuzzleFlash** - temporary flash sprite played when firing a shock projectile.
* [shock_beam visuals]({{< ref "shock_beam/#visuals" >}}).
* [spore visuals]({{< ref "spore/#visuals" >}}).

### Bugfixes

The original shock trooper model from Opposing Force (**models/strooper.mdl**) has a bug with `limpingwalk` sequence (used by shock trooper when it's low on health) - the sequence doesn't have a looping flag, so the monster plays the sequence once and then just moves towards its goal without animation. The fixed model is distributed with the sample mod. Make sure your shock trooper model has a looping flag on the `limpingwalk` sequence.
