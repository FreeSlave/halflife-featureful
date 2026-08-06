---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) env_model

The (optionally) animated model that can play two different sequences depending on its state.

{{% hint info %}}
This entity can play sound [animation events]({{< ref model-animation-events >}}). Use entity template with [autoprecache_sounds]({{< ref "entity-templates/#autoprecache_sounds" >}}) enabled to precache such sounds.
{{% /hint %}}

### Parameters

* `Sequence when on` - the sequence name to play in On state.
* `Behaviour when on`
* * `Freeze when sequence ends` - stop animiating.
* * `Loop` - continue looping the animation (only if animation itself is looped).
* * `Change state when sequence ends` - switch to Off state.
* `Sequence when off` - the sequence name to play in Off state.
* `Behaviour when off`
* * `Freeze when sequence ends` - stop animiating.
* * `Loop` - continue looping the animation (only if animation itself is looped).
* * `Change state when sequence ends` - switch to On state.
* `Starting frame` - configure the starting frame. Useful if you want to desync animations of several `env_model` instances located in the same area and using the same model. Negative values mean the random starting frame.
* `Framerate when on` and `Framerate when off` - configure the animation framerate depending on the state.

### Spawnflags

* `Initially Off` - initial state if off.
* `Drop to Floor` - drop to floor when level starts.
* `Solid` - add small solid box.
* `Client-side animation` - whether the model is animated client-side. This may help fixing some visible stuttering when playing animation.
* `Always send to client (ignore PVS)` - make sure the model is sent to the client independent of player's PVS. This ensures the model will render even if its origin is not visible to the player. Can be used on large models for skybox imitation.

### Reaction to use-type

Handles use-types to switch the animation depending on its state.
