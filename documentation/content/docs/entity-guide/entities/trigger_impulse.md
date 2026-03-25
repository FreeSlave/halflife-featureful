---
bookHidden: true
bookToC: false
---

# trigger_impulse

[XashXT](https://documentation.help/XashXT/trigger_impulse.htm)

An out-of-the-box way to activate an object on a map when a key is pressed.

The steps to set up activation are as follows:

* The user binds the desired key to generation of an impulse in the range from 1 to 50, for example: `bind b "impulse 1"`. The mod developer can set a [default key]({{< ref "default-keys" >}}) for such action.
* When user presses the bound key all the `trigger_impulse` with a corresponding impulse get triggered, firing its targets as a result. The player who sent the impulse is passed as an activator.

### Parameters

* `Target` - the name of the target on the map.
* `Impulse` - the impulse number in range `[1..50]` denoting to which `impulse` command the entity will respond.
* `Master` - the master to check against before firing the target.
