---
bookHidden: true
bookToC: false
---

# monster_tripmine

[TWHL](https://twhl.info/wiki/page/monster_tripmine)

## Bugfixes

* Fixed multiple tripmines placed on the door blocking the door movement. If tripmines have been activated before the door started moving, they will explode instead. Otherwise they will be dropped to the ground as a pickable weapon.

### New spawnflags

* `Triggerable` - allows to trigger the tripmine explosion by tripmine's targetname.

### Skill variables

* **sk_plr_tripmine** - explosion damage.
* **sk_tripmine_health** - tripmine health.

### Soundscripts

* **TripmineGrenade.Deploy** - when tripmine is installed on the wall. Plays until beam activation. Must be looped.
* **TripmineGrenade.Charge** - when tripmine is installed on the wall.
* **TripmineGrenade.Activate** - beam activation sound.

### Visuals

* **Tripmine.Beam** - the tripmine beam.
