---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/opfor.png) weapon_shockrifle

### Changes

* Plays idle animations (in Opposing Force it stays still unless player presses the secondary attack button).

### Skill variables

See [shock_beam]({{< ref "shock_beam/#skill-variables" >}})

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
