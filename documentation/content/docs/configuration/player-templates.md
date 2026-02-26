---
title: "Player templates"
---

# {{% param "title" %}}

Player templates allow to configure various player's traits via **templates/player.json** file. The template can be applied to the player during the game by [player_template]({{< ref player_template >}}) or assigned to the spawning player via [map config]({{< ref map-config >}}).

Player templates might be useful if you want to simulate playing different characters through the campaign or to make a change to the main character (e.g. show custom weapon viewmodels after the character put on different clothes).

## Format of templates/player.json

The root document is an object where each key is a template name and each value is a player template. Special template `"default"` is applied automatically if no other template is set on the player.

Example:

```json
{
    "default": {
        "maxspeed": 300,
        "prohibited_items": ["item_armorvest", "item_helmet"]
    },
    "barney": {
        "hud_color": "0x5F5FFF",
        "maxspeed": 260,
        "suit_sentences": false,
        "prohibited_items": ["item_battery", "func_recharge", "item_recharge"],
        "weapons": {
            "weapon_crowbar": {
                "viewmodel": "models/bshift/v_crowbar.mdl"
            },
            "weapon_9mmhandgun": {
                "viewmodel": "models/bshift/v_9mmhandgun.mdl"
            },
            "weapon_9mmAR": {
                "viewmodel": "models/bshift/v_9mmar.mdl"
            },
            "weapon_shotgun": {
                "viewmodel": "models/bshift/v_shotgun.mdl"
            },
            "weapon_satchel": {
                "viewmodel": "models/bshift/v_satchel.mdl",
                "viewmodel_detonator": "models/bshift/v_satchel_radio.mdl"
            }
        },
        "ent_template": {
            "own_visual": {
                "model": "models/player/barney/barney.mdl"
            }
        }
    },
    "alien": {
        "hud_color": "0x00AF00",
        "hud_color_nosuit": "0x00AF00",
        "hud_draw_nosuit": true,
        "maxspeed": 260,
        "play_hev_dead": "never",
        "suit_sentences": false,
        "allowed_items": ["weapon_hornetgun", "weapon_snark", "weapon_grapple", "weapon_shockrifle", "weapon_sporelauncher", "ammo_spore"],
        "ent_template": {
            "blood": "yellow",
            "soundscripts": {
                "Player.NVGOn": {
                    "waves": ["aslave/slv_alert1.wav"]
                },
                "Player.NVGOff": {
                    "waves": ["aslave/slv_alert3.wav"]
                },
                "Player.Death": {
                    "waves": ["aslave/slv_die1.wav", "aslave/slv_die2.wav"]
                },
                "Player.FallPain": {
                    "waves": ["aslave/slv_pain1.wav", "aslave/slv_pain2.wav"]
                },
                "Player.Jump": {
                    "waves": ["aslave/slv_word8.wav"]
                }
            }
        }
    }
}
```

Each player template can have the following properties:

### allowed_items

An array of entity classnames that the player is able to pick up (items, ammo and weapons). These also can be classnames of wall chargers.

Any item that is not in the list can't be picked up by a player.

### ent_template

An [entity template]({{< ref "entity-templates" >}}) for the player. Note that only a portion of entity template parameters is applicable to the player:

* `"blood"` - allows to set a blood color.
* `"own_visual"` - allows to set the player model. Makes sense only if you want to show the player on camera. Note that in developer mode the client's multiplayer model is always used instead.
* `"soundscripts"` - allows to redefine [player soundscripts]({{< ref "player/#soundscripts" >}}).

### hud_color

Custom HUD [color]({{< ref "json/#color" >}}) for the player (when they have a suit). Non-zero value forces the color even if `hud_color.configurable` is enabled in **features/featureful_client.cfg**.

### hud_color_critical

Custom HUD [color]({{< ref "json/#color" >}}) for the player when they're at critical health.

If not set, the `hud_color_critical` from **features/featureful_client.cfg** is used.

### hud_color_nosuit

Custom HUD [color]({{< ref "json/#color" >}}) for the player without a suit.

If not set, the `hud_color_nosuit` from **features/featureful_client.cfg** is used.

### hud_draw_nosuit

A boolean - whether the HUD should be drawn without a suit.

If not set, the behavior depends on `hud_draw_nosuit` option in **features/featureful_client.cfg**.

### maxspeed

Custom [base maximum speed]({{< ref "player/#maximum-speed" >}}) for the player.

### nosuit_allow_healthcharger

A boolean - whether the player is allowed to use [health charger]({{< ref func_healthcharger >}}) without a suit.

If not set, the behavior depends on `nosuit_allow_healthcharger` option in **features/featureful_server.cfg**.

### play_hev_dead

Rules for playing the `HEV_DEAD` sentence on player's death. Possible values:

* `"always"` - play `HEV_DEAD` even if player doesn't have a suit. This is a default behavior in Half-Life.
* `"suit"` - play `HEV_DEAD` only if player has a suit.
* `"never"` - never play `HEV_DEAD`.

If not set, the behavior depends on `hev_dead_requires_suit` option in **features/featureful_server.cfg**.

### prohibited_items

An array of entity classnames that the player shouldn't be able to pick up (items, ammo and weapons). These also can be classnames of wall chargers to prohibit the interaction with them.

{{% hint info %}}
The player still preserves prohibited weapons if player possessed them before switching to a new template.
{{% /hint %}}

{{% hint warning %}}
This list is ignored if [allowed_items](#allowed_items) is specified.
{{% /hint %}}

### suit_sentences

A boolean - whether HEV sentences should be playing for this player template.

If not set, the behavior dependson `suit_sentences` option in **features/featureful_server.cfg**.

### weapons

An object describing custom weapon traits for this player template. The keys are the weapon names. The values are objects that can have following properties:

#### viewmodel

Path to the custom viewmodel to show for this player template.

#### viewmodel_detonator

Path to the custom viewmodel to show for this player template when weapon is in the detonator state (currently applied only to satchels).
