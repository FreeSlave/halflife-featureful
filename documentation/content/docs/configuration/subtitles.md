---
title: "Subtitles"
---

# {{% param "title" %}}

Featureful SDL has a subtitles (closed captions) system allowing for showing texts as the scripted voice line is played.

Captions are not showing for the regular sentences spoken by monsters (e.g. idle voicelines) to avoid littering the screen with too much text.

The client can set `cl_subtitles` to 0 to disable showing the subtitles. They're also not shown if `hud_draw` is set to 0.

## Defining captions profiles

Caption profile defines the color of the captions.

Caption profiles are configured in **sound/captions_profiles.txt**. Caption profile name is a 2-letter identifier.

Example:

```
// Barney
ba 0 115 255

// Scientist
sc 225 225 225
```

It defines two captions profiles intended to use with Barney's and Scientist's sentences. The numbers after the name represent the RGB color. In this example the profile `ba` defines a bluish color and the profile `sc` defines a whitish color.

## Defining captions

Captions for subtitles are defined in **sound/captions.txt**. Each caption has an identifier that corresponds to the entry from **sound/sentences.txt**. When a monster or `ambient_generic` starts playing a scripted sentence it automatically sends an identifier to the client. If there's a caption corresponding to this identifier it renders on the client.

Example:

```
BA_LATE ba Morning, Mr. Freeman. Looks like you're running late.
```

In this example `BA_LATE` corresponds to the sentence of the same name from **sound/sentences.txt**. The `ba` is a caption profile name.

The hold time of the caption depends on the duration specified in the `scripted_sentence`. If the sentence is played via `ambient_generic` the hold time is calculated automatically from the length of the text. Also in this case the speaker icon is automatically added to the caption to imitate the message incoming from the radio.

The full format of the caption line looks like this:

```
SENTENCE_NAME !duration ^delay profile Text
```

`!duration` (optional) is the forced duration of the caption (in seconds). Use it if you don't want to rely on the duration specified in the `scripted_sentence` or automatic calculation of the hold time.

`^delay` (optional) is the delay before showing the caption (in seconds). Useful if the start of the sentence doesn't include any words (e.g. in Half-Life: Induction it's used for phone calls as there're beep sounds at the start of the sentence).

`profile` is a 2-letter profile name from **sound/captions_profiles.txt** (see above).

Example:

```
SC_EXAMPLE !15 ^3 sc This message is delayed by 3 seconds and forced to be shown for 15 seconds despite its length or the duration defined in scripted_sentence.
```

## Captions are client-side

Captions reside fully on the client. The server only sends the caption identifier and the duration. This allows different clients to have different captions (e.g. localized for their chosen language) in the multiplayer environment.

## Configuring captions font

The captions are rendered with the same font as console and chat messages on the screen. This font is defined in the **resource/TrackerScheme.res** file. The `"EngineFont"` entry allows configuring the font size depending on the client screen size.

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
