---
bookHidden: true
bookToC: false
---

# scripted_sequence

[TWHL](https://twhl.info/wiki/page/scripted_sequence)

`scripted_sequence` sometimes is very clunky to use in Half-Life. **Featureful SDK** provides a lot of new parameters and flags to get around its limitations, while preserving the original behavior for the sake of compatibility.

### New parameters

* New option for `Move to position` - `Teleport`. Unlike `Instantaneous` it does not require scripted sequence to be called twice in order for monster to proceed playing animation. The `Instantaneous` is left for compatibility purposes and considered to be legacy.
* `Interruption policy`. The alternative to the `No Interruptions` flag (which is considered legacy). This can be set to `By Death Only` which would allow to make the script uninterruptable unless the monster dies during the script. The `No Interruptions` flag should be unmasked in order for policy to work.
* `Fire on Possession` - fire a trigger after the monster gets possessed by script.
* `Fire after moving` - fire a trigger when the scripted movement is done.
* `Fire on Animation start` allows to sync animation start and firing a trigger (e.g. useful for retina scanner animations). Note that save-load in Half-Life resets the animation, but the trigger is designed to fire only once to avoid unwanted side-effects.
* `Target's Activator` allows to set activator to either script itself or the possessed monster when triggering a script target at the end of sequence or at the animation start.
* ![](/images/spirit.png) `Turn Target` makes monster face the specified target instead of a script entity.
* ![](/images/spirit.png) `Turn mode` makes monster turn to turn target (or script entity if turn target is omitted) position instead of aligning with its facing.
* ![](/images/spirit.png) `Move Target` makes monster to walk/run/teleport to the location of the specified entity instead of a script entity.
* ![](/images/svencoop.png) `Move to radius` makes monster stop moving when they approach the provided distance to the move target.
* ![](/images/spirit.png) `Priority` allows a script to possess the monster even if they're currently possessed by another script.
* `Required follower state` - the script will play depending on whether the monster is following the player or not.
* ![](/images/spirit.png) `Repeat action X more times` repeats an animation multiple times.
* `Search target monster by...`. By default `scripted_sequence` is searching a monster by targetname and then by classname (using the name provided in `Target Monster`). This is usually redundant and can be unwanted. This parameter allows to set whether you want the script to search the monster by targetname only or by classname only.
* `Test against search radius`. By default search radius is taken into account only when searching for a monster by a classname. This parameter allows to apply the same behavior when searching the monster by the targetname.
* `TakeDamage policy` - set the monster to be invulnerable or not killable during the script.
* `Master` - allows to prevent the monster possession until master entity is unlocked.
* `Delay before initial search` - delay before the initial target monster search, e.g. if script is autosearch or it uses the idle animation. Default value is 1.0. Set this to some low value if you want the monster to teleport to play idle animation sooner when the level is loaded.
* `Delay between search attempts` - delay between attempts to find and possess a target monster (if no monster found the next check is delayed). Default value is 1.0.
* `Action Animation framerate` - custom framerate for the action animation. Values higher than 1 mean faster animation. Values lower than 1 mean slower animation.

### New spawnflags

* ![](/images/opfor.png) `No Reset Entity (Continuous animation)` allows a smooth transition between animations that are meant to play one after another.
* `Apply new angles from bones`. If monster turns during the animation, set new angles after sequence finished (example: houndeye's jumpwindow animation).
* `Auto-Search (act as unnamed)` makes the `scripted_sequence` to act as if doesn't have a name. This allows to remove the script from the map while in-game with some other entity's `KillTarget` while preserving the behavior of the unnamed script.
* `Try search once`. By default once triggered the script will continue to search for the target monster if it could not find one instantly. This flag prevents such behavior making `scripted_sequence` do the search only once per firing. If it could not find the appropriate monster, nothing happens.
* `Don't reset head turn`. When monster is talking to someone else, the monster turns its head to the listener. By default playing `scripted_sequence` on the monster resets the head turn. Use this flag to avoid such behavior.
* `Force idle looping` for force idle animation looping even if it doesn't have a loop flag.
* `Remove on interruption`. This flag is useful in scripts with idle animation to ensure that the script won't be activated by some external trigger again after it was interrupted by damage dealt to the possessed monster (which could lead to weird situations like monster starting playing the idle animation again).
* `Don't Drop to Floor` - don't try instantly dropping the monster to floor after the sequence has finished. The monster still may naturally fall due to gravity after that.
* `Allow stuck` - allow monster being stuck after the sequence has done (if the animation included some movement, e.g. some jump/leap sequence). Otherwise in case of becoming stuck the monster will just get teleported to its position before the animation.
* `Allow unnamed idle` - make the unnamed scripted sequence with idle animation continue playing the idle animation instead of finishing the sequence (like it does in Half-Life).

### Bugfixes

* The game doesn't crash anymore if `scripted_sequence` is deleted while script is played. Instead the script gets cancelled.
* Fixed repeatable `scripted_sequence` not properly working after it got interrupted and triggered again.
* Fixed `scripted_sequence` being unresponsive when it has Idle Animation and Turn to Face move type
* Barnacles can't grab victims possessed by non-interruptible scripts (this is to prevent monster forgetting about the script if it got 'barnacled').
* Fixed monster not resetting the animation right away if the `scripted_sequence` with Idle Animation but no Action Animation has been triggered or interrupted (in Half-Life the monster may continue playing the idle animation for a while even after exiting the script).
