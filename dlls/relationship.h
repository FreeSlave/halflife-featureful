#pragma once
#ifndef RELATIONSHIP_H
#define RELATIONSHIP_H

// monster to monster relationship types
#define R_AL	-2 // (ALLY) pals. Good alternative to R_NO when applicable.
#define R_FR	-1// (FEAR)will run
#define	R_NO	0// (NO RELATIONSHIP) disregard
#define R_DL	1// (DISLIKE) will attack
#define R_HT	2// (HATE)will attack this character instead of any visible DISLIKEd characters
#define R_NM	3// (NEMESIS)  A monster Will ALWAYS attack its nemsis, no matter what

#endif
