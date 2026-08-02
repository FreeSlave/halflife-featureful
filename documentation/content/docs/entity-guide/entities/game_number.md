---
bookHidden: true
bookToC: false
---

# game_number

The purpose of this entity is to hold the previously calculated number, e.g. from [calc_eval_number]({{< ref calc_eval_number >}}). This is similar to [game_counter]({{< ref game_counter >}}) minus the limit value capabilities. Also unlike `game_counter` it can store non-integer number.

### Calc Ratio

Reports the stored value.

### Reaction to use-type

Increases the number by 1 for `Toggle` and `On`. Decreases the number by 1 for `Off`. Sets the number for `Set` (e.g. if triggered by `calc_eval_number`).
