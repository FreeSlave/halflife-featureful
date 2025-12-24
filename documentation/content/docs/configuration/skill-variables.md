---
title: "Skill variables"
---

# {{% param "title" %}}

Featureful SDK uses the same skill system as Half-Life (values are edited via **skill.cfg**), but with additional twists.

### Skill variables are no longer cvars

The skill variables are no longer implemented as cvars. This allows for some new features like dynamic addition of the new skill variables and support for difficulty-independent skill variables.

{{% hint warning %}}
Setting non-skill cvars in the skill configuration file is invalid now (doing so has always been discouraged). Use **features/featureful_exec.cfg** to override the server cvar values.
{{% /hint %}}

### Difficulty-independent skill variables

If skill variable doesn't end with a digit, it's considered to be difficulty-independent - its value will be used across all difficulties, unless difficulty-specific variable is specified.

```
sk_barney_health 50
```

As you notice the `sk_barney_health` doesn't end with 1, 2 or 3. So, if `sk_barney_health1` is missing the value of `sk_barney_health` will be used on easy difficulty.

If both difficulty-independent and difficulty-specific variable are defined, the latter has a priority on an appropriate difficulty.

### Optional sk_ prefix

The skill variables can be referred with *sk_* prefix omitted. E.g. `sk_barney_health` and `barney_health` refer to the same variable.

The *sk_* prefix was a notation used to distinguish the skill variables from other cvars. As skill variables are not cvars anymore, the prefix is no longer needed.

### get_skill command

Use `get_skill` console command to print the values for the skill variable on all difficulties. E.g. `get_skill barney_health`.

### Overriding skill values for entity template

Entity templates allow entities to override the skill variables they're using. See [Entity templates]({{< ref "entity-templates/#skill" >}}).

### skillopfor.cfg

If **skillopfor.cfg** exists in the mod directory the game will automatically read the variables from this file as well.
