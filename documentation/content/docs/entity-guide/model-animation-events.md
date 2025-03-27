---
title: "Model animation events"
bookToC: false
---

# {{% param "title" %}}

* The model animations in Half-Life can have *events* attached to specific frames.
* Data-wise event is just a number with optional *options* string.
* In the game events are different effects played along with the model animation when it reaches the frame the event resides on.
* The events are handled and processed by the game logic. Entity implementations are responsible for handling the events, but there's a number of shared events, recognized by all monster entities.

The incomplete list of shared events:

* `1004` - play a sound provided in the *options* data of the event on the *body* channel and *idle* attenuation.
* `1008` - play a sound provided in the *options* data of the event on the *voice* channel and *idle* attenuation.
* `1011` - play a sound provided in the *options* data of the event on the *body* channel and *norm* attenuation. Opposing Force compatible.
* `1012` - play a sound provided in the *options* data of the event on the *voice* channel and *norm* attenuation. Opposing Force compatible.
* `1013` - play a sound provided in the *options* data of the event on the *weapon* channel and *norm* attenuation. Opposing Force compatible.
* `1014` - play a soundscript provided in the *options* data of the event. Featureful SDK specific.
* `2001` - play the `NPC.BodyDrop_Light` soundscript.
* `2002` - play the `NPC.BodyDrop_Heavy` soundscript.
* `2010` - play the `NPC.SwishSound` soundscript.

See also [event list on the303](https://www.the303.org/tutorials/gold_qc.htm#A1)
