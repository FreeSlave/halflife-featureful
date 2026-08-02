#pragma once
#ifndef BUCKET_PREFERENCES_H
#define BUCKET_PREFERENCES_H

#include "common_limits.h"

struct BucketPreference
{
	char szName[MAX_WEAPON_NAME] = {0};
	int iPreferredSlot{0};
	int iPreferredSlotPos{0};
};

struct BucketPreferenceSet
{
	BucketPreference list[MAX_WEAPONS];
};

void ParseBucketPreferences(BucketPreferenceSet& preferences, const char* pfile, int fileSize, const char* fileName);

#endif
