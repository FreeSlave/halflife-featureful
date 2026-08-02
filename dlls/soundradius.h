#pragma once
#ifndef SOUNDRADIUS_H
#define SOUNDRADIUS_H

enum
{
	SOUND_RADIUS_SMALL = -2,
	SOUND_RADIUS_MEDIUM = -1,
	SOUND_RADIUS_LARGE = 0,
	SOUND_RADIUS_HUGE,
	SOUND_RADIUS_ENORMOUS,
	SOUND_RADIUS_EVERYWHERE,
};

float SoundAttenuation(short soundRadius);

#endif
