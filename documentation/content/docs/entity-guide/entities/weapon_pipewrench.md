---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/opfor.png) weapon_pipewrench

### Skill variables

* **sk_plr_pipewrench** - damage of the first hit. Consecutive hits damage is halved.
* **sk_plr_pipewrench_wind_base** - big swing (secondary attack) base damage. Default value is 25.
* **sk_plr_pipewrench_wind_factor** - big swing additional damage factor. Used to calculate the additional damage based on the charge time (the higher the value the less time player needs to reach the maximum damage). If not defined, **sk_plr_pipewrench** is used.
* **sk_plr_pipewrench_wind_max** - big swing maximum damage. Default value is 150.

The big swing attack damage is calculated by formula:

```
MIN(sk_plr_pipewrench_wind_base + sk_plr_pipewrench_wind_factor * time_since_charge_start, sk_plr_pipewrench_wind_max)
```
