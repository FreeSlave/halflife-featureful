#pragma once
#ifndef AI_DEBUG_H
#define AI_DEBUG_H

void ReportAIStateByClassname(const char* name);

void AddScheduleWatcher(int entindex);
void RemoveScheduleWatcher(int entindex);
bool HasScheduleWatcher(int entindex);
void ResetScheduleWatchers();

#endif
