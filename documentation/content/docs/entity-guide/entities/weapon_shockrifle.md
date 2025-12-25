---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/opfor.png) weapon_shockrifle

### Changes

* Plays idle animations (in Opposing Force it stays still unless player presses the secondary attack button).

### Skill variables

* See [shock_beam]({{< ref "shock_beam/#skill-variables" >}})
* **sk_plr_shockroach_discharge_factor** - the damage dealt when using the shockrifle underwater is calculated as the amount of primary ammo multiplied by this factor. The default value is 100.

### Weapon templates

{{% tabs %}}

{{% tab "Allow recharging in background" %}}

Unlike [weapon_hornetgun]({{< ref weapon_hornetgun >}}), the shock rifle doesn't recharge its ammo unless it's selected as the current weapon. It can be changed by setting `"only_when_deployed"` option to `false`.

```json
{
    "weapon_shockrifle": {
        "recharge": {
            "only_when_deployed": false
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}
