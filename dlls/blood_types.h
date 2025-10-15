#pragma once
#ifndef BLOOD_TYPES_H
#define BLOOD_TYPES_H

#define		BLOOD_COLOR_INVALID	-2
#define		DONT_BLEED			-1
#define		BLOOD_COLOR_RED		247
#define		BLOOD_COLOR_YELLOW	195
#define		BLOOD_COLOR_GREEN	BLOOD_COLOR_YELLOW

int BloodTypeFromName(const char* name);

#endif
