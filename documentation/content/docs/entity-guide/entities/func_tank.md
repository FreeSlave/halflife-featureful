---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_tank

[TWHL](https://twhl.info/wiki/page/func_tank)

### New parameters

* `Bullet count` - configure a limited number of bullets for tank.
* ![](/images/spirit.png) `Behavior` - set the relationship classification for the tank. Allow tank to attack monsters, not only the player.
* ![](/images/spirit.png) `Trigger on firing (locus = barrel)` - a target to trigger when tank is firing a shot, with the end of the barrel as activator (the special invisible entity is created automatically). This will work even if tank doesn't actually use bullets.
* ![](/images/spirit.png) `Fire Master` - allows to lock the firing ability (the tank still can rotate).
* `Extra barrels name` - the name of other tank entities which will fire their shots along with main one.
    - Tanks that act as extra barrels must be deactivated (so they don't act on their own).
    - Their origin brush should be placed at the same place as the origina brush of the main tank. The `Barrel Horizontal` and `Barrel Vertical` offsets should be provided accordingly to the geometry.
    - In fact, tanks acting as barrels can consist of single origin brush only.
    - Barrel tanks can still have different fire rates, but they currently can't fire faster than the main tank, so it's better if the main tank has the same or higher fire rate.
    - The main tank can be a fake tank (i.e. not firing anything) used only for visuals and as the rotation reference of barrels which actually do damage.
    - You can temporarily disable certain barrels from the set via `Fire Master`.
* `Custom player weapon volume` - the radius of AI sound produced by a player when he's firing a tank. If it's zero or omitted the default value 1000 is used. Set to negative value to disable AI sound for tank shots.

{{% hint info %}}
![](/images/spirit.png) The tank can be controlled by [scripted_tanksequence]({{< ref scripted_tanksequence >}}).
{{% /hint %}}

### New spawnflags

* ![](/images/spirit.png) `Laser spot` - show laser spot when the tank is used.
* ![](/images/spirit.png) `Match Target` - make controllable tank rotations match player's view target (center of the screen), instead of just following the player's angles. This is also a recommended option for setups where multiple tanks are controlled by single [func_tankcontrols]({{< ref func_tankcontrols >}}), so the tanks fire at the same target.

### Bugfixes

* Controllable tanks now can be removed from the map without a risk of crashing the game if tank was removed during the player's usage.
* `func_tank` properly finds the player when the origin of the tank is inside the worldbrush.

### Notes

* The `Controllable` spawnflag is not required anymore. [func_tankcontrols]({{< ref func_tankcontrols >}}) targetting the tank is enough. The `Controllable` spawnflag basically prevents tanks from being activated and deactivated via triggers.
