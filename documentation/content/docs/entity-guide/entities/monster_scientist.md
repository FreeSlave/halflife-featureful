---
bookHidden: true
bookToC: false
---

# monster_scientist

[TWHL](https://twhl.info/wiki/page/monster_scientist)

### Changes

* Can heal allies, e.g. security guards or other scientists (in original Half-Life scientists healed only players).
* Healing now plays a medshot sound.
* Scientists now play a death sound instead of silently dying.
* Scientists now comment on smells (using `SC_SMELL` sentence group). This didn't work in original Half-Life.

### Skill variables

* **sk_scientist_health** - monster's health.
* **sk_scientist_heal** - the amount of healing per needle.
* **sk_scientist_heal_time** - delay before the scientist can heal again.

### Default classification

`Human Passive`

### Default display name

`Scientist`

### Soundscripts

* **Scientist.Pain** - pain sound.
* **Scientist.Die** - death sound. Derived from **Scientist.Pain**
* **Scientist.Heal** - medical shot sound.
