---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) calc_velocity_path

Calculates a vector between start position and destination. Can be used as offset in `LV` parameters or as a face direction in [motion_manager]({{< ref motion_manager >}}).

{{% hint info%}}
Set `Length Calculation` to `Normalise` if you want a normalized vector (of length 1).
{{% /hint %}}

## Example: make an entity to constantly face in direction of another entity

1. Create entity (e.g. `item_generic`) that needs to turn to another entity. Give it a name.
2. Create another entity to face. Give it a name.
3. Create `calc_velocity_path`. Give it a name. Set the `Start position` to the name of the first entity. Set the `Destination` to the name of another entity.
4. Create `motion_manager`. Set the `Target to affect` to the name of the first entity. Set the `Facing` to the name of your `calc_velocity_path`.
