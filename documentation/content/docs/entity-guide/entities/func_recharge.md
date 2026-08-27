---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_recharge

[TWHL](https://twhl.info/wiki/page/func_recharge)

Has same changes as [func_healthcharger]({{< ref func_healthcharger >}})

### Skill variables

* **sk_suitcharger** - suit charger default capacity.
* **sk_suitcharger_rate** - amount of armor to charge per use 'tick'. Default (and minimum) value is 1.

### Soundscripts

* **SuitRecharge.Start** - player starts using the suit charger.
* **SuitRecharge.ChargingLoop** - player continues using the suit charger.
* **SuitRecharge.Deny** - the suit charger is out of charge.
* **SuitRecharge.Recharge** - the suit charger is getting recharged (in multiplayer). Empty by default.
