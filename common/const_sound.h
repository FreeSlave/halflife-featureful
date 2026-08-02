#pragma once
#ifndef CONST_SOUND_H
#define CONST_SOUND_H

// channels
#define CHAN_AUTO			0
#define CHAN_WEAPON			1
#define CHAN_VOICE			2
#define CHAN_ITEM			3
#define CHAN_BODY			4
#define CHAN_STREAM			5	// allocate stream channel from the static or dynamic area
#define CHAN_STATIC			6	// allocate channel from the static area
#define CHAN_NETWORKVOICE_BASE		7	// voice data coming across the network
#define CHAN_NETWORKVOICE_END		500	// network voice data reserves slots (CHAN_NETWORKVOICE_BASE through CHAN_NETWORKVOICE_END).
#define CHAN_BOT			501	// channel used for bot chatter.

// attenuation values
#define ATTN_NONE			0.0f
#define ATTN_NORM			0.8f
#define ATTN_IDLE			2.0f
#define ATTN_STATIC			1.25f

// pitch values
#define PITCH_NORM			100	// non-pitch shifted
#define PITCH_LOW			95	// other values are possible - 0-255, where 255 is very high
#define PITCH_HIGH			120

// volume values
#define VOL_NORM			1.0f

#endif
