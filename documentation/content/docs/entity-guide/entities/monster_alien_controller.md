---
bookHidden: true
bookToC: false
---

# monster_alien_controller

[TWHL](https://twhl.info/wiki/page/monster_alien_controller)

## Bugfixes

* Fixed alien controller resetting its facing in the idle state.

### Skill variables

* **sk_controller_health** - monster's health.
* **sk_controller_dmgzap** - the head energy ball projectile damage.
* **sk_controller_dmgball** - the energy ball projectile damage.
* **sk_controller_speedball** - the energy ball movement speed.

### Default classification

`Alien Military`

### Soundscripts

* **Controller.Idle** - idle sounds.
* **Controller.Alert** - alert sounds.
* **Controller.Pain** - pain sounds.
* **Controller.Die** - death sounds.
* **Controller.Attack** - firing an energy ball.
* **Controller.HeadElectro** - when the head ball hits the enemy. Emitted from the projectile.
* **Controller.ZapElectro** - when the energy ball hits the enemy. Emitted from the projectile.

### Visuals

* **Controller.EnergyBall** - energy balls attached to arms of the controller.
* **Controller.HeadOpenLight** - entity light emitted when the controller's head gets open before the headball attack.
* **Controller.HeadShootLight** - entity light emitted when a controller shoots its headball projectile.
* **Controller.EnergyBallLight** - entity light emitted from the controller's arms when it changes/resets its energy ball brightness.
* **Controller.ZapBall** - energy balls coming from arms.
* **Controller.HeadBall** - big energy ball from the controller's head.
* **Controller.HeadBallBeam** - a temporary beam produced when the headball hits the target.
* **Controller.HeadBallLight** - entity light emitted from the headball as it travels.

Shared visuals:

* **Controller.EnergyBallBase** - sprite, color and alpha shared by **Controller.EnergyBall**, **Controller.HeadBall** and **Controller.ZapBall**.
* **Controller.EnergyBallLightBase** - color shared by **Controller.HeadOpenLight**, **Controller.HeadShootLight** and **Controller.EnergyBallLight**.

Notes:

* **Controller.ZapBall** customizations are also applied to projectiles spawned by [env_energy_ball_trap]({{< ref env_energy_ball_trap >}}).
