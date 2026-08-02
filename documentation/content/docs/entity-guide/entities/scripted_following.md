---
bookHidden: true
bookToC: false
---

# scripted_following

Make the monster start or stop following the player. This works only on the [following monsters]({{< ref "monsters/#following-monsters-features" >}}).

{{% hint info %}}
If the monster was asked to follow the player via the `scripted_following` but couldn't fulfill request because he was in the [scripted sequence]({{< ref scripted_sequence >}}) at the moment, the `scripted_following` will keep trying to make the monster follow the player, waiting for the monster to leave the scripted state.
{{% /hint %}}

{{% hint info %}}
The monster won't say its Use and Unuse voice lines when made start or stop following by this entity.
{{% /hint %}}

### Parameters

* `Affected Monster` - the targetname of the monster. This will affect only the first suitable target even if there're many monsters with the same targetname.
* `Use type` - how the entity interprets its firing.
    - `Forward input` - the *ON* input will make the monster start following, the *OFF* input will make the monster stop following, and *TOGGLE* will make it start/stop following depending on its current state.
    - `Make start following` - make monster start following the player or do nothing if he's already following.
    - `Make stop following` - make monster stop following the player or do nothing if he's not currently following.
* `Fire on start following` - target to fire if the monster started following the player due to the activation of this entity. This will fire only if the following state has changed, i.e. it won't fire if monster has been following the player already.
* `Fire on stop following` - target to fire if the monster stopped following the player due to the activation of this entity. This will fire only if the following state has changed, i.e. it won't fire if monster wasn't following the player.

### Spawnflags

* `Remove on fire` - whether to remove the entity after the first successful use.

### Reaction to use-type

Depends on the `Use type` parameter.
