#pragma once
#ifndef FILE_UTILS_H
#define FILE_UTILS_H

char* ReadFileContents(const char* fileName, int& fileSize);
void FreeFileContents(char* pMemFile);

#endif
