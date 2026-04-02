---
title: "delta.lst"
bookToC: false
---

# {{% param "title" %}}

**delta.lst** file is used for entity networking in Half-Life. It describes the number of bits to send for certain variables that encode the entity state. The state is used by the client for various purposes (mostly rendering).

The mods can use a custom **delta.lst** to expand the networking protocol. While Featureful SDK should work with default **delta.lst** from valve/ directory, using some features do require having the custom file. Its latest version is distributed with the sample mod. Modmakers shouldn't forget to include the custom **delta.lst** in their mods so these features work properly.

Just for documentation purposes we list the changes comparing to the default **delta.lst**:

* `clientdata_t` and `weapon_data_t` structs set `m_iId` to use 6 bits in order to support prediction for more weapons.
* `entity_state_t` sets `skin` to use 12 bits for proper rendering of attached sprites and beams on the maps with high number of entities.
* `entity_state_t` sets `effects` to use 12 bits to support more effects recognized by the client.
