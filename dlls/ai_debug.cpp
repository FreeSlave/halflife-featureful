#include "ai_debug.h"

#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "cbase.h"
#include "basemonster.h"

#include <set>

void ReportAIStateByClassname(const char* name)
{
	if (!name || !*name) {
		ALERT(at_console, "Must provide a classname!\n");
		return;
	}
	CBaseEntity* pEntity = 0;
	ALERT(at_console, "Listing all monsters of \"%s\" classname\n", name);
	while ( ( pEntity = UTIL_FindEntityByClassname(pEntity, name) ) != 0 ) {
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (pMonster) {
			pMonster->ReportAIState(at_console);
			const bool clientInPVS = !FNullEnt(FIND_CLIENT_IN_PVS( pMonster->edict() ));
			ALERT(at_console, "Position in the world: (%3.1f, %3.1f, %3.1f). ", pMonster->pev->origin.x, pMonster->pev->origin.y, pMonster->pev->origin.z);
			ALERT(at_console, "Client in PVS: %s\n\n", clientInPVS ? "yes" : "no");
		}
	}
}

std::set<int> scheduleWatchers;

void AddScheduleWatcher(int entindex)
{
	scheduleWatchers.insert(entindex);
}

void RemoveScheduleWatcher(int entindex)
{
	scheduleWatchers.erase(entindex);
}

bool HasScheduleWatcher(int entindex)
{
	return scheduleWatchers.find(entindex) != scheduleWatchers.end();
}

void ResetScheduleWatchers()
{
	scheduleWatchers.clear();
}
