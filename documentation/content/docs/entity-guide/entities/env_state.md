---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) env_state

A simpler alternative to multisource, can be used as master to lock some entities.

### Reaction to use-type

* Handles use-type to change its internal state.
* When going to `Off` state the `env_state` fires its `Target` with `Off` use-type and its `Fire when turned off` with `Toggle` use-type.
* When going to `On` state the `env_state` fires its `Target` with `On` use-type and its `Fire when turned on` with `Toggle` use-type.
* If state doesn't change, no triggers will fire.

### Master capability

The internal state is reported. When `env_state` is `Off`, it acts as lock. When `env_state` is `On`, it unlocks dependent entities.
