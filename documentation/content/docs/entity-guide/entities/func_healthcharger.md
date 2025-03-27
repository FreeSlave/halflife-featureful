---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_healthcharger

[TWHL](https://twhl.info/wiki/page/func_healthcharger)

### New parameters

* ![](/images/svencoop.png) `Custom Juice Amount`.
* ![](/images/svencoop.png) `Custom Loop Sound`, `Custom Denied Sound` and `Custom Start Sound`.
* `Custom Recharge Sound`.
* ![](/images/svencoop.png) `Trigger On Empty` and `Trigger On Recharged`.
* `Trigger On First Use`.

### Reaction to use-type

Can be turned off and on by a trigger.

### Calc Ratio

Reports the fraction of juice left (a number between 0 and 1 inclusive).

### Skill variables

* **sk_healthcharger** - health charger default capacity.

### Soundscripts

* **WallHealth.Start** - player starts using the medkit.
* **WallHealth.ChargingLoop** - player continues using the medkit.
* **WallHealth.Deny** - the medkit is out of charge.
* **WallHealth.Recharge** - the medkit is getting recharged (in multiplayer).
