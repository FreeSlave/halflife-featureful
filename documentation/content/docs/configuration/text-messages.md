---
title: "Text messages"
---

# {{% param "title" %}}

Featureful SDK supports the same means to show text as original Half-Life ([env_message]({{< ref env_message >}}) and [game_text]({{< ref game_text >}})) along with some new methods and improvements.

## Titles

In vanilla Half-Life on Steam **titles.txt** expects to be either in [ASCII](https://en.wikipedia.org/wiki/ASCII) or in [ISO 8859-15](https://en.wikipedia.org/wiki/ISO/IEC_8859-15) character encodings which limit the texts to English and other Western European languages. This is due to the text being rendered with a font that contains characters only from ISO 8859-15.

{{% hint info %}}
In WON versions of Half-Life the fonts for other 8-bit encodings could be provided via custom **gfx.wad**. This method doesn't work anymore in Steam version.
{{% /hint %}}

In Featureful SDK **titles.txt** can contain UTF-8 text. This, however, makes the text to render with a console engine font which can't be rendered in additive mode. Non-UTF texts are still rendered as usual.

While **titles.txt** is a decent way to handle texts in Half-Life, it suffers from some flaws like mixing of presentation and data (render properties and the text itself are defined in the same file which makes translations harder to sync with the original if it changes the presentation) and being "too close" to the engine.

### Autosave

In Half-Life the game automatically shows `GAMESAVED` title when the player saves the game manually or via quick-save.

In Featureful the game also shows `AUTOSAVE` title when player touches the [trigger_autosave]({{< ref trigger_autosave >}}) or activates the [game_autosave]({{< ref game_autosave >}}). If `AUTOSAVE` title is not defined in the **titles.txt** it won't show anything.

## Translatable messages

Featureful SDK implements an additional way to handle texts, used by some subsystems (by [journal]({{< ref journal >}}) and [game_messagebox]({{< ref game_messagebox >}})).

**messages.en.json** (located in the mod main directory) defines the mapping between message IDs and messages texts. Example:

```json
{
    "HELLO": "Hello, world!",
    "QUESTION": "What are you on about?",
    "ANNOUNCEMENT": "Attention. This announcement system now under military command.",
    "STILL_ALIVE": {
        "file": "texts/still_alive.txt"
    }
}
```

It also allows specifying the file path via the object with `"file"` parameter (the `"STILL_ALIVE"` message example) - the file contents will be read and used as the text message. There's no restriction on what subdirectory should be used to store such files - it's up to the mod developer.

If the message text is written right in the .json file, some characters must be [escaped](https://en.wikipedia.org/wiki/Escape_character#JavaScript). E.g. the backslash should be written as `\\` instead of `\`. If the text is loaded from the file, no escape characters needed.

The ".en" part in **messages.en.json** is because the default text is expected to be in English. The addon directories for localizations (like **mod_german**, **mod_russian**, etc.) should contain **messages.json** file. If some translations are missing, the default one from **messages.en.json** will be used.

Let's say you have a Russian localization **messages.json** file:

```json
{
    "HELLO": "Привет, мир!",
    "QUESTION": "О чём это ты?"
}
```

As you can see, the `"ANNOUNCEMENT"` message ID is missing here, so the English version will be used in the absence. The `"STILL_ALIVE"` will still refer to the specified file (the translated version can be shipped in the file by the same relative path in the addon directory).

This allows to add new message IDs without worrying about translations not having the new message, e.g. if you want to update the mod with new messages and third-party users already published their localizations - the users of such localizations will see new texts in English and it's better than nothing.

{{% hint info %}}
You can use **messages.json** for English texts as well, but then you lose the benefits the method above provides.
{{% /hint %}}

{{% hint info %}}
The message IDs are case-insensitive. I.e. `hello` and `HELLO` would refer to the same message.
{{% /hint %}}

{{% hint info %}}
It's recommended not to create your own message IDs that start with `__` as this convention is used by [predefined message](#predefined-messages).
{{% /hint %}}

The `get_message` console command is used to retrieve the message text for the provided id.

### Predefined messages

Some message IDs are predefined. They start with `__`. The default messages are provided for them and they can be translated as well.

| ID | Default text | Description |
| ---- | ---- | ---- |
| `__SHOWINFO_HEALTH` | Health | Used when displaying the monster info or player info in co-op |
| `__SHOWINFO_ARMOR` | Armor | Used when displaying the player info in co-op |
| `__SHOWINFO_STRENGTH` | Strength | Used when displaying the breakable's info |

Example of Russian localization:

```json
{
    "__SHOWINFO_HEALTH": "Здоровье",
    "__SHOWINFO_ARMOR": "Броня",
    "__SHOWINFO_STRENGTH": "Прочность",

    //...other messages
}
```

## Display names

See [Display names]({{< ref displaynames >}})

## Subtitles

See [Subtitles]({{< ref subtitles >}})
