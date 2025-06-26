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

{{% hint info %}}
![](/images/spirit.png) The tank can be controlled by [scripted_tanksequence]({{< ref scripted_tanksequence >}}).
{{% /hint %}}

### New spawnflags

* ![](/images/spirit.png) `Laser spot` - show laser spot when the tank is used.
* ![](/images/spirit.png) `Match Target` - make controllable tank rotations match player's view target (center of the screen), instead of just following the player's angles.

### Bugfixes

* Controllable tanks now can be removed from the map without a risk of crashing the game if tank was removed during the player's usage.
* `func_tank` properly finds the player when the origin of the tank is inside the worldbrush.
