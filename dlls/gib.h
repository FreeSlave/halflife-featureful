#pragma once
#ifndef GIB_H
#define GIB_H

// when calling KILLED(), a value that governs gib behavior is expected to be
// one of these three values
#define GIB_NORMAL			0// gib if entity was overkilled
#define GIB_NEVER			1// never gib, no matter how much death damage is done ( freezing, etc )
#define GIB_ALWAYS			2// always gib ( Houndeye Shock, Barnacle Bite )

#endif
