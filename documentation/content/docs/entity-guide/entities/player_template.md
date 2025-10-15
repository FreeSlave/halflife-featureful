---
bookHidden: true
bookToC: false
---

# player_template

Apply a [player template]({{< ref player-templates >}}) to the player who activated this entity. This won't do anything if the provided template is already set on the player. If the player template defines a custom viewmodel for the player's current weapon, the weapon will be re-deployed.

{{% hint info %}}
If the player template disables HEV suit sentences, the sentence queue is getting cleared up, but if where was already a sentence playing during the template switch, it won't stop.
{{% /hint %}}

### Parameters

* `Template name` - the player template name to apply. Leave it empty or set `default` value to reset the player's template to default.

### Master capability

If player belongs to the specified template, the master state is On. Otherwise it's Off.
