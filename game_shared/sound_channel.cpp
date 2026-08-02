#include "sound_channel.h"
#include "const_sound.h"

#include <cstring>
#include <utility>

bool ParseSoundChannel(const char* str, int& channel)
{
	constexpr std::pair<const char*, int> channels[] = {
		{"auto", CHAN_AUTO},
		{"weapon", CHAN_WEAPON},
		{"voice", CHAN_VOICE},
		{"item", CHAN_ITEM},
		{"body", CHAN_BODY},
		{"static", CHAN_STATIC},
	};

	for (auto& p : channels)
	{
		if (stricmp(str, p.first) == 0)
		{
			channel = p.second;
			return true;
		}
	}
	return false;
}
