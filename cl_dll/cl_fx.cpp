#include "cl_fx.h"

#include "cl_util.h"
#include "in_defs.h"
#include "studio.h"
#include "color_utils.h"
#include "fx_flags.h"
#include "r_efx.h"
#include "clamp.h"
#include "util_shared.h"
#include "hl_palette.h"
#include "particleman.h"

model_t* cl_sprite_ricochet = nullptr;
model_t* cl_sprite_dot = nullptr;
model_t* cl_sprite_dot_index = nullptr;

extern Vector v_origin;
extern Vector g_viewPlaneNormal;

#define RGBPAL565(p,i)    ( ( ( (short) *(((p)+((i)*3)) + 2) << 8 ) & 0xf800 ) | ( ( (short) *(((p)+((i)*3)) + 1) << 3 ) & 0x07e0 ) | ( (short) *(((p)+((i)*3)) + 0) >> 3 ) )
#define RGBPAL555(p,i)    ( ( ( (short) *(((p)+((i)*3)) + 2) << 7 ) & 0x7c00 ) | ( ( (short) *(((p)+((i)*3)) + 1) << 2 ) & 0x03e0 ) | ( (short) *(((p)+((i)*3)) + 0) >> 3 ) )

short GetPackedColor(int colorIndex)
{
	colorIndex = clamp(colorIndex, 0, 255);
	return RGBPAL565(hlPalette, colorIndex);
}

static particle_t* AllocColoredParticle(int colorIndex)
{
	particle_t* p = gEngfuncs.pEfxAPI->R_AllocParticle(nullptr);
	p->color = colorIndex;
	p->packedColor = GetPackedColor(colorIndex);
	return p;
}

void LoadDefaultSprites()
{
	cl_sprite_ricochet = const_cast<model_t*>(gEngfuncs.GetSpritePointer(gEngfuncs.pfnSPR_Load("sprites/richo1.spr")));
	cl_sprite_dot = const_cast<model_t*>(gEngfuncs.GetSpritePointer(gEngfuncs.pfnSPR_Load("sprites/dot.spr")));

	{
		const char* const dotIndexPath = "sprites/dot_index.spr";
		int fileSize;
		auto pfile = gEngfuncs.COM_LoadFile(dotIndexPath, 5, &fileSize);
		if (pfile)
		{
			cl_sprite_dot_index = const_cast<model_t*>(gEngfuncs.GetSpritePointer(gEngfuncs.pfnSPR_Load(dotIndexPath)));
			gEngfuncs.COM_FreeFile(pfile);
		}
		else
		{
			cl_sprite_dot_index = nullptr;
		}
	}
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

void FX_Spray(Vector pos, Vector dir, int modelIndex, int count, int speed, float noise, int rendermode, color24 color, int renderamt, int renderfx, float scale, float framerate, int flags)
{
	model_t	*pmodel = gEngfuncs.pfnGetModelByIndex(modelIndex);
	if (!pmodel)
		return;

	float znoise = Q_min( 1.0f, noise * 1.5f );

	const float clientTime = gEngfuncs.GetClientTime();

	for (int i = 0; i < count; i++)
	{
		TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( pos, pmodel );
		if( !pTemp ) return;

		pTemp->frameMax = pmodel->numframes - 1;
		if (pmodel->numframes > 1)
			pTemp->flags |= FTENT_SPRCYCLE;

		pTemp->entity.curstate.scale = scale;
		pTemp->entity.curstate.rendermode = rendermode;
		pTemp->entity.curstate.rendercolor = color;
		pTemp->entity.baseline.renderamt = pTemp->entity.curstate.renderamt = renderamt;
		pTemp->entity.curstate.renderfx = renderfx;
		pTemp->entity.curstate.framerate = framerate;

		pTemp->flags |= FTENT_SLOWGRAVITY;
		if (flags & SPRAY_FLAG_COLLIDEWORLD)
			pTemp->flags |= FTENT_COLLIDEWORLD;
		if (flags & SPRAY_FLAG_ANIMATE)
			pTemp->flags |= FTENT_SPRANIMATE;
		if (flags & SPRAY_FLAG_FADEOUT)
		{
			pTemp->flags |= FTENT_FADEOUT;
			pTemp->fadeSpeed = 2.0f;
		}

		if(pmodel->numframes > 1 && (flags & SPRAY_FLAG_ANIMATE))
		{
			pTemp->die = clientTime + (pTemp->frameMax / framerate);
		}
		else
			pTemp->die = clientTime + 0.35f;

		if (pmodel->numframes > 1 && !(flags & SPRAY_FLAG_ANIMATE))
		{
			pTemp->entity.curstate.frame = Com_RandomLong( 0, pmodel->numframes - 1 );
		}

		pTemp->entity.baseline.origin[0] = dir[0] + Com_RandomFloat( -noise, noise );
		pTemp->entity.baseline.origin[1] = dir[1] + Com_RandomFloat( -noise, noise );
		pTemp->entity.baseline.origin[2] = dir[2] + Com_RandomFloat( 0, znoise );
		VectorScale( pTemp->entity.baseline.origin, Com_RandomFloat(( speed * 0.8f ), ( speed * 1.2f )), pTemp->entity.baseline.origin );
	}
}

void FX_Spray(Vector pos, Vector dir, int modelIndex, int count, int speed, float noise, const Visual& visual, int flags)
{
	FX_Spray(pos, dir, modelIndex, count, speed, noise,
		visual.rendermode, MakeColor24(visual.rendercolor.r, visual.rendercolor.g, visual.rendercolor.b), visual.renderamt, visual.renderfx,
		RandomizeNumberFromRange(visual.scale), RandomizeNumberFromRange(visual.framerate), flags);
}

void FX_BloodSpray(const Vector& org, int colorIndex, int modelIndex, float size)
{
	model_t *pModel = gEngfuncs.pfnGetModelByIndex(modelIndex);
	if (!pModel)
		return;

	Vector pos = org;
	pos[2] += Com_RandomFloat(2.0f, 4.0f);

	TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->CL_TempEntAllocHigh(pos, pModel);
	if (!pTemp)
		return;

	pTemp->frameMax = pModel->numframes - 1;
	pTemp->entity.curstate.rendermode = kRenderTransTexture;
	pTemp->entity.curstate.renderfx = kRenderFxClampMinScale;
	pTemp->entity.curstate.scale = Com_RandomFloat(size / 25.0f, size / 35.0f);
	pTemp->flags = FTENT_SPRANIMATE;

	pTemp->entity.curstate.rendercolor = Color24FromPalette(colorIndex);
	pTemp->entity.baseline.renderamt = pTemp->entity.curstate.renderamt = 250;

	pTemp->entity.curstate.framerate = pTemp->frameMax * 4.0f;
	pTemp->die = gEngfuncs.GetClientTime() + (pTemp->frameMax / pTemp->entity.curstate.framerate ); // play the whole thing once

	pTemp->entity.curstate.frame = 0;
	pTemp->bounceFactor = 0;
	pTemp->entity.angles[2] = Com_RandomLong(0, 360);
}

void FX_BloodSplatter(const Vector& org, int colorIndex, int modelIndex, int amount)
{
	const float clientTime = gEngfuncs.GetClientTime();

	model_t *pModel = gEngfuncs.pfnGetModelByIndex(modelIndex);
	if (!pModel)
		return;

	Vector pos = org;
	const int splatterCount = amount + (Com_RandomLong( 1, 8 ) + Com_RandomLong( 1, 8 ));

	for (int i = 0; i < splatterCount; i++)
	{
		TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, pModel);
		if (!pTemp)
			return;

		pTemp->frameMax = pModel->numframes - 1;
		pTemp->entity.curstate.rendermode = kRenderTransTexture;
		pTemp->entity.curstate.renderfx = kRenderFxClampMinScale;
		pTemp->entity.curstate.scale = Com_RandomFloat( amount / 15.0f, amount / 25.0f );
		pTemp->flags = FTENT_ROTATE | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD;

		pTemp->entity.curstate.rendercolor = Color24FromPalette(colorIndex);
		pTemp->entity.baseline.renderamt = pTemp->entity.curstate.renderamt = 250;

		pTemp->entity.baseline.origin[0] = Com_RandomFloat( -96.0f, 95.0f );
		pTemp->entity.baseline.origin[1] = Com_RandomFloat( -96.0f, 95.0f );
		pTemp->entity.baseline.origin[2] = Com_RandomFloat( -32.0f, 95.0f );
		pTemp->entity.baseline.angles[0] = Com_RandomFloat( -256.0f, -255.0f );
		pTemp->entity.baseline.angles[1] = Com_RandomFloat( -256.0f, -255.0f );
		pTemp->entity.baseline.angles[2] = Com_RandomFloat( -256.0f, -255.0f );

		pTemp->die = clientTime + Com_RandomFloat( 1.0f, 3.0f );

		pTemp->entity.curstate.frame = Com_RandomFloat( 1, pTemp->frameMax );

		if (pTemp->entity.curstate.frame > 8.0f)
			pTemp->entity.curstate.frame = pTemp->frameMax;

		pTemp->entity.angles[2] = Com_RandomFloat( 0.0f, 360.0f );
		pTemp->bounceFactor	= 0.0f;
	}
}

void FX_BloodStream(const Vector& org, const Vector& ndir, const IntRange& colorRange, int speed)
{
	const float clientTime = gEngfuncs.GetClientTime();

	int i;
	float arc;
	int accel = speed;

	const Vector dir = ndir.Normalize();

	for (arc = 0.05f, i = 0; i < 100; i++)
	{
		particle_t *p = AllocColoredParticle(RandomizeNumberFromRange(colorRange));
		if (!p) return;

		p->die = clientTime + 2.0f;
		p->type = pt_vox_grav;

		p->org = org;
		p->vel = dir;

		p->vel[2] -= arc;
		arc -= 0.005f;
		p->vel *= accel;
		accel -= 1;
	}

	for (arc = 0.075f, i = 0; i < (speed / 5); i++)
	{
		particle_t *p = AllocColoredParticle(RandomizeNumberFromRange(colorRange));
		if (!p) return;

		p->die = clientTime + 3.0f;
		p->type = pt_vox_slowgrav;

		p->org = org;
		p->vel = dir;

		p->vel[2] -= arc;
		arc -= 0.005f;

		float num = Com_RandomFloat(0.0f, 1.0f);
		accel = speed * num;
		num *= 1.7f;

		p->vel *= num;
		p->vel *= accel;

		for (int j = 0; j < 2; j++)
		{
			p = AllocColoredParticle(RandomizeNumberFromRange(colorRange));
			if (!p) return;

			p->die = clientTime + 3.0f;
			p->type = pt_vox_slowgrav;

			p->org = org + Vector(Com_RandomFloat(-1.0f, 1.0f), Com_RandomFloat(-1.0f, 1.0f), Com_RandomFloat(-1.0f, 1.0f));

			p->vel = dir;
			p->vel[2] -= arc;

			p->vel *= num;
			p->vel *= accel;
		}
	}
}

void FX_BloodLegacy(const Vector& org, const Vector& ndir, const IntRange& colorRange, int amount)
{
	const float clientTime = gEngfuncs.GetClientTime();
	const Vector dir = ndir.IsZero() ? Vector() : ndir.Normalize();

	int pspeed = amount * 3;

	const float posNoise = 3.0f;
	const float dirNoise = 0.06f;

	for (int i = 0; i < (amount / 2); i++)
	{
		const Vector pos = org + Vector(Com_RandomFloat(-posNoise, posNoise), Com_RandomFloat(-posNoise, posNoise), Com_RandomFloat(-posNoise, posNoise));
		const Vector vec = dir + Vector(Com_RandomFloat(-dirNoise, dirNoise), Com_RandomFloat(-dirNoise, dirNoise), Com_RandomFloat(-dirNoise, dirNoise));

		for (int j = 0; j < 8; j++)
		{
			particle_t *p = AllocColoredParticle(RandomizeNumberFromRange(colorRange));
			if (!p) return;

			p->die = clientTime + 1.5f;
			p->type = pt_vox_grav;

			p->org = pos + Vector(Com_RandomFloat(-1.0f, 1.0f), Com_RandomFloat(-1.0f, 1.0f), Com_RandomFloat(-1.0f, 1.0f));
			p->vel = vec * pspeed;
		}

		pspeed -= amount;
	}
}

void FX_BloodParticles(const Vector& org, const IntRange& colorRange, int count)
{
	const float clientTime = gEngfuncs.GetClientTime();

	for (int i = 0; i < count; i++)
	{
		particle_t *p = AllocColoredParticle(RandomizeNumberFromRange(colorRange));
		if (!p) return;

		p->die = clientTime + Com_RandomFloat(1.0f, 3.0f);
		p->type = pt_grav;
		p->org = org;
		p->vel[0] = Com_RandomFloat( -96.0f, 95.0f );
		p->vel[1] = Com_RandomFloat( -96.0f, 95.0f );
		p->vel[2] = Com_RandomFloat( -32.0f, 95.0f );
	}
}

class CQParticle : public CBaseParticle
{
public:
	CQParticle() = default;
	CQParticle(const Vector& pos)
	{
		model_t* sprite = cl_sprite_dot_index ? cl_sprite_dot_index : cl_sprite_dot;
		const float size = cl_sprite_dot_index ? 1.0f : 1.5f;
		InitializeSprite(pos, Vector(0.0f, 0.0f, 0.0f), sprite, size, 255);
		strcpy(m_szClassname, "qparticle");

		m_iRendermode = cl_sprite_dot_index ? kRenderTransAlpha : kRenderTransAdd;

		SetLightFlag(LIGHT_NONE);
		SetCullFlag(CULL_PVS);
		SetRenderFlag(RENDER_FACEPLAYER);

		m_flGravity = 0.05f;
	}
	void Draw()
	{
		const float particleSize = m_flSize;

		float factor = DotProduct((m_vOrigin - v_origin), g_viewPlaneNormal);
		if (factor < 20)
			factor = 1;
		else
			factor = 1 + factor * 0.004f;

		m_flSize *= factor;

		CBaseParticle::Draw();
		m_flSize = particleSize;
	}
};

// Quake wall impact puffs and blood
void FX_QuakeParticles(const Vector& org, const Vector& ndir, const IntRange& colorRange, int count)
{
	const float clientTime = gEngfuncs.GetClientTime();
	const Vector dir = ndir.IsZero() ? Vector() : ndir.Normalize();

	const Vector velocity = dir * 15.0f;

	for (int i=0 ; i<count ; i++)
	{
		Vector position = org;
		for (int j=0 ; j<3 ; j++)
		{
			position[j] = org[j] + Com_RandomLong(-8, 8);
		}
		const float dieTime = clientTime + 0.1f * Com_RandomLong(2, 5);

		if (cl_sprite_dot_index)
		{
			CQParticle *particle = new CQParticle(position);

			particle->m_vVelocity = velocity;

			color24 rgb = Color24FromPalette(RandomizeNumberFromRange(colorRange));
			particle->m_vColor[0] = rgb.r;
			particle->m_vColor[1] = rgb.g;
			particle->m_vColor[2] = rgb.b;

			particle->m_flDieTime = dieTime;
		}
		else
		{
			particle_t *p = AllocColoredParticle(RandomizeNumberFromRange(colorRange));
			if (!p) return;
			p->die = dieTime;
			p->type = pt_slowgrav;
			p->org = position;
			p->vel = velocity;
		}
	}
}

// Quake 2 wall impact puffs and blood
void FX_DotParticles(const Vector& org, const Vector& ndir, const IntRange& colorRange, int count)
{
	const float clientTime = gEngfuncs.GetClientTime();
	const Vector dir = ndir.IsZero() ? Vector() : ndir.Normalize();

	for (int i=0 ; i<count ; i++)
	{
		const float d = Com_RandomLong(0, 31);

		Vector position;
		Vector velocity;
		for (int j=0 ; j<3 ; j++)
		{
			position[j] = org[j] + Com_RandomLong(-4, 4) + d*dir[j];
			velocity[j] = Com_RandomFloat(-20.0f, 20.0f);
		}

		CQParticle *particle = new CQParticle(position);

		particle->m_vVelocity = velocity;

		color24 rgb = Color24FromPalette(RandomizeNumberFromRange(colorRange));
		particle->m_vColor[0] = rgb.r;
		particle->m_vColor[1] = rgb.g;
		particle->m_vColor[2] = rgb.b;

		particle->m_flFadeSpeed = 0;
		particle->m_flDieTime = clientTime + Com_RandomFloat(0.6f, 0.8f);
	}
}

void FX_ImpactParticles(const Vector& pos, int baseColor)
{
	constexpr int maxDistance = 1000;
	constexpr int maxColorVariance = 3;
	constexpr int maxBaseQuantity = 10;

	int dist = (int)(pos - v_origin).Length();

	int quantity = (maxDistance - dist + 40) / 100;
	quantity = clamp(quantity, 1, maxBaseQuantity);

	int color = baseColor;

	if (baseColor < 224)
	{
		int darkener = (maxColorVariance * maxBaseQuantity * quantity) / 100;
		darkener = clamp(darkener, 0, maxColorVariance);

		int shift = darkener - 2;

		if (baseColor >= 0 && baseColor <= 127)
			shift = -shift;

		const int palColumn = baseColor % 16;
		const int palRow = baseColor / 16;

		if (palColumn + shift >= 16)
		{
			color = palRow * 16 + 15;
		}
		else if (palColumn + shift < 0)
		{
			color = palRow * 16;
		}
		else
		{
			color += shift;
		}
	}

	//gEngfuncs.Con_Printf("ImpactParticles: base index: %d. Result: %d. Dist: %d\n", baseColor, color, dist);

	for (int i = 0; i < quantity * 4; i++)
	{
		particle_t	*p = AllocColoredParticle(color);
		if (!p) return;

		p->org = pos;

		p->vel[0] = Com_RandomFloat( -1.0f, 1.0f );
		p->vel[1] = Com_RandomFloat( -1.0f, 1.0f );
		p->vel[2] = Com_RandomFloat( -1.0f, 1.0f );

		p->vel = p->vel * Com_RandomFloat( 50.0f, 100.0f );

		p->die = gEngfuncs.GetClientTime() + 0.5f;
		p->type = pt_grav;
	}
}

void FX_WallImpact(const Vector& pos, const Vector& dir, int particleColor, int style)
{
	switch(style)
	{
	case WALLIMPACT_QUAKE:
		FX_QuakeParticles(pos + dir, Vector(), GetRangeForColorIndex(particleColor, 6), 20);
		break;
	case WALLIMPACT_QUAKE2:
		FX_DotParticles(pos, dir, GetRangeForColorIndex(particleColor, 6), 40);
		break;
	case WALLIMPACT_HL:
	default:
		FX_ImpactParticles(pos, particleColor);
		break;
	}
}

void FX_GunshotDecal(const Vector& pos, const Vector& dir, int decalIndex, int entIndex, int particleColor)
{
	gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalIndex), entIndex, 0, Vector(pos), 0);

	FX_WallImpact(pos, -dir, particleColor, GetWallImpactStyle());

	if (Com_RandomLong(0, 1))
	{
		gEngfuncs.pEfxAPI->R_RicochetSound(pos);
	}
}
