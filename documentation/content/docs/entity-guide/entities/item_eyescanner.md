---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/decay.png) item_eyescanner

The model-based retinal scanner from PS2 version of Half-Life.

When scanner receives the use input (e.g. gets fired by a trigger or directly used by a player), it first plays the animation and plays beeping sounds along with screen blinking. Then it can be respond in two different ways: *Grant access* and *Deny access*, depending on who or what sent the input. The appropriate sound is played and the proper target is fired.

If scanner gets input by the unlocker entity while it's in the active state caused by the input from something else, it will grant access. This makes sure that if player used the scanner right before the monster started its scripted sequence the retinal scanner will grant access, so the scripted input won't be lost due to the player's action.

The retinal scanner is non-solid by default. You're supposed to place an invisible `func_wall` for collision (recommended width is 8 units) or set the `Solid` spawnflag.

{{% hint info %}}
This entity can play sound [animation events]({{< ref model-animation-events >}}). Use entity template with [autoprecache_sounds]({{< ref "entity-templates/#autoprecache_sounds" >}}) enabled to precache such sounds.
{{% /hint %}}

### Parameters

* `Unlocker's name` - a targetname or a classname of the entity which will be perceieved as unlocker by the retinal scanner. When getting a use input the `item_eyescanner` checks the activator. If activator is empty, the caller is used instead. If this parameter is empty, non-player activator will be unlocker. Put `player` as a parameter value to allow players to get access granted on this retinal scanner.
* `Unlocked target` - fire when got input from an unlocker entity.
* `Locked target` - fire when got input from something that is not unlocker entity.
* `Custom Granted Sound` - a sound to play instead of samples of **EyeScanner.Granted** soundscript.
* `Custom Denied Sound` - a sound to play instead of samples of **EyeScanner.Denied** soundscript.
* `Custom Beeping Sound` - replacement for samples of **EyeScanner.Beep**.
* `Granted sentence` - replacement for samples of **EyeScanner.GrantedSentence**.
* `Denied sentence` - replacement for samples of **EyeScanner.DeniedSentence**.
* `Sentence delay` - custom delay before **EyeScanner.GrantedSentence** or **EyeScanner.DeniedSentence** is played.

### Spawnflags

* `Solid` - make the eye scanner solid.

### Skill variables

* **sk_eyescanner_sentence_delay** - delay before the **EyeScanner.GrantedSentence** or **EyeScanner.DeniedSentence** is played. This shouldn't really depend on the difficulty level, but the skill variable is just a convenient way to set the value. Default value is 0, i.e. no delay which means the sentence soundscripts will be played at the same time as base soundscripts (which may not sound well).

### Soundscripts

* **EyeScanner.Granted** - play when access is granted.
* **EyeScanner.Denied** - play when access is denied.
* **EyeScanner.Beep** - soundscript for the series of beeps during the eye scanner sequence.
* **EyeScanner.GrantedSentence** - play when acess is granted, after the sentence delay has passed. Empty by default.
* **EyeScanner.DeniedSentence** - play when acess is denied, after the sentence delay has passed. Empty by default.

Example of **sound/soundscripts.json** entries to enable sentences:

```json
{
    "EyeScanner.GrantedSentence": {
        "waves": ["!EA0"]
    },
    "EyeScanner.DeniedSentence": {
        "waves": ["!NA0"]
    }
}
```

Should work well with this **skill.cfg** entry:

```
sk_eyescanner_sentence_delay 0.4
```

Or if you prefer keeping the configuration to one file, you can add the definition to **templates/entities.json**:

```json
{
    "item_eyescanner": {
        "soundscripts": {
            "EyeScanner.GrantedSentence": {
                "waves": ["!EA0"]
            },
            "EyeScanner.DeniedSentence": {
                "waves": ["!NA0"]
            }
        },
        "skill": {
            "eyescanner_sentence_delay": 0.4
        }
    }
}
```
