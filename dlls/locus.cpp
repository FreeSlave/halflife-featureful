//=========================================
// NEW file for Spirit of Half-Life 0.7
// Created 14/01/02
//=========================================

// Spirit of Half-Life's particle system uses "locus triggers" to tell
// entities where to perform their actions.

#include "util.h"
#include "cbase.h"
#include "locus.h"
#include "effects.h"
#include "decals.h"

bool IsLikelyNumber(const char* szText)
{
	return (*szText >= '0' && *szText <= '9') || *szText == '-';
}

bool TryCalcLocus_Position(CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText, Vector& result, bool showError)
{
	if (IsLikelyNumber(szText))
	{ // it's a vector
		Vector tmp;
		UTIL_StringToRandomVector( (float *)tmp, szText );
		result = tmp;
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);
	if (pCalc != NULL)
	{
		return pCalc->CalcPosition( pLocus, &result );
	}

	if (showError)
	{
		const char* requesterClassname = pEntity ? STRING(pEntity->pev->classname) : "";
		const char* requesterTargetname = pEntity ? STRING(pEntity->pev->targetname) : "";
		ALERT(at_error, "%s \"%s\" has bad or missing calc_position value \"%s\"\n", requesterClassname, requesterTargetname, szText);
	}
	return false;
}

bool TryCalcLocus_Velocity(CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText, Vector& result, bool showError)
{
	if (IsLikelyNumber(szText))
	{ // it's a vector
		Vector tmp;
		UTIL_StringToRandomVector( (float *)tmp, szText );
		result = tmp;
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);
	if (pCalc != NULL)
	{
		return pCalc->CalcVelocity( pLocus, &result );
	}

	if (showError)
	{
		const char* requesterClassname = pEntity ? STRING(pEntity->pev->classname) : "";
		const char* requesterTargetname = pEntity ? STRING(pEntity->pev->targetname) : "";
		ALERT(at_error, "%s \"%s\" has bad or missing calc_velocity value \"%s\"\n", requesterClassname, requesterTargetname, szText);
	}
	return false;
}

bool TryCalcLocus_Ratio(CBaseEntity *pLocus, const char *szText, float& result, bool showError)
{
	if (IsLikelyNumber(szText))
	{ // assume it's a float
		result = atof( szText );
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);

	if (pCalc != NULL)
	{
		return pCalc->CalcRatio( pLocus, &result );
	}

	if (showError)
	{
		ALERT(at_error, "Bad or missing calc_ratio entity \"%s\"\n", szText);
	}
	return false;
}

bool TryCalcLocus_Color(CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText, Vector& result, bool showError)
{
	if (IsLikelyNumber(szText))
	{ // it's a vector
		Vector tmp;
		UTIL_StringToRandomVector( (float *)tmp, szText );
		result = tmp;
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);
	if (pCalc != NULL)
	{
		result = pCalc->pev->rendercolor;
		return true;
	}

	if (showError)
	{
		const char* requesterClassname = pEntity ? STRING(pEntity->pev->classname) : "";
		const char* requesterTargetname = pEntity ? STRING(pEntity->pev->targetname) : "";
		ALERT(at_error, "%s \"%s\" has bad or missing color value \"%s\"\n", requesterClassname, requesterTargetname, szText);
	}
	return false;
}

//=============================================
//locus_x effects
//=============================================
#if 0
// Entity variable
class CLocusAlias : public CBaseAlias
{
public:
	void	PostSpawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	CBaseEntity		*FollowAlias( CBaseEntity *pFrom );
	void	FlushChanges( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE	m_hValue;
	EHANDLE m_hChangeTo;
};

TYPEDESCRIPTION	CLocusAlias::m_SaveData[] = 
{
	DEFINE_FIELD( CLocusAlias, m_hValue, FIELD_EHANDLE),
	DEFINE_FIELD( CLocusAlias, m_hChangeTo, FIELD_EHANDLE),
};

LINK_ENTITY_TO_CLASS( locus_alias, CLocusAlias );
IMPLEMENT_SAVERESTORE( CLocusAlias, CBaseAlias );

void CLocusAlias::PostSpawn( void )
{
	m_hValue = UTIL_FindEntityByTargetname( NULL, STRING(pev->netname) );
}

void CLocusAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hChangeTo = pActivator;
	UTIL_AddToAliasList( this );
}

void CLocusAlias::FlushChanges( void )
{
	m_hValue = m_hChangeTo;
	m_hChangeTo = NULL;
}

CBaseEntity *CLocusAlias::FollowAlias( CBaseEntity *pFrom )
{
	if (m_hValue == 0)
		return NULL;
	else if ( pFrom == NULL || (OFFSET(m_hValue->pev) > OFFSET(pFrom->pev)) )
	{
//		ALERT(at_console, "LocusAlias returns %s:  %f %f %f\n", STRING(m_pValue->pev->targetname), m_pValue->pev->origin.x, m_pValue->pev->origin.y, m_pValue->pev->origin.z);
		return m_hValue;
	}
	else
		return NULL;
}
#endif



// Beam maker
#define BEAM_FSINE		0x10
#define BEAM_FSOLID		0x20
#define BEAM_FSHADEIN	0x40
#define BEAM_FSHADEOUT	0x80

#define SF_LBEAM_SHADEIN	128
#define SF_LBEAM_SHADEOUT	256
#define SF_LBEAM_SOLID		512
#define SF_LBEAM_SINE		1024

class CLocusBeam : public CPointEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszSprite;
	string_t m_iszTargetName;
	string_t m_iszStart;
	string_t m_iszEnd;
	int		m_iWidth;
	int		m_iDistortion;
	float	m_fFrame;
	int		m_iScrollRate;
	float	m_fDuration;
	float	m_fDamage;
	int		m_iDamageType;
	int		m_iFlags;
	byte	m_iStartAttachment;
	byte	m_iEndAttachment;
};

TYPEDESCRIPTION	CLocusBeam::m_SaveData[] = 
{
	DEFINE_FIELD( CLocusBeam, m_iszSprite, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iszTargetName, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iszStart, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iszEnd, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iWidth, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_iDistortion, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_fFrame, FIELD_FLOAT),
	DEFINE_FIELD( CLocusBeam, m_iScrollRate, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_fDuration, FIELD_FLOAT),
	DEFINE_FIELD( CLocusBeam, m_fDamage, FIELD_FLOAT),
	DEFINE_FIELD( CLocusBeam, m_iDamageType, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_iFlags, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_iStartAttachment, FIELD_CHARACTER),
	DEFINE_FIELD( CLocusBeam, m_iEndAttachment, FIELD_CHARACTER),
};

LINK_ENTITY_TO_CLASS( locus_beam, CLocusBeam )
IMPLEMENT_SAVERESTORE(CLocusBeam, CPointEntity)

void CLocusBeam::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszSprite"))
	{
		m_iszSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTargetName"))
	{
		m_iszTargetName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszStart"))
	{
		m_iszStart = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEnd"))
	{
		m_iszEnd = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iWidth"))
	{
		m_iWidth = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iDistortion"))
	{
		m_iDistortion = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fFrame"))
	{
		m_fFrame = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iScrollRate"))
	{
		m_iScrollRate = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fDuration"))
	{
		m_fDuration = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fDamage"))
	{
		m_fDamage = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iDamageType"))
	{
		m_iDamageType = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartAttachment"))
	{
		m_iStartAttachment = atoi(pkvd->szValue);
		if (m_iStartAttachment > 4 || m_iStartAttachment < 0)
			m_iStartAttachment = 0;
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iEndAttachment"))
	{
		m_iEndAttachment = atoi(pkvd->szValue);
		if (m_iEndAttachment > 4 || m_iEndAttachment < 0)
			m_iEndAttachment = 0;
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CLocusBeam::Precache ( void )
{
	PRECACHE_MODEL ( STRING(m_iszSprite) );
}

void CLocusBeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pStartEnt;
	CBaseEntity *pEndEnt;
	Vector vecStartPos;
	Vector vecEndPos;
	CBeam *pBeam;

	switch(pev->impulse)
	{
	case 0: // ents
		pStartEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszStart), pActivator);
		pEndEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEnd), pActivator);

		if (pStartEnt == NULL || pEndEnt == NULL)
			return;
		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->EntsInit( pStartEnt->entindex(), pEndEnt->entindex(), m_iStartAttachment, m_iEndAttachment );
		break;

	case 1: // pointent
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszStart), vecStartPos ))
			return;

		pEndEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEnd), pActivator);

		if (pEndEnt == NULL)
			return;
		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->PointEntInit( vecStartPos, pEndEnt->entindex(), m_iEndAttachment );
		break;
	case 2: // points
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszStart), vecStartPos ) || !TryCalcLocus_Position( this, pActivator, STRING(m_iszEnd), vecEndPos )) {
			return;
		}

		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->PointsInit( vecStartPos, vecEndPos );
		break;
	case 3: // point & offset
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszStart), vecStartPos ) || !TryCalcLocus_Velocity( this, pActivator, STRING(m_iszEnd), vecEndPos ))
			return;

		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->PointsInit( vecStartPos, vecStartPos + vecEndPos );
		break;
	default:
		ALERT(at_error, "%s: unknown 'Start and End' type. Refusing to spawn a beam\n", STRING(pev->classname));
		return;
	}
	pBeam->SetColor( pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z );
	pBeam->SetBrightness( pev->renderamt );
	pBeam->SetNoise( m_iDistortion );
	pBeam->SetFrame( m_fFrame );
	pBeam->SetScrollRate( m_iScrollRate );
	pBeam->SetFlags( m_iFlags );
	pBeam->pev->dmg = m_fDamage;
	pBeam->pev->frags = m_iDamageType;
	pBeam->pev->spawnflags |= pev->spawnflags & (SF_BEAM_RING |
			SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND | SF_BEAM_DECALS);
	if (m_fDuration)
	{
		pBeam->SetThink(&CBeam:: SUB_Remove );
		pBeam->pev->nextthink = gpGlobals->time + m_fDuration;
	}
	pBeam->pev->targetname = m_iszTargetName;

	if (pev->target)
	{
		FireTargets( STRING(pev->target), pBeam, this );
	}
}

void CLocusBeam::Spawn( void )
{
	Precache();
	m_iFlags = 0;
	if (pev->spawnflags & SF_LBEAM_SHADEIN)
		m_iFlags |= BEAM_FSHADEIN;
	if (pev->spawnflags & SF_LBEAM_SHADEOUT)
		m_iFlags |= BEAM_FSHADEOUT;
	if (pev->spawnflags & SF_LBEAM_SINE)
		m_iFlags |= BEAM_FSINE;
	if (pev->spawnflags & SF_LBEAM_SOLID)
		m_iFlags |= BEAM_FSOLID;
}

//=============================================
//calc_x entities
//=============================================

class CCalcPosition : public CPointEntity
{
public:
	bool CalcPosition( CBaseEntity *pLocus, Vector* outVector );
};

LINK_ENTITY_TO_CLASS( calc_position, CCalcPosition )

bool CCalcPosition::CalcPosition( CBaseEntity *pLocus, Vector* outVector )
{
	CBaseEntity *pSubject = UTIL_FindEntityByTargetname(NULL, STRING(pev->netname), pLocus);
	if ( !pSubject )
	{
		ALERT(at_console, "%s \"%s\" failed to find target entity \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->netname) );
		return false;
	}

	Vector vecOffset = g_vecZero;

	if (pev->message) {
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->message), vecOffset)) {
			ALERT(at_console, "%s \"%s\" failed, bad LV \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->message) );
			return false;
		}
	}

	CBaseAnimating* pAnimatingSubject = nullptr;
	if (pev->impulse >= 5 && pev->impulse <= 8)
	{
		pAnimatingSubject = pSubject->MyAnimatingPointer();
		if (!pAnimatingSubject)
		{
			ALERT(at_console, "%s \"%s\" can't get attachment points from non-animating entity\n");
			return false;
		}
	}

	Vector vecPosition;
	Vector vecJunk;

	switch (pev->impulse)
	{
	case 1: //eyes
		*outVector = vecOffset + pSubject->EyePosition();
		//ALERT(at_console, "calc_subpos returns %f %f %f\n", vecResult.x, vecResult.y, vecResult.z);
		break;
		//return vecOffset + pLocus->EyePosition();
	case 2: // top
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			(pSubject->pev->mins.x + pSubject->pev->maxs.x)/2,
			(pSubject->pev->mins.y + pSubject->pev->maxs.y)/2,
			pSubject->pev->maxs.z
		);
		break;
	case 3: // centre
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			(pSubject->pev->mins.x + pSubject->pev->maxs.x)/2,
			(pSubject->pev->mins.y + pSubject->pev->maxs.y)/2,
			(pSubject->pev->mins.z + pSubject->pev->maxs.z)/2
		);
		break;
	case 4: // bottom
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			(pSubject->pev->mins.x + pSubject->pev->maxs.x)/2,
			(pSubject->pev->mins.y + pSubject->pev->maxs.y)/2,
			pSubject->pev->mins.z
		);
		break;
	case 5:
		pAnimatingSubject->GetAttachment( 0, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 6:
		pAnimatingSubject->GetAttachment( 1, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 7:
		pAnimatingSubject->GetAttachment( 2, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 8:
		pAnimatingSubject->GetAttachment( 3, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 9:
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			RANDOM_FLOAT(pSubject->pev->mins.x, pSubject->pev->maxs.x),
			RANDOM_FLOAT(pSubject->pev->mins.y, pSubject->pev->maxs.y),
			RANDOM_FLOAT(pSubject->pev->mins.z, pSubject->pev->maxs.z)
		);
		break;
	case 10:
		if (pSubject == this)
		{
			ALERT(at_warning, "Recursion: %s refers to its own CalcPosition\n", STRING(pev->targetname));
			return false;
		}
		if (!pSubject->CalcPosition(pLocus, outVector))
			return false;
		break;
	default:
		*outVector = vecOffset + pSubject->pev->origin;
		break;
	}
	return true;
}

//=======================================================

static bool ClampToMinMax(CBaseEntity* pLocus, float& fBasis, string_t iszMin, string_t iszMax, int clampPolicy)
{
	if (!FStringNull(iszMin))
	{
		float fMin = 0;
		if (!TryCalcLocus_Ratio( pLocus, STRING(iszMin), fMin))
			return false;

		if (!FStringNull(iszMax))
		{
			float fMax = 0;
			if (!TryCalcLocus_Ratio( pLocus, STRING(iszMax), fMax))
				return false;

			if (fBasis >= fMin && fBasis <= fMax) {
				return true;
			}
			switch (clampPolicy)
			{
			case 0:
				if (fBasis < fMin)
					fBasis = fMin;
				else
					fBasis = fMax;
				return true;
			case 1:
				while (fBasis < fMin)
					fBasis += fMax - fMin;
				while (fBasis > fMax)
					fBasis -= fMax - fMin;
				return true;
			case 2:
				while (fBasis < fMin || fBasis > fMax)
				{
					if (fBasis < fMin)
						fBasis = fMin + fMax - fBasis;
					else
						fBasis = fMax + fMax - fBasis;
				}
				return true;
			}
		}

		if (fBasis < fMin)
			fBasis = fMin; // crop to nearest value
	}
	else if (!FStringNull(iszMax))
	{
		float fMax = 0;
		if (!TryCalcLocus_Ratio( pLocus, STRING(iszMax), fMax))
			return false;

		if (fBasis > fMax)
			fBasis = fMax; // crop to nearest value
	}
	return true;
}

enum
{
	CALCRATIO_TRANSFORM_NONE = 0,
	CALCRATIO_TRANSFORM_REVERSED,
	CALCRATIO_TRANSFORM_NEGATIVE,
	CALCRATIO_TRANSFORM_RECIPROCAL,
	CALCRATIO_TRANSFORM_SQUARE,
	CALCRATIO_TRANSFORM_INVERSESQUARE,
	CALCRATIO_TRANSFORM_SQUAREROOT,
	CALCRATIO_TRANSFORM_COSINE,
	CALCRATIO_TRANSFORM_SINE,
	CALCRATIO_TRANSFORM_TANGENT,
	CALCRATIO_TRANSFORM_ACOSINE,
	CALCRATIO_TRANSFORM_ASINE,
	CALCRATIO_TRANSFORM_ATANGENT,
};

enum
{
	CALC_CASTTOINT_NO = 0,
	CALC_CASTTOINT_ROUND = 1,
	CALC_CASTTOINT_FLOOR = 2,
	CALC_CASTTOINT_CEIL = 3,
	CALC_CASTTOINT_TRUNC = 4,
};

static float ApplyCastMode(float f, byte castMode)
{
	switch(castMode)
	{
	case CALC_CASTTOINT_ROUND:
		return round(f);
	case CALC_CASTTOINT_FLOOR:
		return floor(f);
	case CALC_CASTTOINT_CEIL:
		return ceil(f);
	case CALC_CASTTOINT_TRUNC:
		return trunc(f);
	case CALC_CASTTOINT_NO:
	default:
		return f;
	}
}

class CCalcRatio : public CPointEntity
{
public:
	bool CalcRatio( CBaseEntity *pLocus, float* outResult );

	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMin;
	string_t m_iszMax;
	byte m_baseCastMode;
	byte m_resultCastMode;
};

LINK_ENTITY_TO_CLASS( calc_ratio, CCalcRatio )

TYPEDESCRIPTION	CCalcRatio::m_SaveData[] =
{
	DEFINE_FIELD( CCalcRatio, m_iszMin, FIELD_STRING ),
	DEFINE_FIELD( CCalcRatio, m_iszMax, FIELD_STRING ),
	DEFINE_FIELD( CCalcRatio, m_baseCastMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CCalcRatio, m_resultCastMode, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CCalcRatio, CPointEntity )

void CCalcRatio::Spawn()
{
	m_iszMin = pev->noise;
	pev->noise = iStringNull;
	m_iszMax = pev->noise1;
	pev->noise1 = iStringNull;
}

void CCalcRatio::KeyValue(KeyValueData *pkvd)
{
	if(FStrEq(pkvd->szKeyName, "base_cast_mode"))
	{
		m_baseCastMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "result_cast_mode"))
	{
		m_resultCastMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

bool CCalcRatio::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	float fBasis = 0;
	if (!TryCalcLocus_Ratio( pLocus, STRING(pev->target), fBasis)) {
		return false;
	}

	fBasis = ApplyCastMode(fBasis, m_baseCastMode);

	switch (pev->impulse)
	{
	case CALCRATIO_TRANSFORM_REVERSED:	fBasis = 1-fBasis; break;
	case CALCRATIO_TRANSFORM_NEGATIVE:	fBasis = -fBasis; break;
	case CALCRATIO_TRANSFORM_RECIPROCAL: fBasis = 1/fBasis; break;
	case CALCRATIO_TRANSFORM_SQUARE: fBasis = fBasis * fBasis; break;
	case CALCRATIO_TRANSFORM_INVERSESQUARE: fBasis = 1 / (fBasis * fBasis); break;
	case CALCRATIO_TRANSFORM_SQUAREROOT: fBasis = sqrt(fBasis); break;
	case CALCRATIO_TRANSFORM_COSINE: fBasis = cos(fBasis * (M_PI / 180.0f)); break;
	case CALCRATIO_TRANSFORM_SINE: fBasis = sin(fBasis * (M_PI / 180.0f)); break;
	case CALCRATIO_TRANSFORM_TANGENT: fBasis = tan(fBasis * (M_PI / 180.0f)); break;
	case CALCRATIO_TRANSFORM_ACOSINE: fBasis = acos(fBasis) * (180.0f / M_PI); break;
	case CALCRATIO_TRANSFORM_ASINE: fBasis = asin(fBasis) * (180.0f / M_PI); break;
	case CALCRATIO_TRANSFORM_ATANGENT: fBasis = atan(fBasis) * (180.0f / M_PI); break;
	}

	if (!FStringNull(pev->netname)) {
		float fTmp = 0;
		if (!TryCalcLocus_Ratio( pLocus, STRING(pev->netname), fTmp))
			return false;
		fBasis += fTmp;
	}
	if (!FStringNull(pev->message)) {
		float fTmp = 1;
		if (!TryCalcLocus_Ratio( pLocus, STRING(pev->message), fTmp))
			return false;
		fBasis = fBasis * fTmp;
	}

	if (!ClampToMinMax(pLocus, fBasis, m_iszMin, m_iszMax, (int)pev->frags))
		return false;

	fBasis = ApplyCastMode(fBasis, m_resultCastMode);
	*outResult = fBasis;
	return true;
}

enum
{
	CALCVECTOR_VELOCITY = 0,
	CALCVECTOR_POSITION = 1,
	CALCVECTOR_COLOR = 2,
};

static bool TryCalcLocus_Vector(CBaseEntity* pEntity, CBaseEntity* pLocus, const char* szText, Vector& result, int requestedVectorType)
{
	switch (requestedVectorType) {
	case CALCVECTOR_POSITION:
		return TryCalcLocus_Position(pEntity, pLocus, szText, result);
	case CALCVECTOR_COLOR:
		return TryCalcLocus_Color(pEntity, pLocus, szText, result);
	case CALCVECTOR_VELOCITY:
	default:
		return TryCalcLocus_Velocity(pEntity, pLocus, szText, result);
	}
}

class CCalcNumFromVec : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "vector_type"))
		{
			pev->weapons = atoi(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
	bool CalcRatio( CBaseEntity *pLocus, float* outResult );
};

LINK_ENTITY_TO_CLASS( calc_numfromvec, CCalcNumFromVec )

bool CCalcNumFromVec::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	if ( FStringNull(pev->target) )
	{
		ALERT(at_error, "No base vector given for %s \"%s\"\n", STRING(pev->classname), STRING(pev->targetname));
		return false;
	}

	Vector vecA;
	if (!TryCalcLocus_Vector(this, pLocus, STRING(pev->target), vecA, pev->weapons))
	{
		ALERT(at_error, "%s: Couldn't get the vector value from %s\n", STRING(pev->classname), STRING(pev->target));
		return false;
	}

	switch(pev->impulse)
	{
	case 0: // X
		*outResult = vecA.x; return true;
	case 1: // Y
		*outResult = vecA.y; return true;
	case 2: // Z
		*outResult = vecA.z; return true;
	case 3: // Length
		*outResult = vecA.Length(); return true;
	case 4: // Pitch
		{
			Vector ang = UTIL_VecToAngles(vecA);
			*outResult = ang.x;
			return true;
		}
	case 5: // Yaw
		{
			Vector ang = UTIL_VecToAngles(vecA);
			*outResult = ang.y;
			return true;
		}
	}

	ALERT(at_console, "%s \"%s\" doesn't understand mode %d\n", STRING(pev->classname), STRING(pev->targetname), pev->impulse);
	return false;
}

class CCalcVectorFromNums : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "x_value"))
		{
			m_xValue = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "y_value"))
		{
			m_yValue = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "z_value"))
		{
			m_zValue = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "base_vector"))
		{
			pev->netname = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "vector_type"))
		{
			pev->weapons = atoi(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}

	bool CalcPosition(CBaseEntity *pLocus, Vector *outResult)
	{
		return CalcVector(pLocus, outResult);
	}
	bool CalcVelocity(CBaseEntity *pLocus, Vector *outResult)
	{
		return CalcVector(pLocus, outResult);
	}

	bool CalcVector(CBaseEntity* pLocus, Vector *outResult)
	{
		Vector baseVector = g_vecZero;

		if (!FStringNull(pev->netname))
		{
			if (!TryCalcLocus_Vector(this, pLocus, STRING(pev->netname), baseVector, pev->weapons))
				return false;
		}

		string_t components[3] = {m_xValue, m_yValue, m_zValue};
		for (int i=0; i<3; ++i)
		{
			if (!FStringNull(components[i]))
			{
				float value;
				if (!TryCalcLocus_Ratio(pLocus, STRING(components[i]), value))
					return false;
				baseVector[i] = value;
			}
		}
		*outResult = baseVector;
		return true;
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_xValue;
	string_t m_yValue;
	string_t m_zValue;
};

TYPEDESCRIPTION CCalcVectorFromNums::m_SaveData[] =
{
	DEFINE_FIELD(CCalcVectorFromNums, m_xValue, FIELD_STRING),
	DEFINE_FIELD(CCalcVectorFromNums, m_yValue, FIELD_STRING),
	DEFINE_FIELD(CCalcVectorFromNums, m_zValue, FIELD_STRING),
};

LINK_ENTITY_TO_CLASS( calc_vecfromnums, CCalcVectorFromNums )
IMPLEMENT_SAVERESTORE( CCalcVectorFromNums, CPointEntity )

//=======================================================
#define SF_CALCVELOCITY_NORMALIZE 1
#define SF_CALCVELOCITY_SWAPZ 2
#define SF_CALCVELOCITY_DISCARDX 4
#define SF_CALCVELOCITY_DISCARDY 8
#define SF_CALCVELOCITY_DISCARDZ 16
class CCalcSubVelocity : public CPointEntity
{
	bool Convert( CBaseEntity *pLocus, Vector vecVel, Vector* outVector );
	bool ConvertAngles( CBaseEntity *pLocus, Vector vecAngles, Vector* outVector );
public:
	bool CalcVelocity( CBaseEntity *pLocus, Vector* outResult );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszFactor;
};

LINK_ENTITY_TO_CLASS( calc_subvelocity, CCalcSubVelocity )

TYPEDESCRIPTION	CCalcSubVelocity::m_SaveData[] =
{
	DEFINE_FIELD( CCalcSubVelocity, m_iszFactor, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcSubVelocity, CPointEntity )

void CCalcSubVelocity::Spawn()
{
	m_iszFactor = pev->noise;
	pev->noise = iStringNull;
}

bool CCalcSubVelocity::CalcVelocity( CBaseEntity *pLocus, Vector* outResult )
{
	// Note: this loses the original pLocus. This is how it's coded in all versions of SoHL
	pLocus = UTIL_FindEntityByTargetname( NULL, STRING(pev->netname), pLocus );
	if ( !pLocus )
	{
		ALERT(at_console, "%s \"%s\" failed to find target entity \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->netname) );
		return false;
	}

	CBaseAnimating* pAnimatingSubject = nullptr;
	if (pev->impulse >= 5 && pev->impulse <= 8)
	{
		pAnimatingSubject = pLocus->MyAnimatingPointer();
		if (!pAnimatingSubject)
		{
			ALERT(at_console, "%s \"%s\" can't get attachment points from non-animating entity\n");
			return false;
		}
	}

	Vector vecAngles;
	Vector vecJunk;

	switch (pev->impulse)
	{
	case 1: //angles
		return ConvertAngles( pLocus, pLocus->pev->angles, outResult );
	case 2: //v_angle
		return ConvertAngles( pLocus, pLocus->pev->v_angle, outResult );
	case 5:
		pAnimatingSubject->GetAttachment( 0, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 6:
		pAnimatingSubject->GetAttachment( 1, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 7:
		pAnimatingSubject->GetAttachment( 2, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 8:
		pAnimatingSubject->GetAttachment( 3, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 10:
	{
		if (pLocus == this)
		{
			ALERT(at_warning, "Recursion: %s refers to its own CalcVelocity\n", STRING(pev->targetname));
			return false;
		}
		Vector calcedVelocity;
		if (!pLocus->CalcVelocity(pLocus, &calcedVelocity))
			return false;
		return Convert( pLocus, calcedVelocity, outResult );
	}
	default:
		return Convert( pLocus, pLocus->pev->velocity, outResult );
	}
}

bool CCalcSubVelocity::Convert( CBaseEntity *pLocus, Vector vecDir, Vector* outVector )
{
	if (pev->spawnflags & SF_CALCVELOCITY_NORMALIZE)
		vecDir = vecDir.Normalize();
	
	float fRatio = 1;
	if (m_iszFactor) {
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszFactor), fRatio ))
			return false;
	}
	Vector vecOffset = g_vecZero;

	if (pev->message) {
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->message), vecOffset))
			return false;
	}

	Vector vecResult = vecOffset + (vecDir*fRatio);

	if (pev->spawnflags & SF_CALCVELOCITY_DISCARDX)
		vecResult.x = 0;
	if (pev->spawnflags & SF_CALCVELOCITY_DISCARDY)
		vecResult.y = 0;
	if (pev->spawnflags & SF_CALCVELOCITY_DISCARDZ)
		vecResult.z = 0;
	if (pev->spawnflags & SF_CALCVELOCITY_SWAPZ)
		vecResult.z = -vecResult.z;
//	ALERT(at_console, "calc_subvel returns (%f %f %f) = (%f %f %f) + ((%f %f %f) * %f)\n", vecResult.x, vecResult.y, vecResult.z, vecOffset.x, vecOffset.y, vecOffset.z, vecDir.x, vecDir.y, vecDir.z, fRatio);
	*outVector = vecResult;
	return true;
}

bool CCalcSubVelocity::ConvertAngles( CBaseEntity *pLocus, Vector vecAngles, Vector* outVector )
{
	UTIL_MakeVectors( vecAngles );
	return Convert( pLocus, gpGlobals->v_forward, outVector );
}



//=======================================================
class CCalcVelocityPath : public CPointEntity
{
public:
	bool CalcVelocity( CBaseEntity *pLocus, Vector* outVector );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszFactor;
};

LINK_ENTITY_TO_CLASS( calc_velocity_path, CCalcVelocityPath )

TYPEDESCRIPTION	CCalcVelocityPath::m_SaveData[] =
{
	DEFINE_FIELD( CCalcVelocityPath, m_iszFactor, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcVelocityPath, CPointEntity )

void CCalcVelocityPath::Spawn()
{
	m_iszFactor = pev->noise;
	pev->noise = iStringNull;
}

bool CCalcVelocityPath::CalcVelocity( CBaseEntity *pLocus, Vector* outVector )
{
	Vector vecStart = pev->origin;
	if ( !FStringNull(pev->target) )
	{
		if ( !TryCalcLocus_Position( this, pLocus, STRING(pev->target), vecStart ) )
		{
			//ALERT(at_console, "%s \"%s\" failed: bad LP \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->target));
			return false;
		}
	}

//	ALERT(at_console, "vecStart %f %f %f\n", vecStart.x, vecStart.y, vecStart.z);
	Vector vecOffs = g_vecZero;
	float fFactor = 1;
	if (m_iszFactor) {
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszFactor), fFactor ))
			return false;
	}

	switch ((int)pev->armorvalue)
	{
	case 0:
		if (!TryCalcLocus_Position( this, pLocus, STRING(pev->netname), vecOffs ))
			return false;
		vecOffs = vecOffs - vecStart;
		break;
	case 1:
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->netname), vecOffs ))
			return false;
		break;
	}
//	ALERT(at_console, "vecOffs %f %f %f\n", vecOffs.x, vecOffs.y, vecOffs.z);

	if (pev->health)
	{
		float len = vecOffs.Length();
		switch ((int)pev->health)
		{
		case 1:
			vecOffs = vecOffs/len;
			break;
		case 2:
			vecOffs = vecOffs/(len*len);
			break;
		case 3:
			vecOffs = vecOffs/(len*len*len);
			break;
		case 4:
			vecOffs = vecOffs*len;
			break;
		}
	}

	vecOffs = vecOffs * fFactor;

	if (pev->frags)
	{
		TraceResult tr;
		IGNORE_GLASS iIgnoreGlass = ignore_glass;
		IGNORE_MONSTERS iIgnoreMonsters = ignore_monsters;

		switch ((int)pev->frags)
		{
		case 2:
			iIgnoreGlass = dont_ignore_glass;
			break;
		case 4:
			iIgnoreGlass = dont_ignore_glass;
			// fall through
		case 3:
			iIgnoreMonsters = dont_ignore_monsters;
			break;
		}

		UTIL_TraceLine( vecStart, vecStart+vecOffs, iIgnoreMonsters, iIgnoreGlass, NULL, &tr );
		vecOffs = tr.vecEndPos - vecStart;
	}

//	ALERT(at_console, "path: %f %f %f\n", vecOffs.x, vecOffs.y, vecOffs.z);
	*outVector = vecOffs;
	return true;
}


//=======================================================
class CCalcVelocityPolar : public CPointEntity
{
public:
	bool CalcVelocity( CBaseEntity *pLocus, Vector* outResult );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszFactor;
};

LINK_ENTITY_TO_CLASS( calc_velocity_polar, CCalcVelocityPolar )

TYPEDESCRIPTION	CCalcVelocityPolar::m_SaveData[] =
{
	DEFINE_FIELD( CCalcVelocityPolar, m_iszFactor, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcVelocityPolar, CPointEntity )

void CCalcVelocityPolar::Spawn()
{
	m_iszFactor = pev->noise;
	pev->noise = iStringNull;
}

bool CCalcVelocityPolar::CalcVelocity( CBaseEntity *pLocus, Vector* outResult )
{
	Vector vecBasis = g_vecZero;
	TryCalcLocus_Velocity( this, pLocus, STRING(pev->netname), vecBasis );
	Vector vecAngles = UTIL_VecToAngles( vecBasis ) + pev->angles;
	Vector vecOffset = g_vecZero;
	if (pev->message) {
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->message), vecOffset ))
			return false;
	}

	float fFactor = 1;
	if (m_iszFactor) {
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszFactor), fFactor ))
			return false;
	}

	if (!(pev->spawnflags & SF_CALCVELOCITY_NORMALIZE))
		fFactor = fFactor * vecBasis.Length();

	UTIL_MakeVectors( vecAngles );
	*outResult = (gpGlobals->v_forward * fFactor) + vecOffset;
	return true;
}

//=======================================================

// Position marker
class CMark : public CPointEntity
{
public:
	bool	CalcVelocity(CBaseEntity *pLocus, Vector* outVector) { *outVector = pev->movedir; return true; }
	bool	CalcRatio(CBaseEntity *pLocus, float* outResult) { *outResult = pev->frags; return true; }
	void	Think( void ) { SUB_Remove(); }
};

class CLocusVariable : public CPointEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool	CalcVelocity(CBaseEntity *pLocus, Vector* outVector) { *outVector = pev->movedir; return true; }
	bool	CalcRatio(CBaseEntity *pLocus, float* outResult) { *outResult = pev->frags; return true; }

	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszPosition;
	string_t m_iszVelocity;
	string_t m_iszRatio;
	string_t m_iszTargetName;
	string_t m_iszFireOnSpawn;
	float m_fDuration;
};

TYPEDESCRIPTION	CLocusVariable::m_SaveData[] = 
{
	DEFINE_FIELD( CLocusVariable, m_iszPosition, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszVelocity, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszRatio, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszTargetName, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszFireOnSpawn, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_fDuration, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE( CLocusVariable, CPointEntity )
LINK_ENTITY_TO_CLASS( locus_variable, CLocusVariable )

void CLocusVariable :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelocity"))
	{
		m_iszVelocity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszRatio"))
	{
		m_iszRatio = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTargetName"))
	{
		m_iszTargetName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszFireOnSpawn"))
	{
		m_iszFireOnSpawn = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fDuration"))
	{
		m_fDuration = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CLocusVariable::Spawn( void )
{
	SetMovedir(pev);
}

void CLocusVariable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecPos = g_vecZero;
	Vector vecDir = g_vecZero;
	float fRatio = 0;
	if (m_iszPosition)
		TryCalcLocus_Position(this, pActivator, STRING(m_iszPosition), vecPos);
	if (m_iszVelocity)
		TryCalcLocus_Velocity(this, pActivator, STRING(m_iszVelocity), vecDir);
	if (m_iszRatio)
		TryCalcLocus_Ratio(pActivator, STRING(m_iszRatio), fRatio);

	if (m_iszTargetName)
	{
		CMark *pMark = GetClassPtr( (CMark*)NULL );
		pMark->pev->classname = MAKE_STRING("mark");
		pMark->pev->origin = vecPos;
		pMark->pev->movedir = vecDir;
		pMark->pev->frags = fRatio;
		pMark->pev->targetname = m_iszTargetName;
		pMark->pev->nextthink = gpGlobals->time + m_fDuration;

		FireTargets(STRING(m_iszFireOnSpawn), pMark, this);
	}
	else
	{
		pev->origin = vecPos;
		pev->movedir = vecDir;
		pev->frags = fRatio;

		FireTargets(STRING(m_iszFireOnSpawn), this, this);
	}
}

class CCalcEvalNumber : public CPointEntity
{
public:
	enum {
		EVAL_ADD = 0,
		EVAL_SUBSTRUCT = 1,
		EVAL_MULTIPLY = 2,
		EVAL_DIVIDE = 3,
		EVAL_MOD = 4,
		EVAL_MIN = 5,
		EVAL_MAX = 6
	};

	enum {
		REPORTED_VECTOR_NONE = 0,
		REPORTED_VECTOR_X00 = 1,
		REPORTED_VECTOR_0Y0 = 2,
		REPORTED_VECTOR_00Z = 3,
	};

	void KeyValue( KeyValueData *pkvd );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	bool CalcRatio(CBaseEntity *pLocus, float *outResult)
	{
		return CalcEvalNumber(pLocus, *outResult);
	}
	bool CalcVelocity(CBaseEntity *pLocus, Vector *outResult)
	{
		return ReportVector(pLocus, *outResult);
	}
	bool CalcPosition(CBaseEntity *pLocus, Vector *outResult)
	{
		return ReportVector(pLocus, *outResult);
	}

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

protected:
	bool CalcEvalNumber(CBaseEntity* pActivator, float& result);
	bool DoOperation(float& result, float* operands, int operandCount, int operationId);
	bool ReportVector(CBaseEntity* pLocus, Vector& result);

	string_t m_left;
	string_t m_right;
	string_t m_third;
	int m_operation;
	string_t m_storeIn;
	string_t m_triggerOnFail;
	string_t m_iszMin;
	string_t m_iszMax;
	int m_clampPolicy;
	int m_vectorMode;
	byte m_firstCastMode;
	byte m_secondCastMode;
	byte m_thirdCastMode;
	byte m_resultCastMode;
};

TYPEDESCRIPTION CCalcEvalNumber::m_SaveData[] =
{
	DEFINE_FIELD( CCalcEvalNumber, m_left, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_right, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_third, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_operation, FIELD_INTEGER ),
	DEFINE_FIELD( CCalcEvalNumber, m_storeIn, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_triggerOnFail, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_iszMin, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_iszMax, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_clampPolicy, FIELD_INTEGER ),
	DEFINE_FIELD( CCalcEvalNumber, m_vectorMode, FIELD_INTEGER ),
	DEFINE_FIELD( CCalcEvalNumber, m_firstCastMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CCalcEvalNumber, m_secondCastMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CCalcEvalNumber, m_thirdCastMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CCalcEvalNumber, m_resultCastMode, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CCalcEvalNumber, CPointEntity )

LINK_ENTITY_TO_CLASS( calc_eval_number, CCalcEvalNumber )

void CCalcEvalNumber::KeyValue(KeyValueData *pkvd)
{
	if(FStrEq(pkvd->szKeyName, "operation"))
	{
		m_operation = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "left_operand"))
	{
		m_left = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "right_operand"))
	{
		m_right = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "third_operand"))
	{
		m_third = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "store_result"))
	{
		m_storeIn = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "min_value"))
	{
		m_iszMin = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "max_value"))
	{
		m_iszMax = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "clamp_policy"))
	{
		m_clampPolicy = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "trigger_on_fail"))
	{
		m_triggerOnFail = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "vector_mode"))
	{
		m_vectorMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "first_cast_mode"))
	{
		m_firstCastMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "second_cast_mode"))
	{
		m_secondCastMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "third_cast_mode"))
	{
		m_thirdCastMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(FStrEq(pkvd->szKeyName, "result_cast_mode"))
	{
		m_resultCastMode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CCalcEvalNumber::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	float result;
	if (CalcEvalNumber(pActivator, result))
	{
		if (m_storeIn)
			FireTargets(STRING(m_storeIn), pActivator, this, USE_SET, result);
		if (pev->target)
			FireTargets(STRING(pev->target), pActivator, this);
	}
	else
	{
		if (m_triggerOnFail)
			FireTargets(STRING(m_triggerOnFail), pActivator, this);
	}
}

bool CCalcEvalNumber::CalcEvalNumber(CBaseEntity* pActivator, float& result)
{
	if (FStringNull(m_left) || FStringNull(m_right))
	{
		ALERT(at_error, "%s needs at least left and right operands defined\n", STRING(pev->classname));
		return false;
	}

	float leftValue;
	float rightValue;

	if (!TryCalcLocus_Ratio(pActivator, STRING(m_left), leftValue))
		return false;
	if (!TryCalcLocus_Ratio(pActivator, STRING(m_right), rightValue))
		return false;

	leftValue = ApplyCastMode(leftValue, m_firstCastMode);
	rightValue = ApplyCastMode(rightValue, m_secondCastMode);

	int operandCount = 2;

	float thirdValue;
	if (!FStringNull(m_third))
	{
		if (!TryCalcLocus_Ratio(pActivator, STRING(m_third), thirdValue))
			return false;
		operandCount++;
		thirdValue = ApplyCastMode(thirdValue, m_thirdCastMode);
	}

	float operands[3] = {leftValue, rightValue, thirdValue};

	if (!DoOperation(result, operands, operandCount, m_operation))
		return false;

	if (!ClampToMinMax(pActivator, result, m_iszMin, m_iszMax, m_clampPolicy))
		return false;

	result = ApplyCastMode(result, m_resultCastMode);
	return true;
}

bool CCalcEvalNumber::DoOperation(float& result, float* operands, int operandCount, int operationId)
{
	if (operandCount <= 0)
		return false;
	float value = operands[0];
	for (int i=1; i<operandCount; ++i)
	{
		float operand = operands[i];
		switch (operationId) {
		case EVAL_ADD:
			value += operand;
			break;
		case EVAL_SUBSTRUCT:
			value -= operand;
			break;
		case EVAL_MULTIPLY:
			value *= operand;
			break;
		case EVAL_DIVIDE:
			if (operand == 0.0f)
				return false;
			value /= operand;
			break;
		case EVAL_MOD:
			if (operand == 0.0f)
				return false;
			value = fmod(value, operand);
			break;
		case EVAL_MIN:
			value = Q_min(value, operand);
			break;
		case EVAL_MAX:
			value = Q_max(value, operand);
			break;
		default:
			ALERT(at_error, "%s: unknown operation id %d\n", STRING(pev->classname), operationId);
			return false;
		}
	}
	result = value;
	return true;
}

bool CCalcEvalNumber::ReportVector(CBaseEntity *pLocus, Vector &result)
{
	float f;
	if (!CalcEvalNumber(pLocus, f))
		return false;

	switch (m_vectorMode) {
	case REPORTED_VECTOR_X00:
		result = Vector(f, 0.0f, 0.0f);
		return true;
	case REPORTED_VECTOR_0Y0:
		result =  Vector(0.0f, f, 0.0f);
		return true;
	case REPORTED_VECTOR_00Z:
		result = Vector(0.0f, 0.0f, f);
		return true;
	case REPORTED_VECTOR_NONE:
	default:
		return false;
	}
}
