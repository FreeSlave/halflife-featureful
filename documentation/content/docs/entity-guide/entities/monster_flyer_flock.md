---
bookHidden: true
bookToC: false
---

# monster_flyer_flock

[TWHL](https://twhl.info/wiki/page/monster_flyer_flock)

### Changes

* Boids (flocking flyers) no longer block the player movement.

### New parameters

* `Custom Fly Speed`- an average speed of the flyer. Default is 125.
* `Custom Turn Rate` - how fast the flyers turn. Default is 75.
* `Custom Check Distance` - how far flyers check to not collide with the walls. Default is 192. Set this value higher if flyers tend to bump into the walls on your map. Setting a higher Turn Rate and slower Fly Speed helps as well.
* `Custom Too Close Distance`. Flyers try to maintain a certain distance between each other and especially the flock leader. This is the minimum distance the flyers should maintain. Default is 100.
* `Custom Too Far Distance` - the maximum distance the flyers should maintain between each other and especially the flock leader. Default is 256. This must be larger than the Too Close Distance.

{{% hint info %}}
If the flyers tend to bump into the walls on your map try do the following:

* Set the higher than default Check Distance, e.g. 512.
* Set the higher than default Turn Rate, e.g. 100.
* Set the slower than default Fly Speed, e.g. 100.
{{% /hint %}}

### Soundscripts

* **FlockingFlyer.Idle** - idle sounds.
* **FlockingFlyer.Alert** - played when some flyer in the flock is gettng killed. Not used because killable boids are not implemented yet.
