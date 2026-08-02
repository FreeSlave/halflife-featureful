#include "file_utils.h"

#if CLIENT_DLL
#include "cl_dll.h"
#elif SERVER_DLL
#include "extdll.h"
#include "enginecallback.h"
#else
#include <cstdio>
#include <cstdlib>
#endif

char* ReadFileContents(const char* fileName, int& fileSize)
{
	char *pMemFile = nullptr;

#if CLIENT_DLL
	pMemFile = (char*)gEngfuncs.COM_LoadFile( fileName, 5, &fileSize );
#elif SERVER_DLL
	pMemFile = (char*)g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
#else
	FILE* f = fopen(fileName, "r");
	if (f) {
		fseek(f, 0, SEEK_END);
		fileSize = (int)ftell(f);
		fseek(f, 0, SEEK_SET);
		pMemFile = (char*)malloc(fileSize);
		if (pMemFile) {
			fread(pMemFile, 1, fileSize, f);
		}
		fclose(f);
	}
#endif
	return pMemFile;
}

void FreeFileContents(char* pMemFile)
{
#if CLIENT_DLL
	gEngfuncs.COM_FreeFile(pMemFile);
#elif SERVER_DLL
	g_engfuncs.pfnFreeFile(pMemFile);
#else
	free(pMemFile);
#endif
}
