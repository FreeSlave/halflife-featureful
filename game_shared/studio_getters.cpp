#include "studio_getters.h"
#include "common_types.h"
#include "vector.h"
#include "studio.h"

int GetOverallBodyNum(void *pmodel)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if( !pstudiohdr )
		return 0;

	int bodiesNum = 1;
	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)pstudiohdr + pstudiohdr->bodypartindex );

	for (int j=0; j<pstudiohdr->numbodyparts; ++j)
	{
		bodiesNum = bodiesNum * pbodypart[j].nummodels;
	}
	return bodiesNum;
}

int GetBodygroupNumModels(void *pmodel, int iGroup)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if( !pstudiohdr )
		return 0;

	if( iGroup > pstudiohdr->numbodyparts )
		return 0;

	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)pstudiohdr + pstudiohdr->bodypartindex ) + iGroup;
	return pbodypart->nummodels;
}
