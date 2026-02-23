---
bookHidden: true
bookToC: false
---

# scripted_sentence

[TWHL](https://twhl.info/wiki/page/scripted_sentence)

* Buttons and cyclers can be speakers for scripted sentences (like in HL 25 anniversary).
* `Speaker` can be `*locus` (`!activator`) for chained scripted sentences.
* `Listener type` can be `*locus` (`!activator`) or `*player`.

### New parameters

* `Required state` - require a certain monster state for saying a sentence.
* `Follow action` - make a monster start or stop following upon saying a sentence.
* `Test against search radius` - do the radius check when searching for monster by its targetname.
* `Speaker Search Origin` - set a different entity to search the speaker around instead of the `scripted_sentence` itself.
* More options for `Sound Radius` parameter.
* `Target's Activator` - allows to set activator to either script itself or the speaking monster when triggering the script target.
* `Allow use during speech` - allow monster start or stop following the player when `+use` is pressed while the monster is speaking its scripted sentence.

### New spawnflags

* `Require listener` - make sentence playable only if listener is present.
* `Listener looks at speaker` - make listener look at speaker if possible.
* `Speaker turns to listener` - make speaker to turn their body to the listener if possible (not only the head).
* `Listener turns to speaker` - make listener to turn their body to the speaker if possible (not only the head).
* `Try once`. By default once triggered the script will continue to search for the target monster if it could not find one instantly. This flag prevents such behavior.

{{% hint info %}}
When the `scripted_sentence` is played on the brush entity (like a button) this entity must have an origin brush for `Requires listener` to work correctly.
{{% /hint %}}
