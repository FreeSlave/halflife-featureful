---
title: "Save titles"
---

# {{% param "title" %}}

The *Load game* menu dialog is capable of showing the chapter name for the save in the list (e.g. ANOMALOUS MATERIALS). However, Half-Life has a hardcoded list that maps level names to chapters names. Featureful SDK allows to provide your own mapping. To achieve that follow these steps:

1. Create the file named `save_titles.txt` in the main directory of your mod.
2. Write down the level name prefixes and corresponding chapter names.

Example:

```
FEATUREFUL "Featureful Demo"
FLYERS "Flyers Demo"
```

Thus all saves made on maps which names start with "featureful" prefix (e.g. *featureful_demo*) will have a "Featureful Demo" title in the *Load game* menu. All saves made on maps which names start with "flyers" will have a "Flyers Demo" title.

The search is case-insensitive.

Note that the order matters - the first matched prefix will be used. Consider the following case:

```
CROSS "Some crossing"
CROSSFIRE "Crossfire deathmatch map"
```

As the `CROSS` comes first in the list and matches the name `crossfire`, the save file made on this level will use the title defined by CROSS prefix.

## Localization

Half-Life supports save title localization via files **resource/valve_english.txt**, **resource/valve_russian.txt**, etc (depending on the chosen language). There you can define the identifier and the corresponding text, e.g.

```
"CHAPTER_IDENTIFIER"	"DISPLAYED CHAPTER NAME"
```

Then, in **save_titles.txt** you can map the level name prefix to this title by prepending the `#` symbol:

```
CHAPTER1 "#CHAPTER_IDENTIFIER"
```

## Fallback

If no title is defined for the map, the value from `Map Description / Title` of the map properties will be used as a title. If it's empty, the map name is used.
