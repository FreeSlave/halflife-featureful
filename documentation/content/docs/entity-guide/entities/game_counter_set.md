---
bookHidden: true
bookToC: false
---

# game_counter_set

[TWHL](https://twhl.info/wiki/page/game_counter_set)

This entity is used to send the *Set* use-type along with a number value to other entities. If the value must be dynamically calcualted from other values use [calc_eval_number]({{< ref calc_eval_number >}}) instead.

It can be used with following entities:

* [env_global]({{< ref env_global >}}) - set the integer value associated with the global variable.
* [game_counter]({{< ref game_counter >}}) - set the integer value.
* [game_number]({{< ref game_number >}}) - set the value.
* [player_inventory]({{< ref player_inventory >}}) - set the number of inventory items.
