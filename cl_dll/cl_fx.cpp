#include "cl_fx.h"

#include "cl_util.h"
#include "in_defs.h"
#include "studio.h"
#include "r_efx.h"

model_t* cl_sprite_ricochet = nullptr;

void LoadDefaultSprites()
{
	cl_sprite_ricochet = const_cast<model_t*>(gEngfuncs.GetSpritePointer(gEngfuncs.pfnSPR_Load("sprites/richo1.spr")));
}

void FX_Streaks(Vector pos, Vector dir, const StreakParams& streakParams, bool isDirectional)
{
	float maxLife = streakParams.maxLife;
	float minLife = streakParams.minLife;
	if (maxLife < minLife)
		maxLife = minLife;

	const Vector baseVel = isDirectional ? dir * streakParams.speed : Vector{};

	for( int i = 0; i < streakParams.count; i++ )
	{
		Vector vel = baseVel;
		vel.x += Com_RandomFloat( streakParams.velocityMin, streakParams.velocityMax );
		vel.y += Com_RandomFloat( streakParams.velocityMin, streakParams.velocityMax );
		vel.z += Com_RandomFloat( streakParams.velocityMin, streakParams.velocityMax );

		particle_t *p = gEngfuncs.pEfxAPI->R_TracerParticles( pos, vel, Com_RandomFloat( minLife, maxLife ));
		if( !p ) return;

		p->type = streakParams.particleType;
		p->color = streakParams.color;
		p->ramp = streakParams.length;
	}
}

void FX_RicochetSprite(Vector pos, model_t *pmodel, float duration, float scale)
{
	TEMPENTITY	*pTemp;

	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc((float*)pos, pmodel);
	if (!pTemp) return;

	pTemp->entity.curstate.rendermode = kRenderGlow;
	pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 200;
	pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
	pTemp->entity.curstate.scale = scale;
	pTemp->die = gEngfuncs.GetClientTime() + duration;
	pTemp->flags = FTENT_FADEOUT;
	pTemp->fadeSpeed = 8;

	pTemp->entity.curstate.frame = 0;
	pTemp->entity.angles[ROLL] = 45.0f * Com_RandomLong(0, 7);
}

void FX_SparkEffect(Vector pos, const SparkEffectParams& params)
{
	model_t* pmodel = params.sparkModelIndex > 0 ? gEngfuncs.pfnGetModelByIndex(params.sparkModelIndex) : cl_sprite_ricochet;
	if (pmodel)
	{
		float sparkDuration = params.sparkDuration > 0 ? params.sparkDuration : 0.1f;
		float scaleMin  = params.sparkScaleMin > 0 ? params.sparkScaleMin : 0.5f;
		float scaleMax  = params.sparkScaleMax > 0 ? params.sparkScaleMax : 1.0f;
		FX_RicochetSprite(pos, pmodel, sparkDuration, Com_RandomFloat(scaleMin, scaleMax));
		if (!(params.flags & SPARK_EFFECT_NO_STREAK))
			gEngfuncs.pEfxAPI->R_SparkStreaks((float*)pos, params.streakCount, -params.streakVelocity, params.streakVelocity);
	}
}

void FX_SparkShower(Vector pos, const SparkEffectParams& params)
{
	TEMPENTITY	*pTemp;

	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel((float*)pos);
	if( !pTemp ) return;

	pTemp->entity.baseline.origin[0] = Com_RandomFloat( -300.0f, 300.0f );
	pTemp->entity.baseline.origin[1] = Com_RandomFloat( -300.0f, 300.0f );
	pTemp->entity.baseline.origin[2] = Com_RandomFloat( -200.0f, 200.0f );

	pTemp->flags |= FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SPARKSHOWER;

	pTemp->entity.curstate.framerate = Com_RandomFloat( 0.5f, 1.5f );
	pTemp->entity.curstate.scale = gEngfuncs.GetClientTime();
	pTemp->die = gEngfuncs.GetClientTime() + 0.5f;
	pTemp->entity.curstate.iuser1 = params.sparkModelIndex;
	pTemp->entity.curstate.iuser2 = params.streakCount;
	pTemp->entity.curstate.iuser3 = params.streakVelocity;
	pTemp->entity.curstate.iuser4 = params.flags;
	pTemp->entity.curstate.fuser1 = params.sparkDuration;
	pTemp->entity.curstate.fuser2 = params.sparkScaleMin;
	pTemp->entity.curstate.fuser3 = params.sparkScaleMax;
}
