#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#if !CLIENT_DLL
#include "gamerules.h"
#endif

WeaponInfo& AccessWeaponInfo(int id)
{
	static WeaponInfo arr[MAX_WEAPONS] = {};
	return arr[id];
}

WeaponRegistrator::WeaponRegistrator(const char* classname, CBasePlayerWeapon* pWeapon)
{
	const int id = pWeapon->WeaponId();
	WeaponInfo& info = AccessWeaponInfo(id);
	if (info.classname)
	{
		return;
	}
	info.id = id;
	info.classname = classname;
	info.pWeapon = pWeapon;
}

bool CBasePlayerWeapon::CanDeploy( void )
{
	bool bHasAmmo = false;

	if( !UsesAmmo() )
	{
		// this weapon doesn't use ammo, can always deploy.
		return true;
	}

	if( UsesAmmo() )
	{
		bHasAmmo |= ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0 );
	}
	if( UsesSecondaryAmmo() )
	{
		bHasAmmo |= ( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0 );
	}
	if( m_iClip > 0 )
	{
		bHasAmmo |= true;
	}
	if( !bHasAmmo )
	{
		return false;
	}

	return true;
}

bool CBasePlayerWeapon::DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return false;

	int j = Q_min( iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] );

	if( j == 0 )
		return false;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, body );

	m_fInReload = true;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	return true;
}

bool CBasePlayerWeapon::DefaultClipReload(int iAnim, float fDelay, int body)
{
	return DefaultReload(m_iMaxClip, iAnim, fDelay, body);
}

void CBasePlayerWeapon::ResetEmptySound( void )
{
	m_iPlayEmptySound = true;
}

bool CanAttack( float attack_time, float curtime, bool isPredicted )
{
#ifdef CLIENT_DLL
	return attack_time <= 0.0f;
#else
#if CLIENT_WEAPONS
	if( !isPredicted )
#else
	if( 1 )
#endif
	{
		return ( attack_time <= curtime );
	}
	else
	{
		return ( (static_cast<int>(::floor(attack_time * 1000.0f)) * 1000.0f) <= 0.0f);
	}
#endif
}

void CBasePlayerWeapon::ItemPostFrame( void )
{
	if( ( m_fInReload ) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		int maxClip = iMaxClip();
		// complete the reload.
		int j = Q_min( maxClip - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_fInReload = false;
	}

#ifndef CLIENT_DLL
	if( !(m_pPlayer->pev->button & IN_ATTACK ) )
	{
		m_flLastFireTime = 0.0f;
	}
#endif

	if( ( m_pPlayer->pev->button & IN_ATTACK2 ) && CanAttack( m_flNextSecondaryAttack, gpGlobals->time, UseDecrement() ) )
	{
		if( UsesSecondaryAmmo() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = true;
		}

		if (!FBitSet(m_pPlayer->m_suppressedCapabilities, PLAYER_SUPPRESS_ATTACK) && !FBitSet(m_pPlayer->pev->flags, FL_FROZEN))
			SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && CanAttack( m_flNextPrimaryAttack, gpGlobals->time, UseDecrement() ) )
	{
		if( ( m_iClip == 0 && UsesAmmo() ) || ( iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = true;
		}

		if (!FBitSet(m_pPlayer->m_suppressedCapabilities, PLAYER_SUPPRESS_ATTACK) && !FBitSet(m_pPlayer->pev->flags, FL_FROZEN))
			PrimaryAttack();
	}
	else if( m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload  && !FBitSet(m_pPlayer->pev->flags, FL_FROZEN) )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if( !( m_pPlayer->pev->button & ( IN_ATTACK | IN_ATTACK2 ) ) )
	{
		// no fire buttons down
		m_fFireOnEmpty = false;

#ifndef CLIENT_DLL
		if( !IsUseable() && m_flNextPrimaryAttack < ( UseDecrement() ? 0.0f : gpGlobals->time ) )
		{
			// weapon isn't useable, switch.
			if( !( iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY ) && g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
			{
				m_flNextPrimaryAttack = ( UseDecrement() ? 0.0f : gpGlobals->time ) + 0.3f;
				return;
			}
		}
		else
#endif
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if( m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD ) &&
#ifdef CLIENT_DLL
					m_flNextPrimaryAttack <= 0.0f
#else
					m_flNextPrimaryAttack < ( UseDecrement() ? 0.0f : gpGlobals->time )
#endif
					)
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}

	// catch all
	if( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

int CBasePlayerWeapon::iMaxClip()
{
	return m_iMaxClip;
}

bool CBasePlayerWeapon::InZoom()
{
	return m_pPlayer->pev->fov != 0;
}

bool CBasePlayerWeapon::CanReload()
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 && m_iClip < m_iMaxClip;
}
