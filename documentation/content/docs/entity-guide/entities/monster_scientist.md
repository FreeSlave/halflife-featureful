---
bookHidden: true
bookToC: false
---

# monster_scientist

[TWHL](https://twhl.info/wiki/page/monster_scientist)

### Changes

* Can heal allies, e.g. security guards or other scientists (originally scientists healed only players).
* Healing now plays a medshot sound.
* Scientists now play a death sound instead of silently dying.

### Skill variables

* **sk_scientist_health** - monster's health.
* **sk_scientist_heal** - the amount of healing per needle.
* **sk_scientist_heal_time** - delay before the scientist can heal again.

### Default classification

`Human Passive`

### Soundscripts

* **Scientist.Pain** - pain sound.
* **Scientist.Die** - death sound. Derived from **Scientist.Pain**
* **Scientist.Heal** - medical shot sound.
