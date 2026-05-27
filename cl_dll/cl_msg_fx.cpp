#include "cl_util.h"
#include "parsemsg.h"

#include "r_efx.h"
#include "customentity.h"
#include "r_studioint.h"
#include "event_api.h"
#include "com_model.h"
#include "studio.h"
#include "pmtrace.h"
#include "cl_msg.h"
#include "cl_fx.h"
#include "clamp.h"
#include "util_shared.h"
#include "hl_palette.h"

#include "eventscripts.h"

#include "fx_flags.h"
#include "particleman.h"
#include "tex_materials.h"

extern "C"
{
int DLLEXPORT CL_IsThirdPerson();
}

extern engine_studio_api_t IEngineStudio;
extern cvar_t* cl_muzzlelight_monsters;

void GibHitCallback( TEMPENTITY* ent, pmtrace_t* pmtrace )
{
	static const char* redBloodDecals[] = {"{blood1", "{blood2", "{blood3", "{blood4", "{blood5", "{blood6"};
	static const char* yellowBloodDecals[] = {"{yblood1", "{yblood2", "{yblood3", "{yblood4", "{yblood5", "{yblood6"};

	const char* decalName = NULL;
	if (ent->entity.curstate.iuser1 == 1)
	{
		decalName = redBloodDecals[gEngfuncs.pfnRandomLong(0, 5)];
	}
	else if (ent->entity.curstate.iuser1 > 1)
	{
		decalName = yellowBloodDecals[gEngfuncs.pfnRandomLong(0, 5)];
	}

	if (ent->entity.curstate.onground)
	{
		ent->entity.baseline.origin = ent->entity.baseline.origin * 0.9;
		ent->entity.curstate.angles.x = 0;
		ent->entity.curstate.angles.z = 0;
		ent->entity.baseline.angles.x = 0;
		ent->entity.baseline.angles.z = 0;
	}
	else
	{
		ent->entity.curstate.origin = ent->entity.curstate.origin + Vector(0, 0, 8);

		if (ent->entity.curstate.iuser1 > 0 && ent->entity.curstate.iuser2 > 0 && decalName != NULL)
		{
			int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( (char*)decalName );
			int textureIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex( decalIndex );
			int traceEntIndex = gEngfuncs.pEventAPI->EV_IndexFromTrace( pmtrace );
			gEngfuncs.pEfxAPI->R_DecalShoot(textureIndex, traceEntIndex, 0, pmtrace->endpos, 0 );
			ent->entity.curstate.iuser2--;
		}
	}
}

int __MsgFunc_RandomGibs( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector absmin;
	Vector size;
	Vector direction;

	absmin = READ_VECTOR();
	size = READ_VECTOR();
	direction = READ_VECTOR();

	float randomization = READ_BYTE() / 100.0f;
	int modelIndex = READ_SHORT();
	int gibCount = READ_BYTE();
	int lifeTime = READ_BYTE();
	int bloodType = READ_BYTE();
	int gibBodiesNum = READ_BYTE();
	int startGibIndex = READ_BYTE();

	float velocityMultiplier = READ_BYTE() / 10.0f;

	struct model_s* model = IEngineStudio.GetModelByIndex(modelIndex);

	if (gibBodiesNum == 0)
	{
		studiohdr_t* pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(model);
		if (pstudiohdr)
		{
			mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)pstudiohdr + pstudiohdr->bodypartindex );

			gibBodiesNum = 1;
			for (int j=0; j<pstudiohdr->numbodyparts; ++j)
			{
				gibBodiesNum = gibBodiesNum * pbodypart[j].nummodels;
			}
		}
	}

	if (gibBodiesNum == 0)
		gibBodiesNum = startGibIndex + 1;
	startGibIndex = startGibIndex > gibBodiesNum - 1 ? gibBodiesNum - 1 : startGibIndex;

	const float clientTime = gEngfuncs.GetClientTime();

	for (int i=0; i<gibCount; ++i)
	{
		Vector gibPos;
		gibPos.x = absmin.x + size.x * gEngfuncs.pfnRandomFloat(0, 1);
		gibPos.y = absmin.y + size.y * gEngfuncs.pfnRandomFloat(0, 1);
		gibPos.z = absmin.z + size.z * gEngfuncs.pfnRandomFloat(0, 1) + 1;

		Vector gibVelocity = direction;
		gibVelocity.x += gEngfuncs.pfnRandomFloat(-randomization, randomization);
		gibVelocity.y += gEngfuncs.pfnRandomFloat(-randomization, randomization);
		gibVelocity.z += gEngfuncs.pfnRandomFloat(-randomization, randomization);

		gibVelocity = gibVelocity * gEngfuncs.pfnRandomFloat( 300, 400 ) * velocityMultiplier;

		gibVelocity.ClampToLengthInPlace(1500);

		TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(gibPos, model);
		if (!pTemp)
			break;

		pTemp->entity.curstate.body = gEngfuncs.pfnRandomLong(startGibIndex, gibBodiesNum - 1);
		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_GRAVITY | FTENT_ROTATE | FTENT_PERSIST;

		pTemp->entity.curstate.iuser1 = bloodType;
		pTemp->entity.curstate.iuser2 = 5;
		pTemp->entity.curstate.solid = SOLID_SLIDEBOX;
		pTemp->entity.curstate.movetype = MOVETYPE_BOUNCE;
		pTemp->entity.curstate.friction = 0.55;
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 255;
		pTemp->hitcallback = &GibHitCallback;

		pTemp->entity.baseline.angles.x = gEngfuncs.pfnRandomFloat(-256, 255);
		pTemp->entity.baseline.angles.z = gEngfuncs.pfnRandomFloat(-256, 255);
		pTemp->entity.baseline.origin = gibVelocity;
		pTemp->die = clientTime + lifeTime;
	}

	return 1;
}

int __MsgFunc_MuzzleLight( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	Vector vecSrc = READ_VECTOR();

	if (cl_muzzlelight_monsters && cl_muzzlelight_monsters->value)
	{
		dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
		dl->origin[0] = vecSrc.x;
		dl->origin[1] = vecSrc.y;
		dl->origin[2] = vecSrc.z;
		dl->radius = 144;
		dl->color.r = 255;
		dl->color.g = 208;
		dl->color.b = 128;
		dl->die = gEngfuncs.GetClientTime() + 0.01f;
	}

	return 1;
}

int __MsgFunc_CustomBeam( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int beamType = READ_BYTE();

	Vector	start, end;
	int	modelIndex, startFrame;
	float	frameRate, life, width;
	int	startEnt, endEnt;
	float	noise, speed;
	float	r, g, b, a;
	BEAM* beam;
	int flags;

	switch (beamType) {
	case TE_BEAMPOINTS:
	case TE_BEAMENTPOINT:
	case TE_BEAMENTS:
	case TE_BEAMRING:
	{
		if( beamType == TE_BEAMENTS || beamType == TE_BEAMRING )
		{
			startEnt = READ_SHORT();
			endEnt = READ_SHORT();
		}
		else
		{
			if( beamType == TE_BEAMENTPOINT )
			{
				startEnt = READ_SHORT();
			}
			else
			{
				start = READ_VECTOR();
			}
			end = READ_VECTOR();
		}
		modelIndex = READ_SHORT();
		startFrame = READ_BYTE();
		frameRate = (float)READ_BYTE() * 0.1f;
		life = (float)READ_BYTE() * 0.1f;
		width = (float)READ_BYTE() * 0.1f;
		noise = (float)READ_BYTE() * 0.01f;
		r = (float)READ_BYTE() / 255.0f;
		g = (float)READ_BYTE() / 255.0f;
		b = (float)READ_BYTE() / 255.0f;
		a = (float)READ_BYTE() / 255.0f;
		speed = (float)READ_BYTE() * 0.1f;
		flags = READ_BYTE();
		if ( beamType == TE_BEAMRING )
			beam = gEngfuncs.pEfxAPI->R_BeamRing( startEnt, endEnt, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		else if( beamType == TE_BEAMENTS )
			beam = gEngfuncs.pEfxAPI->R_BeamEnts( startEnt, endEnt, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		else if( beamType == TE_BEAMENTPOINT )
			beam = gEngfuncs.pEfxAPI->R_BeamEntPoint( startEnt, end, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		else
			beam = gEngfuncs.pEfxAPI->R_BeamPoints( start, end, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		if (beam)
		{
			if (flags & BEAM_FSINE)
				beam->flags |= FBEAM_SINENOISE;
			if (flags & BEAM_FSOLID)
				beam->flags |= FBEAM_SOLID;
			if (flags & BEAM_FSHADEIN)
				beam->flags |= FBEAM_SHADEIN;
			if (flags & BEAM_FSHADEOUT)
				beam->flags |= FBEAM_SHADEOUT;
		}
	}
		break;
	default:
		gEngfuncs.Con_DPrintf("%s: got the unknown beam type %d\n", pszName, beamType);
		break;
	}

	return 1;
}

void FX_TempSprite(Vector pos, int modelIndex, float scale, int rendermode, color24 color, int a, int renderfx, float framerate, float life, const Vector& velocity, float fadeTime)
{
	model_t *pmodel;
	if(( pmodel = gEngfuncs.pfnGetModelByIndex( modelIndex )) == NULL )
		return;

	TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( pos, pmodel );
	if( !pTemp ) return;

	const float clientTime = gEngfuncs.GetClientTime();

	pTemp->frameMax = pmodel->numframes - 1;

	pTemp->entity.curstate.rendermode = rendermode;
	pTemp->entity.baseline.renderamt = pTemp->entity.curstate.renderamt = a;
	pTemp->entity.curstate.renderfx = renderfx;
	pTemp->entity.curstate.rendercolor = color;
	pTemp->entity.curstate.scale = scale;
	pTemp->entity.curstate.framerate = framerate;
	pTemp->entity.baseline.origin = velocity;

	if (fadeTime > 0.0f)
	{
		pTemp->flags |= FTENT_FADEOUT;
		pTemp->fadeSpeed = 1.0f / fadeTime;
	}

	if (framerate > 0 && pmodel->numframes > 1)
	{
		pTemp->flags |= FTENT_SPRANIMATE;
	}

	if (life)
	{
		if (pTemp->flags & FTENT_SPRANIMATE)
			pTemp->flags |= FTENT_SPRANIMATELOOP;
		pTemp->die = clientTime + life;
	}
	else if (framerate > 0)
	{
		pTemp->die = clientTime + (pTemp->frameMax / framerate);
	}
	else
	{
		pTemp->die = clientTime + 0.1f;
	}
}

int __MsgFunc_Sprite( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos = READ_VECTOR();
	int modelIndex = READ_SHORT();
	float scale = READ_BYTE() * 0.1f;
	int rendermode = READ_BYTE();
	color24 color = READ_COLOR();
	int a = READ_BYTE();
	int renderfx = READ_BYTE();
	float framerate = READ_SHORT() * 0.1f;
	float life = READ_BYTE() * 0.1f;
	Vector velocity = READ_VECTOR();
	float fadeTime = READ_BYTE() * 0.1f;

	FX_TempSprite(pos, modelIndex, scale, rendermode, color, a, renderfx, framerate, life, velocity, fadeTime);

	return 1;
}

void FX_Sprite_Trail( Vector start, Vector end, int modelIndex, int count, float life, float scale, float amp,
					  float speed, int rendermode, color24 color, int renderamt, int renderfx = kRenderFxNone, float extraLifeMax = 0.0f )
{
	Vector delta, dir;
	model_t *pmodel;

	if(( pmodel = gEngfuncs.pfnGetModelByIndex( modelIndex )) == NULL )
		return;

	delta = end - start;
	dir = delta.Normalize();

	amp /= 256.0f;

	const float clientTime = gEngfuncs.GetClientTime();

	for( int i = 0; i < count; i++ )
	{
		Vector pos, vel;

		if( i == 0 )
			pos = start;
		else
			VectorMA( start, ( i / ( count - 1.0f )), delta, pos );

		TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( pos, pmodel );
		if( !pTemp ) return;

		pTemp->flags = (FTENT_COLLIDEWORLD|FTENT_FADEOUT|FTENT_SLOWGRAVITY);
		pTemp->frameMax = pmodel->numframes - 1;
		if (pmodel->numframes > 1)
			pTemp->flags |= FTENT_SPRCYCLE;

		VectorScale( dir, speed, vel );
		vel[0] += Com_RandomFloat( -127.0f, 128.0f ) * amp;
		vel[1] += Com_RandomFloat( -127.0f, 128.0f ) * amp;
		vel[2] += Com_RandomFloat( -127.0f, 128.0f ) * amp;
		VectorCopy( vel, pTemp->entity.baseline.origin );
		VectorCopy( pos, pTemp->entity.origin );

		pTemp->entity.curstate.scale = scale;
		pTemp->entity.curstate.rendermode = rendermode;
		pTemp->entity.curstate.renderfx = renderfx;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = renderamt;
		pTemp->entity.curstate.rendercolor = color;

		if (pmodel->numframes > 1)
			pTemp->entity.curstate.frame = Com_RandomLong( 0, pmodel->numframes - 1 );
		pTemp->die = clientTime + life;
		if (extraLifeMax)
			pTemp->die += Com_RandomFloat( 0.0f, extraLifeMax );
	}
}

int __MsgFunc_SpriteTrail( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos = READ_VECTOR();
	Vector pos2 = READ_VECTOR();
	int modelIndex = READ_SHORT();
	int count = READ_BYTE();
	float life = (float)READ_BYTE() * 0.1f;
	float scale = (float)READ_BYTE();
	if( !scale )
		scale = 1.0f;
	else
		scale *= 0.1f;
	float vel = (float)READ_BYTE() * 10;
	float random = (float)READ_BYTE() * 10;
	int rendermode = READ_BYTE();
	color24 color = READ_COLOR();
	int a = READ_BYTE();
	int renderfx = READ_BYTE();
	float extraLifeMax = READ_BYTE() * 0.1f;
	FX_Sprite_Trail( pos, pos2, modelIndex, count, life, scale, random, vel, rendermode, color, a, renderfx, extraLifeMax );

	return 1;
}

int __MsgFunc_Spray( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos = READ_VECTOR();
	Vector dir = READ_VECTOR();
	int modelIndex = READ_SHORT();
	int count = READ_BYTE();
	int speed = READ_BYTE();
	int spread = READ_BYTE();
	int rendermode = READ_BYTE();
	color24 color = READ_COLOR();
	int a = READ_BYTE();
	int renderfx = READ_BYTE();
	float scale = (float)READ_BYTE();
	if( !scale )
		scale = 1.0f;
	else
		scale *= 0.1f;
	float framerate = READ_SHORT() * 0.1f;
	int flags = READ_BYTE();

	FX_Spray(pos, dir, modelIndex, count, speed, spread / 100.0f, rendermode, color, a, renderfx, scale, framerate, flags, FloatRange{});

	return 1;
}

int __MsgFunc_Streaks( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	StreakParams streakParams;

	bool isDirectional = READ_BYTE() ? true : false;
	Vector pos = READ_VECTOR();
	Vector dir;
	if (isDirectional)
		dir = READ_VECTOR();
	streakParams.color = READ_BYTE();
	streakParams.count = READ_SHORT();
	streakParams.speed = READ_SHORT();
	streakParams.velocityMax = READ_SHORT();
	streakParams.velocityMin = -streakParams.velocityMax;
	streakParams.minLife = READ_BYTE() * 0.1f;
	streakParams.maxLife = READ_BYTE() * 0.1f;
	streakParams.particleType = (ptype_t)READ_BYTE();
	streakParams.length = READ_BYTE() * 0.1f;

	FX_Streaks(pos, dir, streakParams, isDirectional);

	return 1;
}

void ExpandCallback(TEMPENTITY *ent, float frametime, float currenttime)
{
	const float minScale = 0.0001;
	const float timeCreated = ent->entity.curstate.fuser1;
	const float originalScale = ent->entity.curstate.fuser2;
	const float scaleSpeed = ent->entity.curstate.fuser3;
	const bool fade = ent->entity.curstate.iuser1 != 0;

	if (fade)
	{
		const int originalRenderamt = ent->entity.curstate.iuser2;
		ent->entity.curstate.renderamt = (ent->frameMax - ent->entity.curstate.frame)/ent->frameMax * originalRenderamt;
		if (ent->entity.curstate.renderamt == 0)
		{
			ent->die = currenttime;
		}
	}

	if (scaleSpeed)
	{
		ent->entity.curstate.scale = scaleSpeed * originalScale * (currenttime - timeCreated) + originalScale;
		if (ent->entity.curstate.scale < minScale)
		{
			ent->entity.curstate.scale = minScale;
			ent->die = currenttime;
		}
	}
}

void FX_Smoke(TEMPENTITY* pTemp, float scale, float speed, float zOffset, int rendermode, int renderamt, color24 color)
{
	pTemp->entity.curstate.rendermode = rendermode;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.baseline.origin[2] = speed;
	pTemp->entity.curstate.rendercolor = color;
	pTemp->entity.curstate.renderamt = renderamt;
	pTemp->entity.origin[2] += zOffset;
	pTemp->entity.curstate.scale = scale;
}

int __MsgFunc_Smoke( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos, dir;
	int modelIndex;
	float scale, frameRate, speed, zOffset, scaleSpeed;
	int rendermode, renderamt;

	const int flags = READ_BYTE();
	const bool directed = (flags & SMOKER_FLAG_DIRECTED) != 0;
	const bool fade = (flags & SMOKER_FLAG_FADE_SPRITE) != 0;
	pos = READ_VECTOR();
	modelIndex = READ_SHORT();

	const float scaleVal = READ_COORD();
	scale = (flags & SMOKER_FLAG_SCALE_VALUE_IS_NORMAL) ? scaleVal : (float)(scaleVal * 0.1f);

	frameRate = READ_BYTE();
	speed = READ_SHORT();
	zOffset = READ_SHORT();
	rendermode = READ_BYTE();
	renderamt = READ_BYTE();
	color24 color = READ_COLOR();
	scaleSpeed = READ_SHORT() / 10.0f;

	if (directed)
	{
		dir = READ_VECTOR();
	}

	// Original hard-coded TE_SMOKE values
	if (speed == 0.0f && zOffset == 0.0f)
	{
		speed = 30;
		zOffset = 20;
	}
	if (renderamt == 0)
		renderamt = 255;
	if (rendermode == 0)
		rendermode = kRenderTransAlpha;
	if (color.r + color.g + color.b == 0)
		color.r = color.g = color.b = Com_RandomLong( 20, 35 );

	TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->R_DefaultSprite( pos, modelIndex, frameRate );

	if (pTemp)
	{
		FX_Smoke(pTemp, scale, speed, zOffset, rendermode, renderamt, color);
		if (directed)
		{
			pTemp->entity.baseline.origin = dir * speed;
		}
		if (scaleSpeed != 0 || fade)
		{
			pTemp->entity.curstate.fuser1 = gEngfuncs.GetClientTime();
			pTemp->entity.curstate.fuser2 = scale;
			pTemp->entity.curstate.fuser3 = scaleSpeed;
			pTemp->entity.curstate.iuser1 = 1;
			pTemp->entity.curstate.iuser2 = renderamt;
			pTemp->flags |= FTENT_CLIENTCUSTOM;
			pTemp->callback = &ExpandCallback;
		}
	}

	return 1;
}

int __MsgFunc_SparkShower( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos = READ_VECTOR();

	SparkEffectParams params;
	params.sparkModelIndex = READ_SHORT();
	params.streakCount = READ_SHORT();
	params.streakVelocity = READ_SHORT();
	params.sparkDuration = READ_SHORT() * 0.01f;
	params.sparkScaleMin = READ_SHORT() * 0.01f;
	params.sparkScaleMax = READ_SHORT() * 0.01f;
	params.flags = READ_SHORT();

	FX_SparkShower(pos, params);

	return 1;
}

extern int GetBloodSplatterStyle();
extern bool ShouldSpawnBloodStream(int damageAmount);

static float BloodSpriteAmount(int amount)
{
	return clamp(amount / 10.0f, 3.0f, 16.0f);
}

enum BloodSplatterType
{
	BLOODSPLATTER_SPRITE = 0,
	BLOODSPLATTER_LEGACY = 1,
	BLOODSPLATTER_QUAKE = 2,
	BLOODSPLATTER_QUAKE2 = 3,
};

int __MsgFunc_Blood( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int entindex = READ_SHORT();
	int params = READ_BYTE();
	const Vector pos = READ_VECTOR();
	const Vector dir = READ_VECTOR();
	const int bloodSpraySpriteIndex = READ_SHORT();
	const int bloodSplatterSpriteIndex = READ_SHORT();

	const int color = READ_BYTE();
	const int amount = READ_SHORT();

	const bool isFirstPerson = EV_IsLocal(entindex) && !CL_IsThirdPerson();

	int style = GetBloodSplatterStyle();

	bool doSplatter = true;
	bool doStream = ShouldSpawnBloodStream(amount);

	if (params == BLOOD_FORCED_TYPE_ONLYDRIPS)
	{
		style = BLOODSPLATTER_SPRITE;
		doStream = false;
	}
	else if (params == BLOOD_FORCED_TYPE_ONLYSTREAM)
	{
		doSplatter = false;
		doStream = true;
	}

	if (doSplatter)
	{
		int variance = 3;
		switch (style) {
		case BLOODSPLATTER_LEGACY:
		case BLOODSPLATTER_QUAKE:
		case BLOODSPLATTER_QUAKE2:
			if (color < 240)
				variance = 6;
			break;
		default:
			break;
		}
		const IntRange colorRange = GetRangeForColorIndex(color, variance);

		if (style == BLOODSPLATTER_LEGACY)
		{
			FX_BloodLegacy(pos + dir.Normalize() * 4.0f, dir, colorRange, clamp(amount, 5, 150));
		}
		else if (style == BLOODSPLATTER_QUAKE)
		{
			if (isFirstPerson)
				FX_BloodParticles(pos, colorRange, (int)BloodSpriteAmount(amount));
			else
				FX_QuakeParticles(pos, dir, colorRange, clamp(amount * 2, 10, 250));
		}
		else if (style == BLOODSPLATTER_QUAKE2)
		{
			FX_DotParticles(pos, dir, colorRange, clamp(amount, 40, 60));
		}
		else
		{
			const int bloodSprayColorIndex = colorRange.IsProperRange() ? RandomizeNumberFromRange(colorRange.min + 1, colorRange.max) : colorRange.min;
			const int bloodSplatterColorIndex = colorRange.IsProperRange() ? bloodSprayColorIndex - 1 : bloodSprayColorIndex;

			const float spriteAmount = BloodSpriteAmount(amount);

			FX_BloodSplatter(pos, bloodSplatterColorIndex, bloodSplatterSpriteIndex, (int)spriteAmount);
			FX_BloodSpray(pos, bloodSprayColorIndex, bloodSpraySpriteIndex, spriteAmount);
		}
	}

	if (doStream)
	{
		const Vector streamVector = Vector(Com_RandomFloat(-1.0f, 1.0f), Com_RandomFloat(-1.0f, 1.0f), Com_RandomFloat(0.0f, 2.0f));
		const IntRange streamColorRange = GetRangeForColorIndex(color, 6);

		const int streamAmount = params == BLOOD_FORCED_TYPE_ONLYSTREAM ? Q_min(amount, 255) : (clamp(amount, 60, 90) + Com_RandomLong(0, 20));

		FX_BloodStream(pos - dir.Normalize() * 8.0f, streamVector, streamColorRange, streamAmount);
	}

	return 1;
}

int __MsgFunc_Gunshot( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos = READ_VECTOR();
	Vector dir = READ_VECTOR();
	int entIndex = READ_SHORT();
	int decalIndex = READ_SHORT();
	char cTextureType = READ_BYTE();

	const MaterialData* mData = g_MaterialRegistry.GetMaterialDataWithFallback(cTextureType);
	int impactParticleColorIndex = g_MaterialRegistry.GetDefaultImpactParticleColorIndex();
	if (mData && mData->hit.impactParticleColorIndex.has_value())
	{
		impactParticleColorIndex = *mData->hit.impactParticleColorIndex;
	}

	FX_GunshotDecal(pos, dir, decalIndex, entIndex, impactParticleColorIndex);

	return 1;
}

int __MsgFunc_Particle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const Vector position = READ_VECTOR();
	const Vector velocity = READ_VECTOR();

	const int renderMode = READ_BYTE();
	const float particleLife = READ_BYTE() / 10.0f;
	const float particleSize = READ_SHORT() / 10.0f;
	const int red = READ_BYTE();
	const int green = READ_BYTE();
	const int blue = READ_BYTE();
	const int brightness = READ_BYTE();
	const float gravity = READ_BYTE() / 100.0f;
	const float fadeSpeed = READ_BYTE() / 10.0f;
	const float scaleSpeed = READ_BYTE() / 10.0f;
	const int frameRate = READ_BYTE();
	const int flags = READ_BYTE();
	const int modelIndex = READ_SHORT();

	if (g_pParticleMan)
	{
		const float clTime = gEngfuncs.GetClientTime();

		model_t* sprite = gEngfuncs.pfnGetModelByIndex(modelIndex);
		if (sprite)
		{
			CBaseParticle *particle = g_pParticleMan->CreateParticle(position, Vector(0.0f, 0.0f, 0.0f), sprite, particleSize > 0 ? particleSize : 16, brightness, "particle");
			if (particle)
			{
				particle->SetLightFlag(LIGHT_NONE);
				particle->SetCullFlag(CULL_PVS);
				particle->SetRenderFlag(RENDER_FACEPLAYER);

				particle->m_vVelocity = velocity;
				particle->m_iRendermode = renderMode;
				particle->m_vColor = Vector(red, green, blue);
				particle->m_flGravity = gravity;
				if (fadeSpeed > 0)
					particle->m_flFadeSpeed = fadeSpeed;
				particle->m_flScaleSpeed = scaleSpeed;
				if (flags & SF_PARTICLESHOOTER_ANIMATED)
				{
					particle->m_iFramerate = frameRate > 0 ? frameRate : 10;
					particle->m_iNumFrames = sprite->numframes;
				}

				if (flags & SF_PARTICLESHOOTER_SPIRAL)
					particle->SetCollisionFlags(TRI_SPIRAL);
				if (flags & SF_PARTICLESHOOTER_COLLIDE_WITH_WORLD)
					particle->SetCollisionFlags(TRI_COLLIDEWORLD);
				if (flags & SF_PARTICLESHOOTER_AFFECTED_BY_FORCE)
					particle->m_bAffectedByForce = true;
				if (flags & SF_PARTICLESHOOTER_KILLED_ON_COLLIDE)
					particle->SetCollisionFlags(TRI_COLLIDEKILL);

				particle->m_flDieTime = clTime + particleLife;
			}
		}
	}

	return 1;
}

int __MsgFunc_Q2Particles( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const int count = READ_SHORT();
	const Vector pos = READ_VECTOR();
	const Vector dir = READ_VECTOR();
	const color24 color = READ_COLOR();

	FX_DotParticles(pos, dir, ColorRandomizer(color.r, color.g, color.b), count);

	return 1;
}

int __MsgFunc_BreakModel( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const Vector pos = READ_VECTOR();
	const Vector size = READ_VECTOR();
	const Vector dir = READ_VECTOR();
	const float random = READ_BYTE() * 10.0f;
	const short modelIndex = READ_SHORT();
	const int count = READ_BYTE();
	const float life = READ_BYTE() * 0.1f;
	const char flags = READ_BYTE();
	const float scale = READ_SHORT() / 100.0f;

	FX_BreakModel(pos, size, dir, random, life, count, modelIndex, flags, scale);
	return 1;
}

void HookFXMessages()
{
	HOOK_MESSAGE( RandomGibs );
	HOOK_MESSAGE( MuzzleLight );
	HOOK_MESSAGE( CustomBeam );
	HOOK_MESSAGE( Sprite );
	HOOK_MESSAGE( SpriteTrail );
	HOOK_MESSAGE( Spray );
	HOOK_MESSAGE( Streaks );
	HOOK_MESSAGE( Smoke );
	HOOK_MESSAGE( SparkShower );
	HOOK_MESSAGE( Blood );
	HOOK_MESSAGE( Gunshot );
	HOOK_MESSAGE( Particle );
	HOOK_MESSAGE( Q2Particles );
	HOOK_MESSAGE( BreakModel );
}
