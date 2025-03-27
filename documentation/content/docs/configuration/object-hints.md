---
title: "Object hints"
---

# {{% param "title" %}}

Object hints are visual marks attached to entities in order to better convey the idea that the object might be of interest for a player. One example is interaction icon from PS2 version of Half-Life. Or object frames in Deus Ex games.

Featureful SDK provides ways to configure what objects hints should look like and to what entities they should be applied automatically. E.g. the button's hint might look differently depending on whether it's currently locked by master or not.

By default objects hints are not present. The modmaker should create the **templates/objecthint.json** file and configure hints there.

## Example

```json
{
    "visuals": {
        "base": {
            "sprite": "sprites/hud_brackets.spr"
        },
        "interactable": {
            "color": [255, 200, 50],
            "sprite": "sprites/hud_brackets.spr"
        },
        "charged": {
            "color": [0, 255, 0],
            "sprite": "sprites/hud_brackets.spr"
        },
        "depleted": {
            "color": [255, 0, 0],
            "sprite": "sprites/hud_brackets.spr"
        },
        "lock": {
            "color": [255, 50, 0],
            "sprite": "sprites/hud_brackets.spr"
        }
    },
    "templates": {
        "button": {
            "interaction": {
                "default": "interactable",
                "unusable": null,
                "locked": "lock"
            }
        },
        "charger": {
            "interaction": {
                "default": "charged",
                "unusable": "depleted"
            }
        },
        "eyescanner": {
            "interaction": {
                "default": "interactable",
                "unusable": null
            }
        },
        "pickup": {
            "scan": {
                "default": "base",
                "unusable": null
            },
            "interaction": {
                "default": "interactable",
                "unusable": null
            },
            "distance": 256
        },
        "hidden": {}
    },
    "entity_mapping": {
        "func_button": "button",
        "func_rot_button": "button",
        "momentary_rot_button": "button",
        "func_healthcharger": "charger",
        "func_recharge": "charger",
        "item_healthcharger": "charger",
        "item_recharge": "charger",
        "item_eyescanner": "eyescanner",
        "item_pickup": "pickup"
    },
    "pickup_mapping": {}
}
```

## Format of objecthint.json

### Hint visuals

First, you define `"visuals"` object consisting of named entries that describe what the hints should look like. Currently only sprite hints are supported - so your mod needs its own sprite (or several sprites) to use them as hints. Each hint visual can have following properties:

* `"sprite"` - the path to the sprite.
* `"color"` - the sprite color. If missing or all zeroes, the client's HUD color will be used.
* `"scale"` - an optional parameter to set the scale factor for the sprite. Note that the sprite size depends on the dimensions of the object the sprite is attached to. If you're not satisfied with the results, you can use adjust them with this parameter.

The sprite should be suitable to use in additive mode, i.e. black parts of the sprite will be transparent in game, and other colors will be mixed with the specified color (like HUD sprites).

### Hint templates

Then, you define `"templates"` object consisting of named entries that describe which hints should show on the entities that refer to the specific template. Template can have following properties:

* `"interaction"` - what visual hints to show when the object is at player's Use proximity (i.e. if player presses their "+use" button, it will go the highlighted object).
* `"scan"` - what visual hints to show when the object is out of player's Use proximity. Or if `"interaction"` is not defined.
* `"distance"` - distance between player and another entity at which visual hints defined in the `"scan"` should be shown.
* `"vertical_offset"` - offset in units between the center of the object and the location where the visual hint should be shown.

Both `"interaction"` and `"scan"` has 3 properties to refer to visual hints that should be shown depending on the object's state. The visuals are referred to by their respective names in `"visuals"` object.

* `"default"` - this hint will be used if others two are not defined or the conditions for them are not met.
* `"locked"` - this hint will be used if the entity is locked by master. If set to `null` or empty string, the hint won't show at all when the entity is locked.
* `"unusable"` - this hint will be used if the object is "unusable" for the player. If set to `null` or empty string, the hint won't show at all when the entity is "unusable". The exact meaning depends on the entity. E.g. wall chargers are "unusable" when they're depleted. Eye scanners are unusable when they won't grant access to the player.

### Mapping between entity classnames and hint templates

`"entity_mapping"` allows to map entity classnames to the hint templates these entities should use by default. E.g. if there's a "func_button" in the mapping, the specified hint template will be used for all buttons. The templates are referred to by their respective names in `"templates"` object.

### Mapping between inventory entity name and hint templates

`"pickup_mapping"` allows to map the inventory item name to the hint template. This is similar to `"entity_mapping"` but used for `item_pickup` entities only. This way you can easily configure different hints for different inventory items.

## Setting hint template for the specific entity

Entities have the "Object Hint Template" (*objecthint*) property that allows to set a custom hint template for the entity. For example, you have "func_button" in the `"entity_mapping"` referring to some template. But for the specific button on the map you want to show a different kind of hint. This is where you can set the name of another hint template in the entity properties. Note that the template should be defined in the **templates/objecthint.json** file like others.
