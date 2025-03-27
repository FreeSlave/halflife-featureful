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

## Path finding changes

* Monsters now properly detour brush entities that block the path between linked nodes (e.g. `func_breakable`, or a closed door which can't be opened by a monster), instead of getting stuck. If the detour doesn't exist they fallback to the classic behavior (waiting for door to open or for `func_breakable` to get broken).
* Corpses no longer block path finding when the corpse lies at the top of the stairs and the NPC is going up the stairs (original Half-Life bug).

## New common features

* ![](/images/spirit.png) ![](/images/svencoop.png) Health, relationship class, blood color, monster model and its gibs model can be customized in map editor.
* ![](/images/svencoop.png) Customizable hull size.
* ![](/images/svencoop.png) New parameter `Is Player Ally` inverts monster relationship with player.
* ![](/images/svencoop.png) New parameter `In-game name` to set a custom display name.
* ![](/images/svencoop.png) New parameter `Monster Roaming` to make monsters free roam on nodes.
* ![](/images/spirit.png) New flag `Don't Drop Gun` to disable weapon and item dropping upon death (applicable to monsters who drop a gun).
* New flag `Non-solid corpse` that makes monster's model non-solid upon dying (useful if you want to prevent blocking doors or pushables by dead bodies).
* New parameter `Trigger Alternative Condition`. Additional condition to fire a Trigger Target.
* Repel spawners (human grunts and male assassins when they come down with a rope) have options to configure the spawned monster similar to monstermaker.
* New parameter `Custom Sound Mask` to configure what sounds monster regards.
* Monsters can correctly patrol on path_corners now (the feature was unfinished in Half-Life) and use wait times specified in path_corners.
* `impulse 103` (Report AI state) is much more informative now.
* New parameter `Custom Field Of View` to configure the monster's field of view.
* New flag `Act ouf of PVS` to make monsters look for enemies and listen to the world independently of the player's presence.

## Squad monsters features

* ![](/images/opfor.png) When squad member dies while not being in combat others go into the hunt state searching the area for the attacker.
* When squad leader dies the new one is selected to prevent squad dissolving upon leader's death.
* Monsters now properly switch to the combat state when they get enemy info from other squad member.

## Following monsters features

* ![](/images/svencoop.png) Many monsters who are by default player's enemies can be used to follow a player when set to be a player ally (similar to scientists and security guards in Half-Life).
* ![](/images/spirit.png) Ally monsters can be 'locked' by master, so they decline following even if they are not pre-disaster.
* ![](/images/opfor.png) Opposing Force soldiers following the player will attack player's attacker.
* New flag `Ignore player pushing` to make monster ignore player's attempts to push them.
* When monster is "pushed" by a player, but can't move away because of the other ally monster blocking the path, this second monster also tries to move away. This may help to get out when two or more ally monsters are blocking the player.

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
