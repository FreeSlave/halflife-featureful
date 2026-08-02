---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# gib

A gib thrown by [env_shooter]({{< ref env_shooter >}}), [gibshooter]({{< ref gibshooter >}}) and gibbed monsters.

{{% hint info %}}
Gibs spawned by [func_breakable]({{< ref func_breakable >}}) are temporary entities that exist only on client. These are different things.
{{% /hint %}}

### Changes

* Save-restore is enabled on gibs (in Half-Life gibs disappear after the save-load).
* Can be launched as a [projectile]({{< ref "weapon-templates/#projectile" >}}) although currently serves no real purpose.
