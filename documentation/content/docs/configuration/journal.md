---
title: "Journal"
---

# {{% param "title" %}}

Featureful SDK implements a simple journal window that can be used to show objectives, hints and other texts to the player. Journal consists of *sections*, and each section may have the *header* and the message text. The journal is designed to support the localization via the [translatable messages]({{< ref "text-messages/#translatable-messages" >}}).

![Journal window](/images/journal-window.png)

Journal is currently for singleplayer only, and it can be opened by the same key that is used to show scoreboard in multiplayer.

Getting a new journal record (see [Usage](#usage)) can show the notification message on the player's screen and optionally play a notification sound. Multiple notifications may render at the same time: they will appear in the delivery order.

## Configuration

The journal look and sections are configured in the **journal.json** file (in the mod main directory).

Example:

```json
{
    "sections": {
        "primary": {
            "header": "PRIMARY_OBJ_HEADER",
            "notification": "PRIMARY_OBJECTIVE_UPDATED1",
            "notification_right": "PRIMARY_OBJECTIVE_UPDATED2",
            "sound": true
        },
        "secondary": {
            "header": "SECONDARY_OBJ_HEADER",
            "notification": "SECONDARY_OBJECTIVE_UPDATED1",
            "notification_right": "SECONDARY_OBJECTIVE_UPDATED2",
            "sound": "items/r_item1.wav"
        },
        "thoughts": {
            "header": "THOUGHTS_HEADER",
            "notification": "NEW_THOUGHT"
        },
        "inventory": {
            "header": "INVENTORY_HEADER",
            "show_inventory": true
        }
    },
    "geometry": {
        "width": 0.75,
        "height": 0.75,
        "padding_horizontal": 0.1,
        "padding_vertical": 0.1
    },
    "notification_position": {
        "x": 0.1,
        "y": 0.2
    },
    "render": {
        "text_color": [255, 0, 0],
        "notification_text_color": [0, 255, 0],
        "background_color": [0, 0, 0],
        "background_alpha": 160,
        "background_additive": true,
        "frame_color": [255, 160, 0],
        "frame_alpha": 0,
        "frame_additive": false
    }
}
```

Properties:

### sections

An object with properties defining the possible sections of the journal. The name of each property is also the section ID used by the [game_journal]({{< ref game_journal >}}) entity. The sections will be shown in the same order as they're defined in **journal.json**.

Each section can have the following properties:

* `"header"` - the [message ID]({{< ref "text-messages/#translatable-messages" >}}) of the section's header. If not defined, no header will be shown. If message with such ID is not found, the ID itself is shown.
* `"notification"` - the message ID of the notification text. If not defined, no nofitication is shown.
* `"notification_right"` - the message ID of the second part of notification text. If both `"notification"` and `"notification_right"` are defined, the resulting text is a combination of the first notification text, the name of the keybind to open the journal and the second notification text.
* `"sound"` - a notification sound. If it's a string, it should be a path to the sound in the **sound/** directory of the mod. If it's `true`, the default notification sound will be used (**misc/talk.wav** - same sound that is used in chat on receiving new message).
* `"show_inventory"` - whether to show the [player inventory]({{< ref player-inventory >}}) items. The items are shown after header and text (if any).
    - Usually you want only one section with `"show_inventory"` set to `true`.
    - If it's the last section, the section will gravitate to the bottom of the journal window for better separation of the inventory section from other sections.
    - While the journal window is active the inventory items won't be shown at other parts of the screen to avoid duplication.
* `"always_show"` - show the section header even if there's no message set (and if player doesn't have any inventory items for the section with `"show_inventory"`).

### geometry

An optional property that lets you to configure geometry settings of the journal window.

* `"width"` - the journal window width, as fraction of the screen width. Accepts values in range (0, 1]. The default value is `0.78`.
* `"height"` - the journal window height, as fraction of the screen height. Accepts values in range (0, 1]. The default value is `0.73`.
* `"padding_horizontal"` - left and right padding before and after text inside the journal window, as fraction of the journal width. Accepts values in range [0, 0.5). The default value is `0.0714`.
* `"padding_vertical"` - bottom and top padding before and after text inside the journal window, as fraction of the journal height. Accepts values in range [0, 0.5). The default value is `0.077`.

### notification_position

An optional property that lets you to configure the position of the notification area on the screen.

* `"x"` - the X coordinate of the text on the screen, as fraction of the screen width. Accepts values in range [0, 1). The default value is `0.556`.
* `"y"` - the Y coordinate of the text on the screen, as fraction of the screen height. Accepts values in range [0, 1). The default value is `0.2`.

### render

An optional property that lets you to configure the colors for the journal window.

* `"text_color"` - the [color]({{< ref "JSON/#color" >}}) used to draw both header and record texts. By default the color based on HUD color is used.
* `"notification_text_color"` - the color used to draw notification messages. By default the color based on HUD color is used.
* `"background_color"` - the background color for the journal window. The default vlaue is  `[0, 0, 0]` (black color).
* `"background_alpha"` - the background [alpha]({{< ref "JSON/#alpha" >}}) value. The default value is 160.
* `"background_additive"` a boolean. Whether the background should be rendered in additive mode to allow semitransparency. This is true by default.
* `"frame_color"` - the window frame color. By default the color based on HUD color is used.
* `"frame_alpha"` - the frame alpha value.
* `"frame_additive"` - a boolean. Whether the frame should be rendered in additive mode to allow semitransparency. This is false by default.

## Usage

For the **journal.json** example above you'll need to define [text messages]({{< ref "text-messages/#translatable-messages" >}}) for headers and journal records. Example of **messages.en.json**:

```json
{
    "PRIMARY_OBJ_HEADER": "PRIMARY OBJECTIVE:",
    "PRIMARY_OBJECTIVE_UPDATED1": "New primary objective! Press [",
    "PRIMARY_OBJECTIVE_UPDATED2": "] to read",
    "SECONDARY_OBJ_HEADER": "SECONDARY OBJECTIVE:",
    "SECONDARY_OBJECTIVE_UPDATED1": "New secondary objective! Press [",
    "SECONDARY_OBJECTIVE_UPDATED2": "] to read",
    "THOUGHTS_HEADER": "THOUGHTS:",
    "INVENTORY_HEADER": "INVENTORY:",
    "NEW_THOUGHT": "Got a new thought!",
    
    "PRIMARY_OBJECTIVE_EXAMPLE": "Run. Shoot. Think.",
    "PRIMARY_OBJECTIVE_EXAMPLE2": "Save the world from alien invasion.",
    "SECONDARY_OBJECTIVE_EXAMPLE": "Find out who ate all donuts.",
    "SECONDARY_OBJECTIVE_EXAMPLE2": "Catch Barney and demand beers he owed you.",
    "THOUGHT_EXAMPLE": "You are Gordon Freeman, a young research associate in the Anomalous Materials Laboratory. You have limited security clearance and no real idea of just how dangerous your job has become, until the morning you are sent alone into the Test Chamber to analyze a strange crystalline specimen. A routine analysis, they tell you. Until something goes wrong."
}
```

To use the journal in the game you put the [game_journal]({{< ref game_journal >}}) somewhere on the map, assign the targetname, the journal section and the journal record. Call it by trigger to update the message in the selected section.

To update the primary objective with the example message you would set:

* `Journal Section` = `primary` (the section is defined in the [previous](#configuration)) example.
* `Journal Record` = `PRIMARY_OBJECTIVE_EXAMPLE` (the id of a translatable objective message)

This also will show the message `New primary objective! Press [TAB] to read` on the player's screen (in case the journal/scoreboard is bound to TAB key), using the `PRIMARY_OBJECTIVE_UPDATED1` and `PRIMARY_OBJECTIVE_UPDATED2` messages as configured in **journal.json**.

To update the thoughts you would set:

* `Journal Section` = `thoughts`
* `Journal Record` = `THOUGHT_EXAMPLE`

The inventory section gets updated automatically as player receives or loses inventory items.

{{% hint info %}}
If there's no record for the specified message ID the message ID will be displayed instead of text.
{{% /hint %}}

To reiterate:

1. Write [configuration](#configuration) in **journal.json**.
2. Add the message entries to **messages.en.json** (and **messages.json** for localizations).
3. Put the [game_journal]({{< ref game_journal >}}) entity on the map. Set section to one of sections defined in **journal.json**. Set record to one of message ids from **messages.en.json**. Give the entity a targetname and trigger it to show a notification and add a record in the player's journal.
