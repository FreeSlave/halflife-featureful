---
bookHidden: true
bookToC: false
---

# multi_manager

[TWHL](https://twhl.info/wiki/page/multi_manager)

* ![](/images/svencoop.png) Configuring use-types is allowed using delay value suffixes: `#0` for Off, `#1` for On, `#2` for Kill.
* In addition to Sven Co-op like suffixes, the following are implemented as alternatives to digit suffixes: `#off`, `#on`, `#kill`.
* Two additional suffixes are `#forward` and `#reverse` used to forward or forward-reversed the use type the `multi_manager` was triggered with. Example: if `multi_manager` was triggered with `On` use type, all targets which have `#forward` suffix in the delay value will be triggered with `On`. All targets which have `#reverse` suffix in the delay will be triggered with `Off`.

### New spawnflags

* ![](/images/spirit.png) `Once only` flag to remove multi-manager after it fires all its targets.
