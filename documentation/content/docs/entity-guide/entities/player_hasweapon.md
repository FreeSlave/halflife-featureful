---
bookHidden: true
bookToC: false
---

# player_hasweapon

Checks if player has a specified weapon (defined by weapon's classname, e.g. `weapon_crowbar`) and fire triggers accordingly.

{{% hint warning %}}
Weapon name aliases like `weapon_mp5` won't work. Use the primary weapon entity name (`weapon_9mmAR`).
{{% /hint %}}

### Master capability

If player has a specified item, the master state is On. Otherwise it's Off.
