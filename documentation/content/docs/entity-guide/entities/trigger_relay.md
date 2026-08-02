---
bookHidden: true
bookToC: false
---

# trigger_relay

[TWHL](https://twhl.info/wiki/page/trigger_relay)

### New parameters

* Parameter `Trigger State` got 2 new options: `Copy Input` (forward the input use-type) and `Reverse Input` (turn `Off` into `On` and vice versa).
* ![](/images/svencoop.png) `Delay Before Reset`. Time, in seconds, the `trigger_relay` can fire its target again after being triggered. Flag `Remove on fire` must be disabled for this to work.

### New spawnflags

* ![](/images/svencoop.png) `Forward activator` - pass the original activator when triggering targets (otherwise `trigger_relay` sends itself as activator).
* `Pass caller as activator` - pass the caller as activator when triggering the target. This has higher priority than `Forward activator`.
