#include "bucket_preferences.h"

#include "logger.h"
#include "parsetext.h"

void ParseBucketPreferences(BucketPreferenceSet& preferences, const char* pfile, int fileSize, const char* fileName)
{
	int weaponCount = 0;
	int i = 0;
	while ( i<fileSize )
	{
		if (IsSpaceCharacter(pfile[i]))
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, fileSize);
		}
		else
		{
			BucketPreference& preference = preferences.list[weaponCount];
			const int weaponNameStart = i;
			ConsumeNonSpaceCharacters(pfile, i, fileSize);
			const int weaponNameLength = i - weaponNameStart;
			if (weaponNameLength > 0 && weaponNameLength < MAX_WEAPON_NAME)
			{
				if (weaponCount >= MAX_WEAPONS)
				{
					LOG_WARNING("Too many entries in %s. Max is %d\n", fileName, MAX_WEAPONS);
					break;
				}
				else
				{
					strncpy(preference.szName, pfile + weaponNameStart, weaponNameLength);
					preference.szName[weaponNameLength] = '\0';

					if (SkipSpacesAndTabs(pfile, i, fileSize))
					{
						if (pfile[i] >= '0' && pfile[i] <= '0' + WEAPON_SLOTS_HARDLIMIT)
						{
							const int slotNumber = pfile[i] - '0';
							preference.iPreferredSlot = slotNumber;
							++i;

							if (SkipSpacesAndTabs(pfile, i, fileSize))
							{
								if (pfile[i] >= '0' && pfile[i] <= '0' + WEAPON_SLOTS_HARDLIMIT)
								{
									const int slotPosNumber = pfile[i] - '0';
									preference.iPreferredSlotPos = slotPosNumber;
									++i;
								}
								else
								{
									LOG_WARNING("Bad position in slot value for %s in %s\n", preference.szName, fileName);
								}
							}
						}
						else
						{
							LOG_WARNING("Bad slot value for %s in %s\n", preference.szName, fileName);
						}
					}
					weaponCount++;
				}
			}
			else
			{
				LOG_WARNING("Bad weapon name length in %s\n", fileName);
			}
			ConsumeLine(pfile, i, fileSize);
		}
	}
}
