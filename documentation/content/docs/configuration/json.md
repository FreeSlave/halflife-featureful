---
title: "JSON"
---

# {{% param "title" %}}

Various configuration files in Featureful SDK are in [JSON](https://en.wikipedia.org/wiki/JSON) format. Such configuration files use the **.json** extension, e.g. **features/materials.json**, **templates/entities.json**, etc. The format allows for structured hierarchical data representation which is not achievable with per-line configuration formats like **.cfg**.

## Editing

JSON files can be opened and modified in any text editor, but the text must be formatted in the special way. To avoid mistakes it's recommended to use a text editor with support for JSON validation, e.g. [Visual Studio Code](https://code.visualstudio.com/), [Sublime Text](https://www.sublimetext.com/) or use online JSON validators. [Notepad++](https://notepad-plus-plus.org/) has a 'JSON Viewer' plugin which is capable of displaying the object hierarchy and do the formatting for better readibility.

## Common mistakes

JSON might look similar to the format used by Valve in some configuration files (especially in Source), but there're important differences which shoulnd't be overlooked.

### Missing comma

Even though the document may look finely formatted with proper indentation, JSON still needs comma between property definitions.

{{% tabs %}}

{{% tab "Invalid" %}}

In this example the comma after the property is missing:

```json
{
    "fast_recruit_monsters": ["monster_human_grunt_ally"]
    "fast_recruit_range": 500
}
```

{{% /tab %}}

{{% tab "Valid" %}}

Notice the `,` after the `fast_recruit_monsters` property definition:

```json
{
    "fast_recruit_monsters": ["monster_human_grunt_ally"],
    "fast_recruit_range": 500
}
```

{{% /tab %}}

{{% /tabs %}}

### Missing quotes

Key names and string values must be taken in quotes.

{{% tabs %}}

{{% tab "Invalid" %}}
```json
{
    Player.DeathUnderwater: {
        "waves": ["player/h2odeath.wav"]
    }
}
```
{{% /tab %}}

{{% tab "Valid" %}}
```json
{
    "Player.DeathUnderwater": {
        "waves": ["player/h2odeath.wav"]
    }
}
```
{{% /tab %}}

{{% /tabs %}}

## Property types

Various .json configuration files in Featureful SDK share a handful of property type definitions.

### alpha

The brightness or opacity.

The value of this type must be an integer number between 0 and 255. Example: `"alpha": 200`

### color

The RGB color.

The value of this type accepts several forms:

1. An array of three integer numbers representing Red, Green and Blue components. Each number must be in 0-255 range. Example: `"color": [0, 200, 100]`
2. A string of three integer numbers separated by spaces. Each number must be in 0-255 range corresponding to Red, Green and Blue components. Example: `"color": "0 200 100"`.
3. A string representing the hexadecimal value starting with `#` (web notation). Example: `"color": "#00C864"`.
4. A string representing the hexadecimal value starting with `0x` (C++ notation). Example: `"color": "0x00C864"` .

### range

The pair of numbers. The value used in the game will be randomized between these two numbers.

The value of this type accepts several forms:

1. An array of two numbers where the first element is a minimum value and the second element is a maximum value. Example: `"life": [0.5, 1.6]`
2. A string that consists of two numbers separated by `,` symbol  where the first number is a minumum value and the second number is a maximum value. Example: `"life": "0.5,1.6"`.
3. Just a singular number. In this case the minimum and the maximum are equal. Example: `"life": 1.0`

### range_int

Same as `range` above, but the numbers must be integers. Examples: `"pitch": "95,100"`, `"pitch": [95, 100]`, `"pitch": 95`
