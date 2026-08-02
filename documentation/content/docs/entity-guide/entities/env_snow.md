---
bookHidden: true
bookToC: false
---

# ![](/images/cstrike.png) env_snow

Featureful SDK implements snow effect similar to one used in Counter Strike and Condition Zero (e.g. on the **cs_office_cz** map).

`env_snow` is similar to [env_rain]({{< ref env_rain >}}), but spawns snowflakes instead of raindrops. It doesn't have any extra effects besides snowflakes. Like rain, snow is affected by a global wind.

The snowflake default sprite is **sprites/effects/snowflake.spr**.


{{% hint info %}}
Snow and rain can be used together to create mixtured precipitation effect.
{{% /hint %}}

{{% hint info %}}
Use different color to create other kinds of precipitation like dust, falling ashes, etc.
{{% /hint %}}

See also: [func_snowvolume]({{< ref func_snowvolume >}})

### Parameters

* Same base parameters as [env_rain]({{< ref env_rain >}}).

### Spawnflags

* Some spawnflags are shared with [env_rain]({{< ref env_rain >}}).
* `Don't dissolve in water` - disable snowflakes dissolving (fading) when the snowflake enters the water.
