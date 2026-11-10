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

{{% hint warning %}}
Note that the order matters - the first matched prefix will be used. Consider the following case:

```
CHAPTER1 "Chapter 1"
CHAPTER10 "Chapter 10"
```

As the `CHAPTER1` comes first in the list and the prefix matches the map name `chapter10`, the save file made on the level `chapter10` will use the title defined by `CHAPTER1` prefix.

As a general rule - the longer prefixes should be listed first, followed by the shorter prefixes.
{{% /hint %}}

## Localization

Half-Life supports save title localization via **resource/mod_english.txt**, **resource/mod_german.txt**, **resource/mod_russian.txt**, etc (depending on the chosen language) where **mod** is the name of your mod directory. See **valve/resource/valve_english.txt** and **gearbox/resource/gearbox_english.txt** for examples.

In such files you can define the *identifier* and the corresponding localized text, e.g.

```
"lang"
{
"Language" "English"
"Tokens"
{
"CHAPTER_IDENTIFIER"	"DISPLAYED CHAPTER NAME"
}
}
```

{{% hint warning %}}
The text localization **resource/** files must be in `UTF-16 LE` encoding.

But the **save_titles.txt** is expected to be in `ASCII` or `UTF-8`.
{{% /hint %}}

In **save_titles.txt** then you should map the level name prefix to this title by prepending the `#` symbol to the *identifier*:

```
CHAPTER1 "#CHAPTER_IDENTIFIER"
```

{{% hint info %}}
Since localizations are usually distributed in the separate directories (like **mod_german**, etc.) you may also just put **mod_english.txt** into the localization directory - this will override the one in the main directory (**mod**).
{{% /hint %}}

## Fallback

If no title is defined for the map, the value from `Map Description / Title` of the map properties will be used as a title. If it's empty, the map name is used.
