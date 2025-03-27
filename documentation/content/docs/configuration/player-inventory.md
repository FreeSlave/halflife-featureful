---
title: "Player inventory"
---

# {{% param "title" %}}

The inventory is a part of player's data. Each inventory item has a name and the associated item count and can be rendered in HUD as a sprite.

Inventory items don't affect the gameplay on their own. It's just a tool for the level designer to introduce the custom player resource. Such examples include security cards and other quest items which usefullness is fully defined by the mapper.

![Inventory HUD](/images/player-inventory-hud.png)

{{% hint warning %}}
The current system is designed with only singleplayer in mind. There's no way for a player to drop an item, there's no way to check if the player who carries the item has left the game, etc.
{{% /hint %}}

## Defining inventory items

Inventory items are defined in the **templates/inventory.json**.

Example:

```json
{
    "items": {
        "longjump": {
            "max_count": 1
        },
        "battery_blue": {
            "max_count": 5,
            "pickup_template": {
                "own_visual": {
                    "model": "models/w_battery.mdl",
                    "renderfx": "Glow Shell",
                    "color": [0, 0, 255]
                },
                "soundscripts": {
                    "Inventory.Pickup": {
                        "waves": ["items/gunpickup4.wav"]
                    }
                }
            }
        },
        "battery_red": {
            "max_count": 5,
            "pickup_template": {
                "own_visual": {
                    "model": "models/w_battery.mdl",
                    "renderfx": "Glow Shell",
                    "color": [255, 0, 0]
                },
                "soundscripts": {
                    "Inventory.Pickup": {
                        "waves": ["items/gunpickup4.wav"]
                    }
                }
            }
        },
        "security_card": {
            "max_count": 4,
            "pickup_template": {
                "own_visual": {
                    "model": "models/w_security.mdl"
                },
                "soundscripts": {
                    "Inventory.Pickup": {
                        "waves": ["items/cardkey_pickup.wav"]
                    }
                }
            }
        }
    }
}
```

This example defines 4 inventory items. The `max_count` defines how many duplicates of the item player normally is allowed to obtain. By default the max count is unlimited. It's not required to define the inventory item in the **templates/inventory.json** but it's the only way to set the max count limit per item.

The `pickup_template` allows to define the [Entity template]({{< ref entity-templates >}}) that will be used by [item_pickup]({{< ref item_pickup >}}) with specified `Inventory item` type. It's useful if you want to ensure a uniform look and pickup sound for all items of the same type. This also allows to change the look and sound without editing maps in the mod.

In the example above inventory items `battery_blue` and `battery_red` are set to have the `models/w_battery.mdl` model and specific render effect, so mapper doesn't need to specify the model and the render effect for each [item_pickup]({{< ref item_pickup >}}) when they want to place these items on the map.

{{% hint warning %}}
Currently the maximum number of *different* items player can carry at the same time is 8.
{{% /hint %}}

## HUD

The way the inventory items are shown in HUD is defined in the **sprites/hud_inventory.json**. This allows to set the associated HUD sprite, its color and (to some extent) its position on the screen.

Example:

```json
{
    "default_sprite_alpha": 225,
    "text_alpha": 200,
    "items": {
        "battery_blue": {
            "sprite": "item_battery",
            "position": "bottom",
            "color": "0 100 255"
        },
        "battery_red": {
            "sprite": "item_battery",
            "position": "bottom",
            "color": "255 0 0"
        },
        "longjump": {
            "sprite": "item_longjump",
            "position": "topleft",
            "show_in_history": false
        },
        "security_card": {
            "sprite": "item_security",
            "position": "topright"
        }
    }
}
```

This example defines the default sprite alpha (opacity), the text alpha (opacity for item counts) render parameters for 4 items.

* `sprite` property should refer to the name from the **sprites/hud.txt**.
* `color` defines sprite [color]({{< ref "JSON/#color" >}}) in HUD. When omitted it will use the client's main HUD color.
* `position` can be `"topleft"`, `"topright"`, `"bottom"` and `"hide"`. The `"topleft"` is the default position. The `"topright"` means that the item icon will be displayed on the right side under the flashlight (or under the move mode if it's enabled). The `"bottom"` means that the item icon will be displayed on the bottom center.
* `show_in_history` allows to configure whether the icon should appear in the pickup history (along with picked up items, ammo and weapons). This is true by default.

If the map utilizes some inventory item that is not defined in the **sprites/hud_inventory.json** the game will try to find a HUD sprite by the item name and show it with default parameters (e.g. the item named `item_helmet` will use `item_helmet` as a HUD sprite and will render with a player's HUD color).

If the player has several duplicates of the same item it will be shown as a number next to the item icon.

## Controlling the player's inventory

The [player_inventory]({{< ref player_inventory >}}) allows to add or remove inventory items for the player.

The [item_pickup]({{< ref item_pickup >}}) allows to put an item in the world. Such item can be picked up by a player like others (e.g. item_battery) and it will give the inventory item and disappear.

The [player_hasinventory]({{< ref player_hasinventory >}}) allows to check if the player has at least one item of the specified type.

The [player_calc_ratio]({{< ref player_calc_ratio >}}) with type `Inventory item count` allows to track the number of item duplicates the player carries (and use it in `trigger_compare`).

## Developer commands

When **sv_cheats** is enabled you can use following commands to get or remove inventory items.

### give_inventory

Syntax: `give_inventory <item_name> <number of items>` where

* `<item_name>` is the name of the inventory item to give.
* `<number of items>` is a number of duplicates to give. When omitted it gives 1 item by default.

Example: `give_inventory battery_red 3`

If the max count limit is imposed on the inventory item, the command won't give more items than that.

### remove_inventory

Syntax: `remove_inventory <item_name> <number of items>` where

* `<item_name>` is the name of the inventory item to remove.
* `<number of items>` is a number of items to remove. When omitted it removes 1 item by default.

Example: `remove_inventory battery_red 2`

## FAQ

**Q:** Why there're two files describing the inventory items?

**A:** The **templates/inventory.json** is read *server-side* and includes game logic parameters. The **sprites/hud_inventory.json** is read *client-side* and defines only how the inventory item is rendered in the client's HUD. If you were making a multiplayer mod the user could configure what the inventory would look like in their HUD.
