---
bookHidden: true
bookToC: false
---

# calc_eval_number

This entity calculates a result of binary operation on two `LR` parameters.

The `calc_eval_number` entity can be used as one of operands for another `calc_eval_number` entity, allowing to do complex calculations involving more than two operands.

### Parameters

Most parameters are self-descriptive and desribed in fgd.

* `First Operand [LR]` - first (left) operand. Can be another entity (including another `calc_eval_number`) or a literal value.
* `Second Operand [LR]` - second (right) operand. Can be another entity (including another `calc_eval_number`) or a literal value.
* `Third Operand [LR] (optional)`. After calculating the result of operation on the first 2 operands, make the same operation with the result and the third operand. This allows, for example, to add 3 values or find the maximum among 3 values without resorting to the use of another `calc_eval_number`.
* `Operation` - operation type.
* `Store result in` - when `calc_eval_number` is triggered and the result is evaluated without errors, store the numeric result in other entity like [game_number]({{< ref game_number >}}) or [game_counter]({{< ref game_counter >}}).
* `Trigger after operation` - when `calc_eval_number` is triggered and the result is evaluated without errors, trigger this target.
* `Trigger on calculation fail` - when `calc_eval_number` is triggered and the operation failed, trigger this target. The operation may fail if one of the operands couldn't be calculated due to the source entity being missing or due invalid operation like dividing by zero.

### Calc Ratio

Reports the result of operation. Requesting the result doesn't fire the targets.

{{% hint danger %}}
`calc_eval_number` must not calculate the value from itself directly on indirectly. Otherwise it will lead to the recursion.
{{% /hint %}}
