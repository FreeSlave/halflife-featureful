---
bookHidden: true
bookToC: false
---

# trigger_compare

Compares two `LR` operands and fires appropriate targets or reports the result as a master state.

### Parameters

* `Base Value [LR]` - the left operand
* `Compare Value [LR]` - the right operand.
* `Comparison to pass` - the comparison type (equal, greater, less, etc.)
* `Fire On Comparison Pass`
* `Fire On Comparison Fail`
* `Fire On Equal To`
* `Fire On Not Equal To`
* `Fire On Less Than`
* `Fire On Greater Than`
* `Fire On Calc Fail` - fire if comparison is not possible (e.g. one of the operands is missing or invalid). Use it as a fallback when it makes sense.

### Master capability

The reported state depends on the result of chosen comparison. Checking the state doesn't fire the targets.
