---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_pushable

[TWHL](https://twhl.info/wiki/page/func_pushable)

Has same changes as [func_breakable]({{< ref func_breakable >}}) (for breakable pushables).

### New parameters

* `Collision with corpses` - allows to ignore collision with dead monsters. This will ensure that the dead body won't block the pushable.
* `Instantly gib corpses` - instantly gib corpses on touch. Of course this won't do anything if `Collision with corpses` is set to not collide with corpses.
* `How to handle tiny creatures` - override handling of [tiny creatures]({{< ref "monsters/#tiny-creatures" >}}).

### Calc Ratio

Reports the *health* / *max health* fraction.

### Soundscripts

* **Pushable.Move** - played when pushed.
* [func_breakable]({{< ref func_breakable >}}) soundscripts (if `Breakable` spawnflag is enabled).

### Cvars

* `pushablemode`. 0 is Anniversary HL (fix excessive acceleration, default). -1 is legacy Half-Life (allow accelerated push with +use). 1 is XashXT (allow pushing sideways. Require going back when pressing +use on the pushable).
