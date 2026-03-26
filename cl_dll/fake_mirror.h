#pragma once
#ifndef FAKE_MIRROR_H
#define FAKE_MIRROR_H

#include "vector.h"

struct FakeMirror
{
	Vector origin{};
	float radius{0};
	int type{0};
	bool enabled{false};
};

#endif
