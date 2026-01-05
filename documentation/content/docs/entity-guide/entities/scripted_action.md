---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) scripted_action

Similar to [scripted_sequence]({{< ref scripted_sequence >}}) but makes a monster play a certain activity (usually the attack) instead of the named animation.

For proper usage:

* Set `Move to Position` to `No - Turn to Face`
* Set `Turn mode` to `Turn to face`
* Set `Entity to attack [LE]` to the name of another entity.

### Parameters

* `Entity to attack [LE]` - entity to turn to and fire at.
* `Action to perform` - type of action to perform.
