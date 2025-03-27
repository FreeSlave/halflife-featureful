---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/decay.png) item_eyescanner

The model-based retinal scanner from PS2 version of Half-Life.

When scanner receives the use input (e.g. gets fired by a trigger or directly used by a player), it first plays the animation and plays beeping sounds along with screen blinking. Then it can be respond in two different ways: *Grant access* and *Deny access*, depending on who or what sent the input. The appropriate sound is played and the proper target is fired.

If scanner gets input by the unlocker entity while it's in the active state caused by the input from something else, it will grant access. This makes sure that if player used the scanner right before the monster started its scripted sequence the retinal scanner will grant access, so the scripted input won't be lost due to the player's action.

### Parameters

* `Unlocker's name` - a targetname or a classname of the entity which will be perceieved as unlocker by the retinal scanner. When getting a use input the `item_eyescanner` checks the activator. If activator is empty, the caller is used instead. If this parameter is empty, non-player activator will be unlocker. Put `player` as a parameter value to allow players to get access granted on this retinal scanner.
* `Unlocked target` - fire when got input from an unlocker entity.
* `Locked target` - fire when got input from something that is not unlocker entity.
