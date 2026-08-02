---
bookHidden: true
bookToC: false
---

# calc_vecfromnums

Calculates the vector from three `LR` parameters. Can be used in both `LV` and `LP` parameters of other entities.

### Parameters

* `Base Vector [LV], [LP], [LE]` - the vector to use as a base. If omitted the base vector is `0 0 0`.
* `Base Vector is...`
* * `Velocity [LV]` - calculate velocity as a base vector.
* * `Position [LP]` - calculate position as a base vector.
* * `Color [LE]` - get rendercolor as a base vector.
* `X value [LR]` - use as X component. If omitted the one from Base Vector will be used.
* `Y value [LR]` - use as Y component. If omitted the one from Base Vector will be used.
* `Z value [LR]` - use as Z component. If omitted the one from Base Vector will be used.
