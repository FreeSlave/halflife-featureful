---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) item_generic

A static model. A simple alternative to [env_model]({{< ref env_model >}}). Despite the `item_` prefix this entity is neither pickable nor usable in any way. It's simply a decoration.

### Spawnflags

* `Drop to floor` - make the model automatically drop when the level is loaded. By default `item_generic` stays at the same spot as it's set in the editor.
* `No map transition` - prohibit entity from going across map transitions.
* `Apply gravity` - make item fall when the floor under it gets broken or removed.
