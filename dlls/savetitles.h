#pragma once
#ifndef SAVETITLES_H
#define SAVETITLES_H

const char* GetSaveTitleForMap(const char* mapname);

void ReadSaveTitles(unsigned char *pMemFile, int fileSize, const char* fileName);

#endif
