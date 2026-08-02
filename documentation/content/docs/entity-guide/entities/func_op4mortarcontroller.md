---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_op4mortarcontroller

A button that allows players to control vertical or horizontal axis of [op4mortar]({{< ref op4mortar >}}). Usually you want to put two instances of `func_op4mortarcontroller` for each `op4mortar` where one controls the vertical alignment, and another controls the horizontal alignment of the gun.

### Changes

* In Opposing Force there's a 0.5 seconds delay before the player can change the rotation direction of the `op4mortar`. In Featureful SDK it's enough to release the 'use' button and press it again to change the direction. Such controls should be easier to understand.
