---
title: "Text font"
---

# {{% param "title" %}}

Half-Life uses two fonts in-game:

* The console font - used by chat messages and console message at the top of the screen (in the developer mode). In Featureful this font is also used by [subtitles]({{< ref subtitles >}}) and [journal]({{< ref journal >}}). This is usually a font that can be used with utf-8 texts, but it can't be rendered semitransparent.
* The credits font - used by **titles.txt** and [game_text]({{< ref game_text >}}) messages. This font supports only 8-bit encodings (ISO 8859-15 in Steam Half-Life) and it's rendered in additive mode (i.e. it can be semitransparent).

Fonts are defined in the **resource/TrackerScheme.res** file (you can copy it to your mod directory from **valve/** folder in anniversary version of Half-Life or from **public/** folder in steam_legacy version). The `"EngineFont"` entry allows configuring size for the console font depending on the client screen size. The `"Legacy_CreditsFont"` entry allows configuring the credits font.

Example:

```
		"EngineFont"
		{
			"1"
			{
				"name"		"Trebuchet"
				"tall"		"18"
				"weight"	"600"
				"yres"	"480 599"
				"dropshadow"	"0"
			}
			"2"
			{
				"name"		"Trebuchet"
				"tall"		"18"
				"weight"	"600"
				"yres"	"600 767"
				"dropshadow"	"0"
			}
			"3"
			{
				"name"		"Trebuchet"
				"tall"		"22"
				"weight"	"600"
				"yres"	"768 1023"
				"dropshadow"	"0"
			}
			"4"
			{
				"name"		"Trebuchet"
				"tall"		"30"
				"weight"	"600"
				"yres"	"1024 1199"
				"dropshadow"	"0"
			}
			"5"
			{
				"name"		"Trebuchet"
				"tall"		"36"
				"weight"	"600"
				"yres"	"1200 1439"
				"dropshadow"	"0"
			}
			"6"
			{
				"name"		"Trebuchet"
				"tall"		"48"
				"weight"	"600"
				"yres"	"1440 2559"
				"dropshadow"	"0"
			}
			"7"
			{
				"name"		"Trebuchet"
				"tall"		"72"
				"weight"	"600"
				"yres"	"2560 6000"
				"dropshadow"	"0"
			}
		}
```
