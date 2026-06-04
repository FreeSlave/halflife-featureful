---
title: Monsters
---

# {{% param "title" %}}

Half-Life Featureful introduces a lot of new monsters, as well as many changes to the monster configuration and AI.

For the list of monsters refer to [entities page]({{< ref "entity-guide#monster-entities" >}}).

## Common behavior changes

* After battle, in alert state, monsters try to freeroam and investigate danger sounds if `npc_active_after_combat` is set.
* Monsters forget their enemies if they didn't see an enemy for some period of time and `npc_forget_enemy_time` is set.
* When look for cover from danger, monsters will try to just run away if could not find a cover.
* When monster's path to the scripted sequence is blocked by an ally monster, the first monster will ask the second one to move away.

## Path finding changes

* Monsters now properly detour brush entities that block the path between linked nodes (e.g. `func_breakable`, or a closed door which can't be opened by a monster), instead of getting stuck. If the detour doesn't exist they fallback to the classic behavior (waiting for door to open or for `func_breakable` to get broken).
* Corpses no longer block path finding when the corpse lies at the top of the stairs and the NPC is going up the stairs (original Half-Life bug).

## New parameters

* ![](/images/spirit.png) ![](/images/svencoop.png) Health, relationship class, blood color, monster model and its gibs model can be customized in map editor.
* ![](/images/svencoop.png) Customizable hull size.
* ![](/images/svencoop.png) `Is Player Ally` inverts monster relationship with player.
* ![](/images/svencoop.png) `In-game name` to set a custom display name.
* ![](/images/svencoop.png) `Monster Roaming` to make monsters free roam on nodes.
* `Trigger Alternative Condition`. Additional condition to fire a Trigger Target.
* `Custom Sound Mask` - allows to configure what [sounds/scents]({{< ref ai-sound >}}) monster regards.
* `Custom Field Of View` - allows to configure the monster's field of view.
* `Trigger on death (locus = killer)`. When monster dies it will pass its killer as an activator. So, unlike the death `Trigger Condition` and `Trigger Target` combination, this allows to do something with the attacker via the `[LE]` parameters of other entities (e.g. grant some health via the [trigger_hurt_remote]({{< ref trigger_hurt_remote >}}) by setting the target to `!activator`). Note that if the monster is in uninterruptible scripted sequence this trigger will fire the moment monster's health goes to 0 or below, i.e. before the monster exits the sequence and realizes it died. We, however, don't recommend to rely on such behavior as it might change in future.

## New spawnflags

* ![](/images/spirit.png) `Don't Drop Gun` - disables weapon and item dropping upon death (applicable to monsters who drop a gun).
* `Non-solid corpse` - makes monster's model non-solid upon dying (useful if you want to prevent blocking doors or pushables by dead bodies).
* `Ignore move away requests` - monster will ignore others monsters' requests to move away (e.g. when monster wants to get somewhere but the other monster is blocking the way) and player's attempts to push the ally monster.
* `Act ouf of PVS` monsters will look for enemies and listen to the world independently of the player's presence. By default they do so only when in player's PVS.

## New common abilities

* Monsters can be set to use [passive health regeneration]({{< ref "entity-templates/#passive_regeneration" >}}) or [active health regeneration]({{< ref "entity-templates/#active_regeneration" >}}) via the [entity templates]({{< ref entity-templates >}}).
* Monsters can be set to use the [power shield]({{< ref "entity-templates/#power_shield" >}}) to have an additional layer of health with special impact effects via the [entity templates]({{< ref entity-templates >}}).
* Most monsters can be configured to have a [cloaking]({{< ref "entity-templates/#cloaking" >}}) ability via the [entity templates]({{< ref entity-templates >}}).
* The items dropped upon monster's death can be configured via [equipment_drop]({{< ref "entity-templates/#equipment_drop" >}}) and [loot_drop]({{< ref "entity-templates/#loot_drop" >}}) properties of [entity templates]({{< ref entity-templates >}}).

## Other features

* Repel spawners (human grunts and male assassins when they come down with a rope) have options to configure the spawned monster similar to monstermaker.
* Monsters can correctly patrol on path_corners now (the feature was unfinished in Half-Life) and use wait times specified in path_corners. Set the speed in `path_corner` to 200 or higher to make monster run instead of walking.
* `impulse 103` (Report AI state) is much more informative now.

## Squad monsters features

* ![](/images/opfor.png) When squad member dies while not being in combat others go into the hunt state searching the area for the attacker.
* When squad leader dies the new one is selected to prevent squad dissolving upon leader's death.
* Monsters now properly switch to the combat state when they get enemy info from other squad member.
* Monster now properly vacates its squad slot if the monster has been removed from the game via trigger.

## Following monsters features

* ![](/images/svencoop.png) Many monsters who are by default player's enemies can be used to follow a player when set to be a player ally (similar to scientists and security guards in Half-Life).
* ![](/images/spirit.png) Ally monsters can be 'locked' by master, so they decline following even if they are not pre-disaster.
* ![](/images/opfor.png) Opposing Force soldiers following the player will attack player's attacker.
* When a monster is following a player, but is unable to follow them due to another ally monster blocking the path, the first monster will ask the second one to move away.
* When a monster is "pushed" by a player, but is unable to move away due to another ally monster blocking the path, this second monster will also try to move away. This may help player to get out when two or more ally monsters are blocking the player.

## Talkmonster features

Talkmonsters are monsters who talk to each other and a player and can follow the player upon using. This includes allies from Half-Life and Opposing Force.

* ![](/images/opfor.png) New parameter `Suspicious` to make a monster pre-provoked towards the player.
* ![](/images/spirit.png) New parameter `Refusal Sentence` to substitute the default decline following sentence (when player uses a pre-disaster monster)
* ![](/images/spirit.png) New parameter `Speech Group` to make monster use sentences with a different prefix (e.g. make Barney use OT_ instead BA_ so he would speak like Otis).
* New parameter `Tolerance level` to configure how easily monster gets provoked by player hits.
* New parameter `Custom voice pitch`.
* New flag `Don't greet player` that prevents a monster saying hello to the player.
* New flag `Don't idle talk to player` to disallow idle speaking to the player.
* If monster does not have a question sentence group or it's missing from sentences.txt, the monster will not try to start a conversation with other friend. In Half-Life talkmonsters could start conversations even in the absence of question sentences that led to weird situations when other monster answered to unspoken questions.
* ![](/images/opfor.png)  If player is attacked by the enemy monster, the ally soldiers (Opposing Force human grunts) following the player will instantly know about the attacker even if they didn't see the enemy yet.

## New monsters

{{% details_header title="Civilian personnel" %}}
* [Civilian]({{< ref monster_civilian >}})
* [Cleansuit Scientist]({{< ref monster_cleansuit_scientist >}})
* [Otis]({{< ref monster_otis >}})
* [Barniel]({{< ref monster_otis >}})
* [Kate]({{< ref monster_kate >}})
* [Rosenberg]({{< ref monster_rosenberg >}})
* [Richard Keller]({{< ref monster_wheelchair >}})
{{% /details_header %}}

{{% details_header title="Human grunts" %}}
* [Opfor Human Grunt]({{< ref monster_human_grunt_ally >}})
* [Medic]({{< ref monster_human_medic_ally >}})
* [Engineer (Torch)]({{< ref monster_human_torch_ally >}})
* [Heave weapons grunt]({{< ref monster_hwgrunt >}})
{{% /details_header %}}

{{% details_header title="Blackops" %}}
* [Male Assassin]({{< ref monster_male_assassin >}})
* [Blackops Apache]({{< ref monster_blkop_apache >}})
* [Blackops Osprey]({{< ref monster_blkop_osprey >}})
{{% /details_header %}}

{{% details_header title="Robots" %}}
* [Robogrunt]({{< ref monster_robogrunt >}})
* [Robocop]({{< ref monster_robocop >}})
{{% /details_header %}}

{{% details_header title="Xen creatures" %}}
* [Baby Gargantua]({{< ref monster_babygarg >}})
* [Floater]({{< ref monster_floater >}})
* [Flybee]({{< ref monster_flybee >}})
* [Kingpin]({{< ref monster_kingpin >}})
* [Panthereye]({{< ref monster_panthereye >}})
* [Tor]({{< ref monster_alien_tor >}})
{{% /details_header %}}

{{% details_header title="Zombies" %}}
* [Gonome]({{< ref monster_gonome >}})
* [Zombie Barney]({{< ref monster_zombie_barney >}})
* [Zombie Soldier]({{< ref monster_zombie_soldier >}})
{{% /details_header %}}

{{% details_header title="Race X" %}}
* [Pitdrone]({{< ref monster_pitdrone >}})
* [Shock Trooper]({{< ref monster_shocktrooper >}})
* [Shock Roach]({{< ref monster_shockroach >}})
* [Voltigore]({{< ref monster_alien_voltigore >}})
* [Baby Voltigore]({{< ref monster_alien_babyvoltigore >}})
* [Pitworm]({{< ref monster_pitworm_up >}})
* [Geneworm]({{< ref monster_geneworm >}})
{{% /details_header %}}

## Children configuration

Some monsters can spawn other monsters, i.e. children. The monster's children can be configured via [entity templates]({{< ref "entity-templates/#children" >}}).

{{% hint info %}}
If the parent has a custom classification the children will inherit it as well.

If the parent is configured to spawn children of enemy classification the parent will force the children to have the same classification as itself to avoid situations when children start attack the parent.
{{% /hint %}}

## Tiny creatures

Some monsters are considered to be "tiny creatures". This affects how moving brush entities interact with them, depending on the `handle_tiny_creatures` server feature value:

* 0 - classic behavior. Tiny monsters can block moving brush entities.
* 1 - moving brush entities will crush tiny creatures even if they normally don't do damage to other entities (e.g. the door with `Damage inflicted when blocked` set to 0).
* 2 - moving brush entities won't collide with tiny creatures.

The goal of the feature is to provide a way to ensure that non-essential monsters like cockroaches won't interfere with gameplay and that hornets won't be used as exploit for blocking the doors.

The affected moving brush entities:

* [func_door]({{< ref func_door >}})
* [func_door_rotating]({{< ref func_door_rotating >}})
* [func_plat]({{< ref func_plat >}})
* [func_platrot]({{< ref func_platrot >}})
* [func_pushable]({{< ref func_pushable >}})
* [func_train]({{< ref func_train >}})
* [func_tracktrain]({{< ref func_tracktrain >}})

Currently the following monsters are considered to be tiny creatures:

* [monster_cockroach]({{< ref monster_cockroach >}})
* [hornet]({{< ref hornet >}})

## Debugging

Commands used for testing and debugging the monster behavior:

* `impulse 103` - report AI state of the monster in front of the player. This is now much more informative comparing to Half-Life.
* `make_start_following` - make the monster (or monsters) start following the player. This applies only to monsters who are able to follow the player. The argument must be the classname or the targetname.
* `make_stop_following` - make the monster (or monsters) stop following the player. This applies only to monsters who are able to follow the player. The argument must be the classname or the targetname.
* `report_ai_state` - report AI state of all monsters with the provided classname. It's like `impulse 103`, but player doesn't have to look at the monster.
* `watch_ai_schedules` - print the monster's schedule changes in the console. The monster is specified by a classname or by entity index. E.g. `watch_ai_schedules monster_barney` or `watch_ai_schedules 53`. Note: the watchers get reset on changelevel or save loading.
