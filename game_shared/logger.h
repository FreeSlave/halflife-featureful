#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#if CLIENT_DLL
#include "cl_dll.h"
#define LOG gEngfuncs.Con_DPrintf
#define LOG_DEV gEngfuncs.Con_DPrintf
#define LOG_WARNING gEngfuncs.Con_DPrintf
#define LOG_ERROR gEngfuncs.Con_Printf
#elif SERVER_DLL
#include "extdll.h"
#include "enginecallback.h"
#define LOG(...) ALERT(at_console, ##__VA_ARGS__)
#define LOG_DEV(...) ALERT(at_aiconsole, ##__VA_ARGS__)
#define LOG_WARNING(...) ALERT(at_warning, ##__VA_ARGS__)
#define LOG_ERROR(...) ALERT(at_error, ##__VA_ARGS__)
#else
#include <cstdio>
#define LOG printf
#define LOG_DEV printf
#define LOG_WARNING(...) fprintf(stderr, ##__VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, ##__VA_ARGS__)
#endif

#endif
