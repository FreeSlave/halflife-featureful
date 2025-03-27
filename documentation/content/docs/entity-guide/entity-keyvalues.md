---
title: "Entity keyvalues"
---

# {{% param "title" %}}

Entity keyvalues are pairs of key and value strings that are passed to the entity upon its initialization. They also can be set via [trigger_changevalue]({{< ref trigger_changevalue >}}) during the game.

## Entvars

Entvars are the base keyvalues that exist in all entities. They're available for reading in [trigger_condition]({{<  ref trigger_condition >}}) and for custom operations in [trigger_changevalue]({{< ref trigger_changevalue >}}), although changing some of them is not recommended.

{{% hint warning %}}
Some entvars have special meaning in the engine, and changing them arbitrarily can lead to unexpected or even catastrophic consequences.
{{% /hint %}}

## List of entvars

|Key name|Value&nbsp;type|Description|
|-----------------|-----------------|-----------------|
|classname|string|Classname of the entity. Must not be changed|
|globalname|string|Global name used for level transitions. Must not be changed|
|origin|3D-vector|Position of the entity in the world coordinates|
|velocity|3D-vector|Direction and speed of the entity|
|movedir|3D-vector|Movement direction used by trigger_push or func_rotating.|
|angles|3D-vector|Angles of the entity. (pitch, yaw, roll)|
|avelocity|3D-vector|Angular velocity. (rotational speed, degrees per second)|
|punchangle|3D-vector|Auto-decaying view angle adjustment. (e.g. when an alien grunt punches you)|
|v_angle|3D-vector|Viewing angle. (player only)|
|endpos|3D-vector|Pre-calculated end position of parametric projectiles (not used yet)|
|startpos|3D-vector|Start position of parametric projectiles (not used yet)|
|impacttime|time|-|
|starttime|time|-|
|idealpitch|float|-|
|pitch_speed|float|-|
|ideal_yaw|float|The angle Yaw the monster wants to turn to|
|yaw_speed|float|Monster's Yaw turn speed|
|modelindex|integer|Index of the entity model. Must not be changed|
|model|string|Name of the entity model. Must not be changed|
|viewmodel|string|Player's viewmodel. (what you see in first person). Must not be changed|
|weaponmodel|string|Player's weaponmodel. (what other players see when they look at you). Must not be changed|
|absmin|3D-vector|Absolute mins; coordinates of the lower left back corner of the entity hull in world space.|
|absmax|3D-vector|Absolute maxs; coordinates of the upper right front corner of the entity hull in world space.|
|mins|3D-vector|Entity hull mins; coordinates of the lower left back corner of the entity hull.|
|maxs|3D-vector|Entity hull maxs; coordinates of the upper right front corner of the entity hull.|
|size|3D-vector|Entity hull size.|
|ltime|time|Local time of the entity. Used by brush entities.|
|nextthink|time|Time-value of the entity, telling the engine when to call the "think"-function of the entity next.|
|[movetype](#list-of-movetype-states)|integer|Movetype-value, telling engine how this entity should behave physically.|
|[solid](#list-of-solid-states)|integer|Solid-value, telling engine how OTHER entities should behave physically when they touch this entity. (see solid value list below)|
|skin|integer|Skin of this entity. (texture selection for studiomodels)|
|body|integer|Body of this entity. (sub-model selection for studiomodels)|
|[effects](#list-of-effects-states)|integer|Effects-flags, telling the engine how to render this entity.|
|gravity|float|Fraction of "normal" gravity affecting the entity|
|friction|float|Used by engine when movetype is MOVETYPE_BOUNCE (inverse elasticity); used by several entities for friction. (e.g. func_pushable)|
|light_level|integer|-|
|sequence|integer|The index of the current animation sequence|
|gaitsequence|integer|Movement animation sequence for player. (0 for none)|
|frame|float|Percentage of playback position in animation sequences. (0 - 255)|
|animtime|time|World time when frame was set.|
|framerate|float|Animation playback rate. Normal framerate for sprites is 10.0, and for models it's 1.0.|
|scale|float|Sprite or model rendering scale|
|rendermode|integer|Render-mode (Normal, Texture, Solid, etc.)|
|renderamt|float|Render-amount (0-255)|
|rendercolor|3D-vector|Render-color for color render-mode. Red, Green and Blue components|
|renderfx|integer|Render-effect|
|health|float|Current entity health. Note: player must have integer value as health to avoid bugs|
|frags|float|Player's frags in multiplayer. Used by some entities as a generic float storage|
|weapons|integer|Bit-mask for available weapons of NPCs|
|[takedamage](#list-of-takedamage-states)|integer|Takedamage-value, telling the engine if and how this entity can take damage.|
|[deadflag](#list-of-deadflag-states)|integer|Deadflag-value, telling the engine what the life/death status of the entity is.|
|view_ofs|3D-vector|View offset vector|
|button|integer|Button-flags, sent from client telling the server, which buttons are pressed.|
|impulse|integer|-|
|chain|entity|-|
|dmg_inflictor|entity|-|
|enemy|entity|-|
|aiment|entity|Entity-pointer when movetype is MOVETYPE_FOLLOW.|
|owner|entity|Entities have no collision with their owner and vice versa. Usually projectiles have this to avoid contact with the entity that fired a projectile. For monsters that came from monstermaker, the monstermaker is their owner (used to tell monstermaker about monster's death).|
|spawnflags|integer|Spawnflags bit-mask, set by mapper|
|flags|integer|Special entity flags|
|colormap|integer|Low byte topcolor; high byte bottomcolor. This specifies model custom colors, as seen in use on player models.|
|team|integer|-|
|max_health|float|Maximum health of player or monster. Note: player must have integer value as max_health to avoid bugs|
|teleport_time|time|-|
|armortype|float|-|
|armorvalue|float|Current amount of player's armor|
|[waterlevel](#list-of-waterlevel-states)|integer|Value to tell the server if the entity is swimming, on dry ground or wading.|
|watertype|integer|-|
|target|string|Name of the target of this entity.|
|targetname|string|Name of this entity.|
|netname|string|For squadmonsters it's the name of the squad (set by mapper). For players, this contains their nickname. Used as a string by other some entities|
|message|string|Used as a string by some entities|
|dmg_take|float|-|
|dmg_save|float|-|
|dmg|float|Amount of damage this entity will inflict on others if it deals damage. Valid for some projectiles and trigger_hurt|
|dmgtime|time|-|
|noise|string|-|
|noise1|string|-|
|noise2|string|-|
|noise3|string|-|
|speed|float|Speed of this entity (not current speed; that's what velocity is!)|
|air_finished|time|-|
|pain_finished|time|-|
|radsuit_finished|time|-|

{{% hint info %}}
**time** keyvalues are actually float that are usually compared to the game time by the entity logic.
{{% /hint %}}

{{% hint info %}}
**entity** keyvalues can't be targeted by [trigger_changevalue]({{< ref trigger_changevalue >}}), but can be read by [read_keyvalue](#read_keyvalue-command).
{{% /hint %}}

## read_keyvalue command

Run this command in console in the developer mode to read the entity's entvars.

Usage: `Usage: read_keyvalue <targetname> <keyname>`.

Example: `read_keyvalue mybreakable health` to get the health value of entity called `mybreakable`.

If `<targetname>` is `!cross` then it will report the key-value of the entity under the crosshair.

## List of "flags" flags

|Flag value|Flag name|Description|
|--------|--------|--------|
|1|FL_FLY|Entity can fly.|
|2|FL_SWIM|Entity can swim.|
|4|FL_CONVEYOR|Entity is a conveyor.|
|8|FL_CLIENT|Entity is a client/player.|
|16|FL_INWATER|Entity is in water.|
|32|FL_MONSTER|Entity is a monster.|
|64|FL_GODMODE|Player is invincible.|
|128|FL_NOTARGET|Entity won't be attacked by enemies.|
|256|FL_SKIPLOCALHOST|Don't tell clients abouts this entity; they're predicting this entity by themselves.|
|512|FL_ONGROUND|Entity is on the ground.|
|1024|FL_PARTIALGROUND|At least one of the 4 corners of this entity's hull does not touch the ground.|
|2048|FL_WATERJUMP|Player jumping out of water.|
|4096|FL_FROZEN|Player is frozen. (by [trigger_camera]({{< ref trigger_camera >}}) or [trigger_playerfreeze]({{< ref trigger_playerfreeze >}}))|
|8192|FL_FAKECLIENT|Fake client, simulated server side; don't send network messages to them. (Used for bots)|
|16384|FL_DUCKING|Player is fully crouching.|
|32768|FL_FLOAT|Entity can float on water (e.g. [func_pushable]({{< ref func_pushable >}}))|
|8388608|FL_MONSTERCLIP|Entity collides with other entiies with the same flag: `func_monsterclip` and monsters with `Monsterclip` spawnflag.|

## List of "deadflag" states

|Value|Name|Description|
|--------|--------|--------|
|0|DEAD_NO|Alive|
|1|DEAD_DYING|Monster is in dying animation|
|2|DEAD_DEAD|Dead, still|
|3|DEAD_RESPAWNABLE|-|
|4|DEAD_DISCARDBODY|-|

## List of "takedamage" states

|Value|Name|Description|
|--------|--------|--------|
|0|DAMAGE_NO|Does not take damage.|
|1|DAMAGE_YES|Does take damage.|
|2|DAMAGE_AIM|Does take damage and allow the autoaim on it.|

## List of "solid" states

|Value|Name|Description|
|--------|--------|--------|
|0|SOLID_NOT|No interaction with other objects.|
|1|SOLID_TRIGGER|Notice touch, but don't block.|
|2|SOLID_BBOX|Touch on edge, block.|
|3|SOLID_SLIDEBOX|Touch on edge, but not if other is on ground.|
|4|SOLID_BSP|BSP clip, touch on edge, block.|

## List of "waterlevel" states

|Value|Name|Description|
|--------|--------|--------|
|0|WL_NotInWater|Not in water.|
|1|WL_Feet|Feet underwater.|
|2|WL_Waist|Waist underwater.|
|3|WL_Eyes|Head underwater.|

## List of "movetype" states

|Value|Name|Description|
|--------|--------|--------|
|0|MOVETYPE_NONE|Never moves.|
|3|MOVETYPE_WALK|Player only; moving on the ground.|
|4|MOVETYPE_STEP|Gravity, special edge handling; monsters use this.|
|5|MOVETYPE_FLY|No gravity, but still collides with stuff.|
|6|MOVETYPE_TOSS|Receive gravity and collisions.|
|7|MOVETYPE_PUSH|No clip to world; push and crush. (e.g. func_train)|
|8|MOVETYPE_NOCLIP|No gravity; no collisions; still handle velocity and angular velocity.|
|9|MOVETYPE_FLYMISSILE|Handled as if was larger for monsters. (Collides more easily)|
|10|MOVETYPE_BOUNCE|Just like "Toss", but reflect velocity when contacting surfaces.|
|11|MOVETYPE_BOUNCEMISSILE|Like "Bounce", but without gravity.|
|12|MOVETYPE_FOLLOW|Track movement of aiming entity.|
|13|MOVETYPE_PUSHSTEP|BSP model that needs gravity and world collisions; uses nearest hull for world collision. (e.g. func_pushable)|

## List of "effects" flags

|Flag value|Flag name|Description|
|--------|--------|--------|
|1|EF_BRIGHTFIELD|Swirling cloud of yellow particles. (Used when a monster is stuck in wall to receice the level designer's attention)|
|2|EF_MUZZLEFLASH|Single frame ELIGHT on entity attachment 0.|
|4|EF_BRIGHTLIGHT|Dynamic light centered at entity origin.|
|8|EF_DIMLIGHT|Player's flashlight.|
|16|EF_INVLIGHT|Get model-lighting from ceiling. (Barnacles and ceiling turrets have this)|
|32|EF_NOINTERP|Don't interpolate the next frame.|
|64|EF_LIGHT|Rocket flare pulsing glow sprite.|
|128|EF_NODRAW|Don't draw entity. This also makes entity to not send updates to the client. Don't use it on entities with attached beams or sprites.|
