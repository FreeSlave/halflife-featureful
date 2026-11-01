---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# ![](/images/spirit.png) shot

A shot (usually a sprite) spawned launched by [env_shooter]({{< ref env_shooter >}}) if the physics is set to one of the shot physics.

* Can be launched as a [projectile]({{< ref "weapon-templates/#projectile" >}}) although currently serves no real purpose other than being checked in some brush triggers.

### Skill variables

* `sk_gib_life` - default life duration, before the shot disappears. This is usually overriden by the [env_shooter]({{< ref env_shooter >}}) entity parameter.
