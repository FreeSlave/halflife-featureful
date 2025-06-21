---
title: "Journal"
---

# {{% param "title" %}}

Featureful SDK implements a simple journal window that can be used to show objectives, hints and other texts to the player. Journal consists of sections, and each section may have the header and the message text.

![Journal window](/images/journal-window.png)

Journal is currently for singleplayer only, and it can be opened by the same key that is used to show scoreboard in multiplayer.

Getting a new journal record can show the notification message on the player's screen and optionally play a notification sound.

## Configuration

The journal sections are configured in the **journal.json** file (in the mod main directory).

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

* `"width"` - the journal window width, as fraction of the screen width. Accepts values in range (0, 1]. The default value is 7 / 9 (~ 0.78).
* `"height"` - the journal window height, as fraction of the screen height. Accepts values in range (0, 1]. The default value is 11 / 15 (~ 0.73).
* `"padding_horizontal"` - left and right padding before and after text inside the journal window, as fraction of the journal width. Accepts values in range [0, 0.5). The default value is 1 / 14 (~ 0.071).
* `"padding_vertical"` - bottom and top padding before and after text inside the journal window, as fraction of the journal height. Accepts values in range [0, 0.5). The default value is 1 / 13 (~ 0.077).

### notification_position

An optional property that lets you to configure the position of the notification messages on the screen.

* `"x"` - the X coordinate of the text on the screen, as fraction of the screen width. Accepts values in range [0, 1). The default value is 1 / 18.
* `"y"` - the Y coordinate of the text on the screen, as fraction of the screen height. Accepts values in range [0, 1). The default value is 0.2.

## Usage

For the **journal.json** example above you'll need to define [text messages]({{< ref "text-messages/#translatable-messages" >}}) for headers and journal records. Example:

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
    "INVENTORY_DESCRIPTION": "This is inventory",
    
    "PRIMARY_OBJECTIVE_EXAMPLE": "Run. Shoot. Think.",
    "PRIMARY_OBJECTIVE_EXAMPLE2": "Save the world from alien invasion.",
    "SECONDARY_OBJECTIVE_EXAMPLE": "Find out who ate all donuts.",
    "SECONDARY_OBJECTIVE_EXAMPLE2": "Catch Barney and demand beers he owed you.",
    "THOUGHT_EXAMPLE": "You are Gordon Freeman, a young research associate in the Anomalous Materials Laboratory. You have limited security clearance and no real idea of just how dangerous your job has become, until the morning you are sent alone into the Test Chamber to analyze a strange crystalline specimen. A routine analysis, they tell you. Until something goes wrong."
}
```

To use the journal in the game you put the [game_journal]({{< ref game_journal >}}) somewhere on the map, assign the targetname, the journal section and the journal record. Call it by trigger to update the message in the selected section.

To update the primary objective with the example message you would set:

* `Journal Section` = `primary`
* `Journal Record` = `PRIMARY_OBJECTIVE_EXAMPLE`

This also will show the message `New primary objective! Press [TAB] to read` on the player's screen (in case the journal/scoreboard is bound to TAB key).

To update the thoughts you would set:

* `Journal Section` = `thoughts`
* `Journal Record` = `THOUGHT_EXAMPLE`

The inventory section gets updated automatically as player receives or loses inventory items.

{{% hint info %}}
If there's no record for the specified message ID the message ID will be displayed instead of text.
{{% /hint %}}
