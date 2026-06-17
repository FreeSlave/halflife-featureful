---
title: "Visual effects"
---

# {{% param "title" %}}

While graphical improvements are far from the primary goal of the Featureful SDK, it still provides some graphic features most of which are configurable to some extent.

## Wallpuffs

*Wallpuffs* are small clouds of dust that play when a bullet-based weapon hits the world geometry or brush objects. This effect is similar to one used in Counter Strike, Day of Defeat and some other mods.

* Wallpuffs are not enabled by default. The user can enable wallpuffs by setting the `cl_weapon_wallpuff` cvar to 1. See [CVars]({{< ref "cvars/#weapon-effects" >}}). See [Advanced settings]({{< ref "shipping-your-mod/#advanced-settings" >}}) for exposing the cvar to the settings UI.
* To enable wallpuffs in the mod by default set the `weapon_wallpuff.enabled_by_default` feature to true in **features/featureful_client.cfg**.
* The sprites used for wallpuffs can be configured by setting `wall_puff1`-`wall_puff4` in **features/featureful_client.cfg**, allowing for up to 4 different sprites.
* By default the single **sprites/stmbal1.spr** sprite is used for wallpuffs (a standard resource that exists in Half-Life).
* The wallpuff color depends on the [material]({{< ref materials >}}) the bullet hits and can be configured.

{{< figure src="/images/wallpuff1.png" caption="Grey wallpuff on concrete walls" >}}
{{< figure src="/images/wallpuff2.png" caption="Brown wallpuff on wood material" >}}

## Wall impact particles

Wall impact particles spawn when the bullet-based weapon hits the world geometry or brush objects (also some projectiles like [nail]({{< ref nail >}})) can produce wall impact particles). This effect existed in Half-Life from the beginning but Featureful SDK provides some options for its customization.

* The wall impact particles style can be configured by a user via the `cl_wallimpact_style` [cvar]({{< ref "cvars/#weapons-effects" >}}). See [Advanced settings]({{< ref "shipping-your-mod/#advanced-settings" >}}) for exposing the cvar to the settings UI.
* The default style used in the mod is configured by `wallimpact_style.default` feature in **features/featureful_client.cfg**.
* The particles color can be changed per [material]({{< ref materials >}}).

{{< figure src="/images/wallimpact_hl.png" caption="Default Half-Life wall impact particles" >}}
{{< figure src="/images/wallimpact_q1.png" caption="Quake style wall impact particles" >}}
{{< figure src="/images/wallimpact_q2.png" caption="Quake II style wall impact particles" >}}
{{< figure src="/images/wallimpact_colored.png" caption="Custom impact particles color for wooden surfaces" >}}

## Blood

Blood effects are shown on monsters when they take damage from tracing attacks (bullets, lasers, projectiles, player's melee weapons). The amount of blood depends on the incoming damage. Featureful SDK provides options for the blood effect customization:

* The blood splatter style can be configured by a user via the `cl_bloodsplatter_style` cvar. See [CVars]({{< ref "cvars/#blood-effects" >}}). See [Advanced settings]({{< ref "shipping-your-mod/#advanced-settings" >}}) for exposing the cvar to the settings UI.
* The default blood splatter style used in the mod is configured by `bloodsplatter_style.default` feature in **features/featureful_client.cfg**.
* The user can enable blood streams by setting the `cl_bloodstream` to 1. Blood streams spawn only when the incoming damage is high enough. See [CVars]({{< ref "cvars/#blood-effects" >}}). See [Advanced settings]({{< ref "shipping-your-mod/#advanced-settings" >}}) for exposing the cvar to the settings UI.

{{% columns %}}

{{< figure src="/images/blood_hl.png" caption="Default Half-Life blood splatter and spray" >}}

<--->

{{< figure src="/images/blood_dayone.png" caption="Half-Life Day One blood" >}}

<--->

{{< figure src="/images/blood_q1.png" caption="Quake blood particles" >}}

{{% /columns %}}
{{% columns %}}

{{< figure src="/images/blood_q2.png" caption="Quake II blood particles" >}}

<--->

{{< figure src="/images/blood_stream.png" caption="Blood stream (like in Half-Life Alpha)" >}}

<--->

{{% /columns %}}

## Entities

The list of entities related to creation of various visual effects in game:

* [env_beam]({{< ref env_beam >}})
* [env_blood]({{< ref env_blood >}})
* [env_dlight]({{< ref env_dlight >}})
* [env_quakefx]({{< ref env_quakefx >}})
* [env_shockwave]({{< ref env_shockwave >}})
* [env_shooter]({{< ref env_shooter >}})
* [env_smoker]({{< ref env_smoker >}})
* [env_splash]({{< ref env_splash >}})
* [env_streak]({{< ref env_streak >}})
* [particle_shooter]({{< ref particle_shooter >}})

## Fullbright textures on studio models

Featureful SDK supports rendering of fullbright textures (fullbright flag) on studio models. Such textures are drawn in full brightness even in the complete darkness (example: the battery model from Sven Co-op).

When running the mod on GoldSource, Featureful SDK uses a special logic to render parts of the model in fullbright. Since Xash3D handles fullbright textures on studio models natively, Featureful SDK skips this special logic when the mod is running on Xash3D.

{{< figure src="/images/fullbright.png" caption="Regular battery next to one with fullbright texture, in the dim lit area" >}}

{{% hint warning %}}
The way Featureful SDK implements support for fullbright textures on GoldSource may conflict with external renderers like Meta Renderer.

If you intend your mod to run on such renderer and use fullbright textures you need to add the following line to the **features/featureful_client.cfg**:

```
fullbright_textures false
```
{{% /hint %}}
