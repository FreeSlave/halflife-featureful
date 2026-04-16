#include <cstdint>
#include <algorithm>

#include "environment.h"

#include "parsemsg.h"

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"

#include "particleman.h"
#include "r_studioint.h"
#include "triangleapi.h"

#include "pm_shared.h"
#include "pm_defs.h"

#include "hull_types.h"
#include "fx_flags.h"
#include "pi_constant.h"

extern engine_studio_api_t IEngineStudio;

extern Vector g_vPlayerVelocity;
extern const Vector g_vecZero;

extern cvar_t* cl_weather;

constexpr const char* RAINDROP_DEFAULT_SPRITE = "sprites/effects/rain.spr";
constexpr const char* WINDPUFF_DEFAULT_SPRITE = "sprites/gas_puff_01.spr";
constexpr const char* SPLASH_DEFAULT_SPRITE = "sprites/wsplash3.spr";
constexpr const char* RIPPLE_DEFAULT_SPRITE = "sprites/effects/ripple.spr";
constexpr const char* SNOWFLAKE_DEFAULT_SPRITE = "sprites/effects/snowflake.spr";

constexpr int DEFAULT_WEATHER_INTENSITY = 150;
constexpr float DEFAULT_WEATHER_UPDATE_PERIOD = 0.3f;
constexpr int DEFAULT_WEATHER_DISTANCE = 400;
constexpr int DEFAULT_PRECIPITATION_MIN_HEIGHT = 100;
constexpr int DEFAULT_PRECIPITATION_MAX_HEIGHT = 300;

CEnvironment g_Environment;

static int ParticleLightModeToFlag(int lightMode)
{
	switch (lightMode) {
	case PARTICLE_LIGHT_NONE:
		return LIGHT_NONE;
	case PARTICLE_LIGHT_COLOR:
		return LIGHT_COLOR;
	case PARTICLE_LIGHT_INTENSITY:
		return LIGHT_INTENSITY;
	default:
		return 0;
	}
}

float ParticleParams::GetSize() const
{
	if (maxSize > minSize)
		return Com_RandomFloat(minSize, maxSize);
	return minSize;
}

WeatherData::WeatherData():
	entIndex(0),
	flags(0),
	intensity(DEFAULT_WEATHER_INTENSITY),
	updatePeriod(DEFAULT_WEATHER_UPDATE_PERIOD),
	distance(DEFAULT_WEATHER_DISTANCE),
	minHeight(DEFAULT_PRECIPITATION_MIN_HEIGHT),
	maxHeight(DEFAULT_PRECIPITATION_MAX_HEIGHT),
	location(0.0f,0.0f,0.0f),
	absmin(0.0f,0.0f,0.0f),
	absmax(0.0f,0.0f,0.0f),
	updateTime(0.0f)
{}

bool WeatherData::IsLocalizedPoint() const
{
	return (flags & SF_WEATHER_LOCALIZED) != 0;
}

bool WeatherData::IsVolume() const
{
	return (flags & WEATHER_BRUSH_ENTITY) != 0;
}

bool WeatherData::IsGlobal() const
{
	return !IsLocalizedPoint() && !IsVolume();
}

bool WeatherData::AllowIndoors() const
{
	return (flags & SF_WEATHER_ALLOW_INDOOR) != 0;
}

bool WeatherData::DistanceIsRadius() const
{
	return (flags & SF_WEATHER_DISTANCE_IS_RADIUS) != 0;
}

bool WeatherData::NeedsPVSCheck() const
{
	return IsLocalizedPoint() || IsVolume();
}

float WeatherData::RandomHeight() const
{
	if (maxHeight > minHeight)
		return Com_RandomFloat(minHeight, maxHeight);
	return minHeight;
}

bool WeatherData::CanGoThroughBrushEntities() const
{
	return (flags & SF_WEATHER_GO_THROUGH_BRUSH_ENTITIES) != 0;
}

Vector WeatherData::GetWeatherOrigin(const Vector &globalWeatherOrigin) const
{
	if (IsVolume())
	{
		Vector result = (absmin + absmax)/2.0f;
		result.z = absmin.z + 8.0f;
		return result;
	}
	if (IsLocalizedPoint())
		return location;
	return globalWeatherOrigin;
}

void WeatherData::GetBoundingBox(const Vector &weatherOrigin, Vector &mins, Vector &maxs) const
{
	if (IsVolume())
	{
		mins = absmin;
		maxs = absmax;
	}
	else
	{
		mins = weatherOrigin - Vector(distance, distance, 0);
		maxs = weatherOrigin + Vector(distance, distance, Q_max(minHeight, maxHeight));
	}
}

Vector WeatherData::GetRandomOrigin(const Vector& weatherOrigin) const
{
	Vector vecOrigin;
	if (IsVolume())
	{
		vecOrigin.x = Com_RandomFloat(absmin.x, absmax.x);
		vecOrigin.y = Com_RandomFloat(absmin.y, absmax.y);
		const float minZ = absmin.z + (absmax.z - absmin.z) / 3.0f;
		vecOrigin.z = Com_RandomFloat(minZ, absmax.z);
	}
	else
	{
		vecOrigin = weatherOrigin;
		if (DistanceIsRadius())
		{
			const float radius = Com_RandomFloat( -distance, distance );
			const float angle = Com_RandomFloat(0.0f, M_PI * 2);
			vecOrigin.x += radius * cos(angle);
			vecOrigin.y += radius * sin(angle);
		}
		else
		{
			vecOrigin.x += Com_RandomFloat( -distance, distance );
			vecOrigin.y += Com_RandomFloat( -distance, distance );
		}
		vecOrigin.z += RandomHeight();
	}
	return vecOrigin;
}

RainData::RainData():
	WeatherData(),
	rainSprite(nullptr),
	windPuffSprite(nullptr),
	splashSprite(nullptr),
	rippleSprite(nullptr)
{
	raindropParticleParams.minSize = raindropParticleParams.maxSize = 2.0f;
	raindropParticleParams.color = Vector(255.0f, 255.0f, 255.0f);
	raindropParticleParams.brightness = 128;
	raindropParticleParams.renderMode = kRenderTransAlpha;
	raindropParticleParams.lightFlag = LIGHT_NONE;
	raindropStretchY = 40.0f;

	raindropMinSpeed = 500.0f;
	raindropMaxSpeed = 1800.0f;
	raindropLife = 1.0f;

	windParticleParams.minSize = 50.0f;
	windParticleParams.maxSize = 75.0f;
	windParticleParams.color = Vector(128.0f, 128.0f, 128.0f);
	windParticleParams.brightness = 128;
	windParticleParams.renderMode = kRenderTransAlpha;
	windParticleParams.lightFlag = LIGHT_NONE;

	windpuffLife = 6.0f;

	splashParticleParams.minSize = 20;
	splashParticleParams.maxSize = 25;
	splashParticleParams.color = Vector(255.0f, 255.0f, 255.0f);
	splashParticleParams.brightness = 150;
	splashParticleParams.renderMode = kRenderTransAdd;
	splashParticleParams.lightFlag = LIGHT_INTENSITY;

	rippleParticleParams.minSize = rippleParticleParams.maxSize = 15.0f;
	rippleParticleParams.color = Vector(255.0f, 255.0f, 255.0f);
	rippleParticleParams.brightness = 150;
	rippleParticleParams.renderMode = kRenderTransAdd;
	rippleParticleParams.lightFlag = LIGHT_INTENSITY;
}

float RainData::GetRaindropFallingSpeed() const
{
	float speed1 = fabs(raindropMinSpeed);
	float speed2 = fabs(raindropMaxSpeed);

	float minSpeed = speed1 < speed2 ? speed1 : speed2;
	float maxSpeed = speed1 > speed2 ? speed1 : speed2;

	if (maxSpeed > minSpeed)
		return Com_RandomFloat(minSpeed, maxSpeed);
	return minSpeed;
}

bool RainData::RaindropsAffectedByWind() const
{
	return !(flags & SF_RAIN_NOT_AFFECTED_BY_WIND);
}

bool RainData::WindPuffsAllowed() const
{
	return !(flags & SF_RAIN_NO_WIND);
}

bool RainData::SplashesAllowed() const
{
	return !(flags & SF_RAIN_NO_SPLASHES);
}

bool RainData::RipplesAllowed() const
{
	return !(flags & SF_RAIN_NO_RIPPLES);
}

SnowData::SnowData():
	WeatherData(),
	snowSprite(nullptr)
{
	updatePeriod = 0.5f;

	snowflakeParticleParams.minSize = 2.0f;
	snowflakeParticleParams.maxSize = 2.5f;
	snowflakeParticleParams.color = Vector(128.0f, 128.0f, 128.0f);
	snowflakeParticleParams.brightness = 130.0f;
	snowflakeParticleParams.renderMode = kRenderTransAdd;
	snowflakeParticleParams.lightFlag = LIGHT_NONE;
	snowflakeInitialBrightness = 1;
	snowflakeMinSpeed = 100;
	snowflakeMaxSpeed = 200;
	snowflakeLife = 3.0f;
}

float SnowData::GetSnowflakeFallingSpeed() const
{
	float speed1 = fabs(snowflakeMinSpeed);
	float speed2 = fabs(snowflakeMaxSpeed);

	float minSpeed = speed1 < speed2 ? speed1 : speed2;
	float maxSpeed = speed1 > speed2 ? speed1 : speed2;

	if (maxSpeed > minSpeed)
		return Com_RandomFloat(minSpeed, maxSpeed);
	return minSpeed;
}

bool SnowData::SnowflakesAffectedByWind() const
{
	return !(flags & SF_SNOW_NOT_AFFECTED_BY_WIND);
}

class CPartRainDrop : public CBaseParticle
{
public:
	CPartRainDrop() = default;
	void Think( float flTime ) override;
	void Touch( Vector pos, Vector normal, int index ) override;

	bool m_splashAllowed = true;
	bool m_rippleAllowed = true;
	ParticleParams m_splashParams;
	ParticleParams m_rippleParams;
	model_t* m_pRainSplash = nullptr;
	model_t* m_pRipple = nullptr;

private:
	bool m_bTouched = false;
};

void CPartRainDrop::Think( float flTime )
{
	Vector vecViewAngles;
	Vector vecForward, vecRight, vecUp;

	gEngfuncs.GetViewAngles( vecViewAngles );

	AngleVectors( vecViewAngles, vecForward, vecRight, vecUp );

	m_vAngles.y = vecViewAngles.y;
	m_vAngles.z = atan( DotProduct( m_vVelocity, vecRight ) / m_vVelocity.z ) * ( 180.0 / M_PI );

	if( m_flBrightness < 155.0f )
		m_flBrightness += 6.5f;

	CBaseParticle::Think( flTime );
}

void CPartRainDrop::Touch( Vector pos, Vector normal, int index )
{
	if( m_bTouched )
	{
		return;
	}

	m_bTouched = true;

	Vector vecStart = m_vOrigin;
	vecStart.z += 32.0f;

	pmtrace_t trace;

	{
		int traceFlags = PM_STUDIO_BOX | PM_STUDIO_IGNORE;
		if ((GetCollisionFlags() & (TRI_COLLIDEBRUSHENTS)) == 0)
			traceFlags = PM_WORLD_ONLY;

		Vector vecEnd = m_vOrigin;
		vecEnd.z -= 16.0f;

		gEngfuncs.pEventAPI->EV_PlayerTrace(vecStart, vecEnd, traceFlags, -1, &trace);

		if (trace.ent != 0 && (GetCollisionFlags() & (TRI_COLLIDEBRUSHENTS)) != 0)
		{
			physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent(trace.ent);
			if (!entity || !entity->model || entity->model->name[0] != '*')
			{
				gEngfuncs.pEventAPI->EV_PlayerTrace(vecStart, vecEnd, PM_WORLD_ONLY, -1, &trace);
			}
		}
	}

	Vector vecNormal;

	vecNormal.x = normal.x;
	vecNormal.y = normal.y;
	vecNormal.z = -normal.z;

	Vector vecAngles;

	VectorAngles( vecNormal, vecAngles );

	if( gEngfuncs.PM_PointContents( trace.endpos, nullptr ) == gEngfuncs.PM_PointContents( vecStart, nullptr ) )
	{
		if (m_splashAllowed && m_pRainSplash)
		{
			CBaseParticle* pParticle = new CBaseParticle();

			model_t* pRainSplash = m_pRainSplash;

			pParticle->InitializeSprite( m_vOrigin + normal, Vector( 90.0f, 0.0f, 0.0f ), pRainSplash, m_splashParams.GetSize(), m_splashParams.brightness );

			pParticle->m_iRendermode = m_splashParams.renderMode;

			pParticle->m_flMass = 1.0f;
			pParticle->m_flGravity = 0.1f;

			pParticle->SetCullFlag( CULL_PVS );
			pParticle->SetLightFlag( m_splashParams.lightFlag );

			pParticle->m_vColor = m_splashParams.color;

			pParticle->m_iNumFrames = pRainSplash->numframes - 1;
			pParticle->m_iFramerate = Com_RandomLong( 30, 45 );
			pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 0.3f;
			pParticle->SetCollisionFlags( TRI_ANIMATEDIE );
			pParticle->SetRenderFlag( RENDER_FACEPLAYER );
		}
	}
	else
	{
		if (m_rippleAllowed && m_pRipple)
		{
			Vector vecBegin = vecStart;
			Vector vecEnd = trace.endpos;

			Vector vecDist = vecEnd - vecBegin;

			Vector vecHalf;

			Vector vecNewBegin;

			while( vecDist.IsLengthGreaterThan(4.0) )
			{
				vecDist = vecDist * 0.5;

				vecHalf = vecBegin + vecDist;

				if( gEngfuncs.PM_PointContents( vecBegin, nullptr ) == gEngfuncs.PM_PointContents( vecHalf, nullptr ) )
				{
					vecBegin = vecHalf;
					vecNewBegin = vecHalf;
				}
				else
				{
					vecEnd = vecHalf;
					vecNewBegin = vecBegin;
				}

				vecDist = vecEnd - vecNewBegin;
			}

			CBaseParticle* pParticle = new CBaseParticle();

			pParticle->InitializeSprite( vecBegin, vecAngles, m_pRipple, m_rippleParams.GetSize(), m_rippleParams.brightness );

			pParticle->m_iRendermode = m_rippleParams.renderMode;
			pParticle->m_flScaleSpeed = 1.0f;
			pParticle->m_vColor = m_rippleParams.color;
			pParticle->SetCullFlag( CULL_PVS );
			pParticle->SetLightFlag( m_rippleParams.lightFlag );
			pParticle->m_flFadeSpeed = 2.0f;
			pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 2.0f;
		}
	}
}

class CPartWind : public CBaseParticle
{
public:
	CPartWind() = default;

	void Think( float flTime ) override;
};

void CPartWind::Think( float flTime )
{
	if( m_flDieTime - flTime <= 3.0 )
	{
		if( m_flBrightness > 0.0 )
		{
			m_flBrightness -= ( flTime - m_flTimeCreated ) * 0.4;
		}

		if( m_flBrightness < 0.0 )
		{
			m_flBrightness = 0;
			flTime = m_flDieTime = gEngfuncs.GetClientTime();
		}
	}
	else
	{
		if( m_flBrightness < 105.0 )
		{
			m_flBrightness += ( flTime - m_flTimeCreated ) * 5.0 + 4.0;
		}
	}

	CBaseParticle::Think( flTime );
}

class CPartSnowFlake : public CBaseParticle
{
public:
	CPartSnowFlake() = default;
	void Think( float flTime ) override;
	void Touch( Vector pos, Vector normal, int index ) override;

public:
	bool m_bSpiral;
	float m_flSpiralTime;
	int m_targetBrightness;

private:
	bool m_bTouched = false;
};

void CPartSnowFlake::Think( float flTime )
{
	if( m_flBrightness < m_targetBrightness && !m_bTouched )
		m_flBrightness += 4.5;

	if (m_flBrightness > 255.0f)
		m_flBrightness = 255.0f;

	Fade( flTime );
	Spin( flTime );

	if( m_flSpiralTime <= gEngfuncs.GetClientTime() )
	{
		m_bSpiral = !m_bSpiral;

		m_flSpiralTime = gEngfuncs.GetClientTime() + Com_RandomLong( 2, 4 );
	}

	if( m_bSpiral && !m_bTouched )
	{
		const float flDelta = flTime - g_Environment.GetOldTime();

		const float flSpin = sin( flTime * 5.0 + reinterpret_cast<std::intptr_t>( this ) );

		m_vOrigin = m_vOrigin + m_vVelocity * flDelta;

		m_vOrigin.x += ( flSpin * flSpin ) * 0.3;
	}
	else
	{
		CalculateVelocity( flTime );
	}

	CheckCollision( flTime );
}

void CPartSnowFlake::Touch( Vector pos, Vector normal, int index )
{
	if( m_bTouched )
	{
		return;
	}

	m_bTouched = true;

	SetRenderFlag( RENDER_FACEPLAYER );

	m_flOriginalBrightness = m_flBrightness;

	m_vVelocity = g_vecZero;

	//m_iRendermode = kRenderTransAdd;

	m_flFadeSpeed = 0;
	m_flScaleSpeed = 0;
	m_flDampingTime = 0;
	m_iFrame = 0;
	m_flMass = 1.0;
	m_flGravity = 0;

	//m_vColor.x = m_vColor.y = m_vColor.z = 128.0;

	m_flDieTime = gEngfuncs.GetClientTime() + 0.5;

	m_flTimeCreated = gEngfuncs.GetClientTime();
}

void CEnvironment::Initialize()
{
	Reset();

	m_flWeatherValue = cl_weather->value;
}

void CEnvironment::Reset()
{
	m_pRainSprite = nullptr;
	m_pGasPuffSprite = nullptr;
	m_pRainSplash = nullptr;
	m_pRipple = nullptr;
	m_pSnowSprite = nullptr;

	Clear();

	m_vecWeatherOrigin = g_vecZero;

	m_vecWind.x = Com_RandomFloat( -80.0f, 80.0f );
	m_vecWind.y = Com_RandomFloat( -80.0f, 80.0f );
	m_vecWind.z = 0;

	m_vecDesiredWindDirection.x = Com_RandomFloat( -80.0f, 80.0f );
	m_vecDesiredWindDirection.y = Com_RandomFloat( -80.0f, 80.0f );
	m_vecDesiredWindDirection.z = 0;

	m_flNextWindChangeTime = gEngfuncs.GetClientTime();
}

void CEnvironment::Clear()
{
	m_rains.clear();
	m_snows.clear();
}

void CEnvironment::Update()
{
	Vector vecOrigin = gHUD.m_vecOrigin;

	if( g_iUser1 > 0 && g_iUser1 != OBS_ROAMING )
	{
		if( cl_entity_t* pFollowing = gEngfuncs.GetEntityByIndex( g_iUser2 ) )
		{
			vecOrigin = pFollowing->origin;
		}
	}

	vecOrigin.z += 36.0f;

	if( cl_weather->value > 3.0 )
	{
		gEngfuncs.Cvar_SetValue( "cl_weather", 3.0 );
	}

	m_flWeatherValue = cl_weather->value;

	if( !IEngineStudio.IsHardware() )
		m_flWeatherValue = 0;

	m_vecWeatherOrigin = vecOrigin;

	if (ShouldUpdateWind())
		UpdateWind();

	const float clientTime = gEngfuncs.GetClientTime();

	for (auto& rain : m_rains)
	{
		if (rain.updateTime <= clientTime)
		{
			UpdateRain(rain);
			rain.updateTime = clientTime + rain.updatePeriod;
		}
	}
	for (auto& snow : m_snows)
	{
		if (snow.updateTime <= clientTime)
		{
			UpdateSnow(snow);
			snow.updateTime = clientTime + snow.updatePeriod;
		}
	}

	m_flOldTime = clientTime;
}

bool CEnvironment::ShouldUpdateWind() const
{
	for (const auto& rain : m_rains)
	{
		if (rain.RaindropsAffectedByWind() || rain.WindPuffsAllowed())
			return true;
	}
	for(const auto& snow : m_snows)
	{
		if (snow.SnowflakesAffectedByWind())
			return true;
	}
	return false;
}

void CEnvironment::UpdateWind()
{
	if( m_flNextWindChangeTime <= gEngfuncs.GetClientTime() )
	{
		m_vecDesiredWindDirection.x = Com_RandomFloat( -80.0f, 80.0f );
		m_vecDesiredWindDirection.y = Com_RandomFloat( -80.0f, 80.0f );
		m_vecDesiredWindDirection.z = 0;

		m_flNextWindChangeTime = gEngfuncs.GetClientTime() + Com_RandomFloat( 15.0f, 30.0f );

		m_flDesiredWindSpeed = m_vecDesiredWindDirection.Length();
		Vector vecDir = m_vecDesiredWindDirection.Normalize();

		if( vecDir.x == 0.0f && vecDir.y == 0.0f )
		{
			m_flIdealYaw = 0;
		}
		else
		{
			m_flIdealYaw = floor( atan2( vecDir.y, vecDir.x ) * ( 180.0 / M_PI ) );

			if( m_flIdealYaw < 0.0f )
				m_flIdealYaw += 360.0f;
		}
	}

	Vector vecWindDir = m_vecWind;

	vecWindDir = vecWindDir.Normalize();

	Vector vecAngles;

	VectorAngles( vecWindDir, vecAngles );

	float flYaw;

	if( vecAngles.y < 0.0f )
	{
		flYaw = 120 * ( 3 * static_cast<int>( floor( vecAngles.y / 360.0 ) ) + 3 );
	}
	else
	{
		if( vecAngles.y < 360.0f )
		{
			flYaw = vecAngles.y;
		}
		else
		{
			flYaw = vecAngles.y - ( 360 * floor( vecAngles.y / 360.0 ) );
		}
	}

	if( m_flIdealYaw != flYaw )
	{
		const float flSpeed = ( gEngfuncs.GetClientTime() - m_flOldTime ) * 0.5 * 10.0;
		vecAngles.y = UTIL_ApproachAngle( m_flIdealYaw, flYaw, flSpeed );
	}

	Vector vecNewWind;

	AngleVectors( vecAngles, vecNewWind, nullptr, nullptr );

	m_vecWind = vecNewWind * m_flDesiredWindSpeed;
}

void CEnvironment::UpdateRain(const RainData& rainData)
{
	const float rainIntensity = rainData.intensity * m_flWeatherValue;
	if( rainIntensity > 0.0f )
	{
		int iWindParticle = 0;

		Vector vecEndPos;
		pmtrace_t trace;

		Vector weatherOrigin = rainData.GetWeatherOrigin(m_vecWeatherOrigin);

		int rainDropCount = 0;
		int windParticleCount = 0;

		const bool allowIndoors = rainData.AllowIndoors();
		const bool windParticlesEnabled = rainData.WindPuffsAllowed();

		bool inPvs = true;
		if (rainData.NeedsPVSCheck())
		{
			Vector pvsMins;
			Vector pvsMaxs;
			rainData.GetBoundingBox(weatherOrigin, pvsMins, pvsMaxs);
			inPvs = gEngfuncs.pTriAPI->BoxInPVS( pvsMins, pvsMaxs ) != 0;
		}

		// Optimization: don't create localized rain particles when not in PVS
		if (!inPvs)
			return;

		const int traceFlags = rainData.CanGoThroughBrushEntities() ? PM_WORLD_ONLY : (PM_STUDIO_BOX | PM_STUDIO_IGNORE);

		for( size_t uiIndex = 0; static_cast<float>( uiIndex ) < rainIntensity; ++uiIndex )
		{
			Vector vecOrigin = rainData.GetRandomOrigin(weatherOrigin);

			vecEndPos.x = vecOrigin.x + ( ( Com_RandomLong( 0, 5 ) > 2 ) ? g_vPlayerVelocity.x : -g_vPlayerVelocity.x );
			vecEndPos.y = vecOrigin.y + g_vPlayerVelocity.y;
			vecEndPos.z = 8000.0f;

			gEngfuncs.pEventAPI->EV_SetTraceHull( large_hull );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecOrigin, vecEndPos, traceFlags, -1, &trace );
			const char* pszTexture = gEngfuncs.pEventAPI->EV_TraceTexture( trace.ent, vecOrigin, trace.endpos );

			if( allowIndoors || (pszTexture && strncmp( pszTexture, "sky", 3 ) == 0) )
			{
				CPartRainDrop* rainParticle = CreateRaindrop( vecOrigin, rainData );
				if (rainParticle)
					rainDropCount++;

				if (windParticlesEnabled)
				{
					if( iWindParticle == 15 )
					{
						iWindParticle = 1;

						Vector vecWindOrigin;
						vecWindOrigin.x = vecOrigin.x;
						vecWindOrigin.y = vecOrigin.y;
						vecWindOrigin.z = weatherOrigin.z;

						vecEndPos.z = 8000.0f;

						gEngfuncs.pEventAPI->EV_SetTraceHull( large_hull );
						gEngfuncs.pEventAPI->EV_PlayerTrace( vecWindOrigin, vecEndPos, PM_WORLD_ONLY, -1, &trace );
						pszTexture = gEngfuncs.pEventAPI->EV_TraceTexture( trace.ent, vecOrigin, trace.endpos );

						if( allowIndoors || (pszTexture && strncmp( pszTexture, "sky", 3 ) == 0) )
						{
							vecEndPos.z = -8000.0f;

							gEngfuncs.pEventAPI->EV_SetTraceHull( large_hull );
							gEngfuncs.pEventAPI->EV_PlayerTrace( vecWindOrigin, vecEndPos, PM_WORLD_ONLY, -1, &trace );

							CPartWind* windParticle = CreateWindParticle( trace.endpos, rainData );
							if (windParticle)
								windParticleCount++;
						}
					}
					else
					{
						++iWindParticle;
					}
				}
			}
		}

		//gEngfuncs.Con_Printf("Rain drops spawned: %d. Wind particles spawned: %d\n", rainDropCount, windParticleCount);
	}
}

void CEnvironment::UpdateSnow(const SnowData& snowData)
{
	const float snowIntensity = snowData.intensity * m_flWeatherValue;
	if( snowIntensity > 0.0f )
	{
		Vector vecEndPos;
		pmtrace_t trace;

		Vector weatherOrigin = snowData.GetWeatherOrigin(m_vecWeatherOrigin);

		const bool allowIndoors = snowData.AllowIndoors();

		bool inPvs = true;
		if (snowData.NeedsPVSCheck())
		{
			Vector pvsMins;
			Vector pvsMaxs;
			snowData.GetBoundingBox(weatherOrigin, pvsMins, pvsMaxs);
			inPvs = gEngfuncs.pTriAPI->BoxInPVS( pvsMins, pvsMaxs ) != 0;
		}

		// Optimization: don't create localized snow particles when not in PVS
		if (!inPvs)
			return;

		const int traceFlags = snowData.CanGoThroughBrushEntities() ? PM_WORLD_ONLY : (PM_STUDIO_BOX | PM_STUDIO_IGNORE);

		for( size_t uiIndex = 0; static_cast<float>( uiIndex ) < snowIntensity; ++uiIndex )
		{
			Vector vecOrigin = snowData.GetRandomOrigin(weatherOrigin);

			vecEndPos.x = vecOrigin.x + ( ( Com_RandomLong( 0, 5 ) > 2 ) ? g_vPlayerVelocity.x : -g_vPlayerVelocity.x );
			vecEndPos.y = vecOrigin.y + g_vPlayerVelocity.y;
			vecEndPos.z = 8000.0f;

			gEngfuncs.pEventAPI->EV_SetTraceHull( large_hull );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecOrigin, vecEndPos, traceFlags, -1, &trace );
			const char* pszTexture = gEngfuncs.pEventAPI->EV_TraceTexture( trace.ent, vecOrigin, trace.endpos );

			if( allowIndoors || (pszTexture && strncmp( pszTexture, "sky", 3 ) == 0) )
			{
				CreateSnowFlake( vecOrigin, snowData );
			}
		}
	}
}

CPartRainDrop* CEnvironment::CreateRaindrop( const Vector& vecOrigin, const RainData& rainData )
{
	if( !rainData.rainSprite )
	{
		return nullptr;
	}

	CPartRainDrop* pParticle = new CPartRainDrop();
	pParticle->m_splashAllowed = rainData.SplashesAllowed();
	pParticle->m_rippleAllowed = rainData.RipplesAllowed();
	pParticle->m_splashParams = rainData.splashParticleParams;
	pParticle->m_rippleParams = rainData.rippleParticleParams;
	pParticle->m_pRainSplash = rainData.splashSprite;
	pParticle->m_pRipple = rainData.rippleSprite;

	pParticle->InitializeSprite( vecOrigin, g_vecZero, rainData.rainSprite, rainData.raindropParticleParams.GetSize(), rainData.raindropParticleParams.brightness );

	strcpy( pParticle->m_szClassname, "particle_rain" );

	pParticle->m_flStretchY = rainData.raindropStretchY;

	if (rainData.RaindropsAffectedByWind())
	{
		pParticle->m_vVelocity.x = m_vecWind.x * Com_RandomFloat( 1.0f, 2.0f );
		pParticle->m_vVelocity.y = m_vecWind.y * Com_RandomFloat( 1.0f, 2.0f );
	}
	else
	{
		pParticle->m_vVelocity.x = pParticle->m_vVelocity.y = 0.0f;
	}

	pParticle->m_vVelocity.z = -rainData.GetRaindropFallingSpeed();

	int flags = TRI_COLLIDEWORLD | TRI_COLLIDEKILL | TRI_WATERTRACE;
	if (!rainData.CanGoThroughBrushEntities())
		flags |= TRI_COLLIDEBRUSHENTS;
	pParticle->SetCollisionFlags(flags);

	pParticle->m_flGravity = 0;

	pParticle->SetCullFlag( CULL_PVS );
	pParticle->SetLightFlag( rainData.raindropParticleParams.lightFlag );

	pParticle->m_iRendermode = rainData.raindropParticleParams.renderMode;

	pParticle->m_vColor = rainData.raindropParticleParams.color;

	pParticle->m_flDieTime = gEngfuncs.GetClientTime() + rainData.raindropLife;

	return pParticle;
}

CPartWind* CEnvironment::CreateWindParticle( const Vector& vecOrigin, const RainData& rainData )
{
	if( !rainData.windPuffSprite )
	{
		return nullptr;
	}

	CPartWind* pParticle = new CPartWind();

	Vector vecPartOrigin = vecOrigin;

	float particleSize = rainData.windParticleParams.GetSize();

	vecPartOrigin.z += particleSize * 0.3f;

	pParticle->InitializeSprite(
		vecPartOrigin, g_vecZero,
		rainData.windPuffSprite,
		particleSize,
		rainData.windParticleParams.brightness );

	pParticle->m_iNumFrames = rainData.windPuffSprite->numframes;

	strcpy( pParticle->m_szClassname, "wind_particle" );

	//pParticle->m_iFrame = UTIL_RandomLong( m_pGasPuffSprite->numframes / 2, m_pGasPuffSprite->numframes );

	pParticle->m_vVelocity.x = m_vecWind.x / Com_RandomFloat( 1.0f, 2.0f );
	pParticle->m_vVelocity.y = m_vecWind.y / Com_RandomFloat( 1.0f, 2.0f );

	if( Com_RandomFloat( 0.0, 1.0 ) < 0.1 )
	{
		pParticle->m_vVelocity.x *= 0.5;
		pParticle->m_vVelocity.y *= 0.5;
	}

	pParticle->SetCollisionFlags( TRI_COLLIDEWORLD );
	pParticle->m_flGravity = 0;

	pParticle->m_iRendermode = rainData.windParticleParams.renderMode;

	pParticle->SetCullFlag( CULL_PVS );
	pParticle->SetLightFlag( rainData.windParticleParams.lightFlag );
	pParticle->SetRenderFlag( RENDER_FACEPLAYER );

	pParticle->m_vAVelocity.z = Com_RandomFloat( -1.0, 1.0 );

	pParticle->m_flScaleSpeed = 0.4;
	pParticle->m_flDampingTime = 0;

	pParticle->m_iFrame = 0;

	pParticle->m_flMass = 1.0f;
	pParticle->m_flBounceFactor = 0;
	pParticle->m_vColor = rainData.windParticleParams.color;

	pParticle->m_flFadeSpeed = -1.0f;

	pParticle->m_flDieTime = gEngfuncs.GetClientTime() + rainData.windpuffLife;

	return pParticle;
}

void CEnvironment::CreateSnowFlake( const Vector& vecOrigin, const SnowData& snowData )
{
	if( !snowData.snowSprite )
	{
		return;
	}

	CPartSnowFlake* pParticle = new CPartSnowFlake();
	pParticle->m_targetBrightness = snowData.snowflakeParticleParams.brightness;

	pParticle->InitializeSprite(
		vecOrigin, g_vecZero,
		snowData.snowSprite,
		snowData.snowflakeParticleParams.GetSize(), snowData.snowflakeInitialBrightness );

	strcpy( pParticle->m_szClassname, "snow_particle" );

	pParticle->m_iNumFrames = snowData.snowSprite->numframes;

	if (snowData.SnowflakesAffectedByWind())
	{
		pParticle->m_vVelocity.x = m_vecWind.x / Com_RandomFloat( 1.0, 2.0 );
		pParticle->m_vVelocity.y = m_vecWind.y / Com_RandomFloat( 1.0, 2.0 );
	}
	else
	{
		pParticle->m_vVelocity.x = pParticle->m_vVelocity.y = 0.0f;
	}

	pParticle->m_vVelocity.z = -snowData.GetSnowflakeFallingSpeed();

	int flags = TRI_COLLIDEWORLD;
	if (!snowData.CanGoThroughBrushEntities())
		flags |= TRI_COLLIDEBRUSHENTS;
	pParticle->SetCollisionFlags(flags);

	const float flFrac = Com_RandomFloat( 0.0, 1.0 );

	if( flFrac >= 0.1 )
	{
		if( flFrac < 0.2 )
		{
			pParticle->m_vVelocity.z = -65.0;
		}
		else if( flFrac < 0.3 )
		{
			pParticle->m_vVelocity.z = -75.0;
		}
	}
	else
	{
		pParticle->m_vVelocity.x *= 0.5;
		pParticle->m_vVelocity.y *= 0.5;
	}

	pParticle->m_iRendermode = snowData.snowflakeParticleParams.renderMode;

	pParticle->SetCullFlag( CULL_PVS );
	pParticle->SetLightFlag( snowData.snowflakeParticleParams.lightFlag );
	pParticle->SetRenderFlag( RENDER_FACEPLAYER );

	pParticle->m_flScaleSpeed = 0;
	pParticle->m_flDampingTime = 0;
	pParticle->m_iFrame = 0;
	pParticle->m_flMass = 1.0;

	pParticle->m_flGravity = 0;
	pParticle->m_flBounceFactor = 0;

	pParticle->m_vColor = snowData.snowflakeParticleParams.color;

	pParticle->m_flDieTime = gEngfuncs.GetClientTime() + snowData.snowflakeLife;

	pParticle->m_bSpiral = Com_RandomLong( 0, 1 ) != 0;

	pParticle->m_flSpiralTime = gEngfuncs.GetClientTime() + Com_RandomLong( 2, 4 );
}

static void ReadWeatherData(WeatherData& data)
{
	const short intensity = READ_SHORT();
	if (intensity > 0)
		data.intensity = intensity;

	const float updatePeriod = READ_SHORT() / 100.0f;
	if (updatePeriod > 0)
		data.updatePeriod = updatePeriod;

	if (data.IsVolume())
	{
		data.absmin = READ_VECTOR();
		data.absmax = READ_VECTOR();
	}
	else
	{
		const short distance = READ_SHORT();
		if (distance > 0)
			data.distance = distance;

		const short minHeight = READ_SHORT();
		if (minHeight > 0)
			data.minHeight = minHeight;

		const short maxHeight = READ_SHORT();
		if (maxHeight > 0)
			data.maxHeight = maxHeight;

		if (data.IsLocalizedPoint())
		{
			data.location = READ_VECTOR();
		}
	}
}

int CEnvironment::MsgFunc_Rain(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	const int entIndex = READ_LONG();
	const int rainFlags = READ_SHORT();

	if (!(rainFlags & SF_RAIN_ACTIVE))
	{
		RemoveRain(entIndex);
		return 1;
	}

	RainData rainData;
	rainData.entIndex = entIndex;
	rainData.flags = rainFlags;

	ReadWeatherData(rainData);

	// Raindrops
	{
		const short raindropWidth = READ_SHORT();
		const short raindropHeight = READ_SHORT();

		if (raindropWidth > 0)
		{
			rainData.raindropParticleParams.minSize = rainData.raindropParticleParams.maxSize = raindropWidth;
		}
		if (raindropHeight > 0)
		{
			const float raindropSize = raindropWidth > 0 ? raindropWidth : rainData.raindropParticleParams.minSize;
			rainData.raindropStretchY = raindropHeight / raindropSize;
		}

		const int raindropRenderMode = READ_BYTE();
		if (raindropRenderMode > 0)
			rainData.raindropParticleParams.renderMode = raindropRenderMode;

		const int raindropBrightness = READ_BYTE();
		if (raindropBrightness > 0)
			rainData.raindropParticleParams.brightness = raindropBrightness;

		const int raindropRed = READ_BYTE();
		const int raindropGreen = READ_BYTE();
		const int raindropBlue = READ_BYTE();

		if (raindropRed + raindropGreen + raindropBlue > 0)
			rainData.raindropParticleParams.color = Vector(raindropRed, raindropGreen, raindropBlue);

		const int rainLightMode = READ_BYTE();
		if (rainLightMode)
			rainData.raindropParticleParams.lightFlag = ParticleLightModeToFlag(rainLightMode);

		const int raindropMinSpeed = READ_SHORT();
		const int raindropMaxSpeed = READ_SHORT();

		if (raindropMinSpeed)
			rainData.raindropMinSpeed = raindropMinSpeed;
		if (raindropMaxSpeed)
			rainData.raindropMaxSpeed = raindropMaxSpeed;

		const float raindropLife = READ_SHORT() / 100.0f;
		if (raindropLife > 0)
			rainData.raindropLife = raindropLife;
	}

	// Wind puffs
	{
		const short windpuffMinSize = READ_SHORT();
		if (windpuffMinSize > 0)
			rainData.windParticleParams.minSize = windpuffMinSize;

		const short windpuffMaxSize = READ_SHORT();
		if (windpuffMaxSize > 0)
			rainData.windParticleParams.maxSize = windpuffMaxSize;

		const int windPuffRenderMode = READ_BYTE();
		if (windPuffRenderMode > 0)
			rainData.windParticleParams.renderMode = windPuffRenderMode;

		const int windPuffBrightness = READ_BYTE();
		if (windPuffBrightness > 0)
			rainData.windParticleParams.brightness = windPuffBrightness;

		const int windPuffRed = READ_BYTE();
		const int windPuffGreen = READ_BYTE();
		const int windPuffBlue = READ_BYTE();

		if (windPuffRed + windPuffGreen + windPuffBlue > 0)
			rainData.windParticleParams.color = Vector(windPuffRed, windPuffGreen, windPuffBlue);

		const int windPuffLightMode = READ_BYTE();
		if (windPuffLightMode)
			rainData.windParticleParams.lightFlag = ParticleLightModeToFlag(windPuffLightMode);

		const float windpuffLife = READ_SHORT() / 100.0f;
		if (windpuffLife > 0)
			rainData.windpuffLife = windpuffLife;
	}

	// Rain splash
	{
		const short splashMinSize = READ_SHORT();
		if (splashMinSize > 0)
			rainData.splashParticleParams.minSize = splashMinSize;

		const short splashMaxSize = READ_SHORT();
		if (splashMaxSize > 0)
			rainData.splashParticleParams.maxSize = splashMaxSize;

		const int splashRenderMode = READ_BYTE();
		if (splashRenderMode > 0)
			rainData.splashParticleParams.renderMode = splashRenderMode;

		const int splashBrightness = READ_BYTE();
		if (splashBrightness > 0)
			rainData.splashParticleParams.brightness = splashBrightness;

		const int splashRed = READ_BYTE();
		const int splashGreen = READ_BYTE();
		const int splashBlue = READ_BYTE();

		if (splashRed + splashGreen + splashBlue > 0)
			rainData.splashParticleParams.color = Vector(splashRed, splashGreen, splashBlue);

		const int splashLightMode = READ_BYTE();
		if (splashLightMode)
			rainData.splashParticleParams.lightFlag = ParticleLightModeToFlag(splashLightMode);
	}

	// Ripple
	{
		const short rippleSize = READ_SHORT();
		if (rippleSize > 0)
			rainData.rippleParticleParams.minSize = rainData.rippleParticleParams.maxSize = rippleSize;

		const int rippleRenderMode = READ_BYTE();
		if (rippleRenderMode > 0)
			rainData.rippleParticleParams.renderMode = rippleRenderMode;

		const int rippleBrightness = READ_BYTE();
		if (rippleBrightness > 0)
			rainData.rippleParticleParams.brightness = rippleBrightness;

		const int rippleRed = READ_BYTE();
		const int rippleGreen = READ_BYTE();
		const int rippleBlue = READ_BYTE();

		if (rippleRed + rippleGreen + rippleBlue > 0)
			rainData.rippleParticleParams.color = Vector(rippleRed, rippleGreen, rippleBlue);

		const int rippleLightMode = READ_BYTE();
		if (rippleLightMode)
			rainData.rippleParticleParams.lightFlag = ParticleLightModeToFlag(rippleLightMode);
	}

	// Sprites

	const char* raindropSprite = READ_STRING();
	if (raindropSprite && *raindropSprite)
	{
		rainData.rainSprite = LoadSprite(raindropSprite);
	}
	else
	{
		if (!m_pRainSprite)
			m_pRainSprite = LoadSprite(RAINDROP_DEFAULT_SPRITE);
		rainData.rainSprite = m_pRainSprite;
	}

	const char* windpuffSprite = READ_STRING();
	if (rainData.WindPuffsAllowed())
	{
		if (windpuffSprite && *windpuffSprite)
		{
			rainData.windPuffSprite = LoadSprite(windpuffSprite);
		}
		else
		{
			if (!m_pGasPuffSprite)
				m_pGasPuffSprite = LoadSprite(WINDPUFF_DEFAULT_SPRITE);
			rainData.windPuffSprite = m_pGasPuffSprite;
		}
	}

	const char* splashSprite = READ_STRING();
	if (rainData.SplashesAllowed())
	{
		if (splashSprite && *splashSprite)
		{
			rainData.splashSprite = LoadSprite(splashSprite);
		}
		else
		{
			if (!m_pRainSplash)
				m_pRainSplash = LoadSprite(SPLASH_DEFAULT_SPRITE);
			rainData.splashSprite = m_pRainSplash;
		}
	}

	const char* rippleSprite = READ_STRING();
	if (rainData.RipplesAllowed())
	{
		if (rippleSprite && *rippleSprite)
		{
			rainData.rippleSprite = LoadSprite(rippleSprite);
		}
		else
		{
			if (!m_pRipple)
				m_pRipple = LoadSprite(RIPPLE_DEFAULT_SPRITE);
			rainData.rippleSprite = m_pRipple;
		}
	}

	AddRain(rainData);

	return 1;
}

int CEnvironment::MsgFunc_Snow(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	const int entIndex = READ_LONG();
	const int snowFlags = READ_SHORT();

	if (!(snowFlags & SF_SNOW_ACTIVE))
	{
		RemoveSnow(entIndex);
		return 1;
	}

	SnowData snowData;
	snowData.entIndex = entIndex;
	snowData.flags = snowFlags;

	ReadWeatherData(snowData);

	// Snowflakes
	{
		const short snowflakeMinSize = READ_SHORT();
		if (snowflakeMinSize > 0)
			snowData.snowflakeParticleParams.minSize = snowflakeMinSize;

		const short snowflakeMaxSize = READ_SHORT();
		if (snowflakeMaxSize > 0)
			snowData.snowflakeParticleParams.maxSize = snowflakeMaxSize;

		const int snowflakeRenderMode = READ_BYTE();
		if (snowflakeRenderMode > 0)
			snowData.snowflakeParticleParams.renderMode = snowflakeRenderMode;

		const int snowflakeBrightness = READ_BYTE();
		if (snowflakeBrightness > 0)
			snowData.snowflakeParticleParams.brightness = snowflakeBrightness;

		const int snowflakeRed = READ_BYTE();
		const int snowflakeGreen = READ_BYTE();
		const int snowflakeBlue = READ_BYTE();

		if (snowflakeRed + snowflakeGreen + snowflakeBlue > 0)
			snowData.snowflakeParticleParams.color = Vector(snowflakeRed, snowflakeGreen, snowflakeBlue);

		const int snowLightMode = READ_BYTE();
		if (snowLightMode)
			snowData.snowflakeParticleParams.lightFlag = ParticleLightModeToFlag(snowLightMode);

		const int snowflakeMinSpeed = READ_SHORT();
		const int snowflakeMaxSpeed = READ_SHORT();

		if (snowflakeMinSpeed)
			snowData.snowflakeMinSpeed = snowflakeMinSpeed;
		if (snowflakeMaxSpeed)
			snowData.snowflakeMaxSpeed = snowflakeMaxSpeed;

		const float snowflakeLife = READ_SHORT() / 100.0f;
		if (snowflakeLife > 0)
			snowData.snowflakeLife = snowflakeLife;
	}

	const char* snowflakeSprite = READ_STRING();
	if (snowflakeSprite && *snowflakeSprite)
	{
		snowData.snowSprite = LoadSprite(snowflakeSprite);
	}
	else
	{
		if (!m_pSnowSprite)
			m_pSnowSprite = LoadSprite(SNOWFLAKE_DEFAULT_SPRITE);
		snowData.snowSprite = m_pSnowSprite;
	}

	AddSnow(snowData);

	return 1;
}

int CEnvironment::MsgFunc_WeatherPos(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	const int entIndex = READ_LONG();

	auto rainIt = std::find_if(m_rains.begin(), m_rains.end(), [&entIndex](const RainData& data) {
		return data.entIndex == entIndex;
	});
	auto snowIt = std::find_if(m_snows.begin(), m_snows.end(), [&entIndex](const SnowData& data) {
		return data.entIndex == entIndex;
	});

	const bool isBrush = READ_BYTE() != 0;

	WeatherData* weatherData = nullptr;
	if (rainIt != m_rains.end())
	{
		weatherData = &*rainIt;
	}
	else if (snowIt != m_snows.end())
	{
		weatherData = &*snowIt;
	}

	if (weatherData)
	{
		if (isBrush)
		{
			weatherData->absmin = READ_VECTOR();
			weatherData->absmax = READ_VECTOR();
		}
		else
		{
			weatherData->location = READ_VECTOR();
		}
	}

	return 1;
}

void CEnvironment::RemoveRain(int entIndex)
{
	auto it = std::find_if(m_rains.begin(), m_rains.end(), [&](const RainData& data) {
		return data.entIndex == entIndex;
	});
	if (it != m_rains.end())
		m_rains.erase(it);
}

void CEnvironment::AddRain(const RainData &rainData)
{
	auto it = std::find_if(m_rains.begin(), m_rains.end(), [&](const RainData& data) {
		return data.entIndex == rainData.entIndex;
	});
	if (it != m_rains.end())
	{
		gEngfuncs.Con_Printf("Rain with index %d already exists!\n", rainData.entIndex);
		return;
	}
	m_rains.push_back(rainData);
}

void CEnvironment::RemoveSnow(int entIndex)
{
	auto it = std::find_if(m_snows.begin(), m_snows.end(), [&](const SnowData& data) {
		return data.entIndex == entIndex;
	});
	if (it != m_snows.end())
		m_snows.erase(it);
}

void CEnvironment::AddSnow(const SnowData &snowData)
{
	auto it = std::find_if(m_snows.begin(), m_snows.end(), [&](const SnowData& data) {
		return data.entIndex == snowData.entIndex;
	});
	if (it != m_snows.end())
	{
		gEngfuncs.Con_Printf("Snow with index %d already exists!\n", snowData.entIndex);
		return;
	}
	m_snows.push_back(snowData);
}

model_t* CEnvironment::LoadSprite(const char* spriteName)
{
	return const_cast<model_t*>(gEngfuncs.GetSpritePointer(gEngfuncs.pfnSPR_Load(spriteName)));
}
