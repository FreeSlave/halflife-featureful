#pragma once
#if !defined(INPUT_MOUSE_H)
#define INPUT_MOUSE_H
#include "cl_dll.h"
#include "usercmd.h"
#include "in_defs.h"

class AbstractInput
{
public:
	virtual void IN_ClientMoveEvent( float forwardmove, float sidemove ) = 0;
	virtual void IN_ClientLookEvent( float relyaw, float relpitch ) = 0;
	virtual void IN_Move( float frametime, usercmd_t *cmd ) = 0;
	virtual void IN_MouseEvent( int mstate ) = 0;
	virtual void IN_ClearStates() = 0;
	virtual void IN_ActivateMouse() = 0;
	virtual void IN_DeactivateMouse() = 0;
	virtual void IN_Accumulate() = 0;
	virtual void IN_Commands() = 0;
	virtual void IN_Shutdown() = 0;
	virtual void IN_Init() = 0;
	virtual void IN_ResetMouse() = 0;
	virtual void Joy_AdvancedUpdate() = 0;
	virtual void IgnoreNextMouseDelta() = 0;
};

class FWGSInput : public AbstractInput
{
public:
	void IN_ClientMoveEvent( float forwardmove, float sidemove ) override;
	void IN_ClientLookEvent( float relyaw, float relpitch ) override;
	void IN_Move( float frametime, usercmd_t *cmd ) override;
	void IN_MouseEvent( int mstate ) override;
	void IN_ClearStates() override;
	void IN_ActivateMouse() override;
	void IN_DeactivateMouse() override;
	void IN_Accumulate() override;
	void IN_Commands() override;
	void IN_Shutdown() override;
	void IN_Init() override;
	void IN_ResetMouse() override {}
	void Joy_AdvancedUpdate() override {}
	void IgnoreNextMouseDelta() override {}

protected:
	float ac_forwardmove;
	float ac_sidemove;
	int ac_movecount;
	float rel_yaw;
	float rel_pitch;
};

// No need for goldsource input support on the platforms that are not supported by GoldSource.
#if GOLDSOURCE_SUPPORT && (_WIN32 || (__linux__ && !__ANDROID__) || __APPLE__) && (__i386 || _M_IX86)
#define SUPPORT_GOLDSOURCE_INPUT	1

#if XASH_WIN32
#define HSPRITE WINDOWS_HSPRITE
#define NOMINMAX
#include <windows.h>
#undef HSPRITE
#else
typedef struct point_s
{
	int x;
	int y;
} POINT;
#define GetCursorPos(x)
#define SetCursorPos(x,y)
#endif

class GoldSourceInput : public AbstractInput
{
public:
	void IN_ClientMoveEvent( float forwardmove, float sidemove ) override {}
	void IN_ClientLookEvent( float relyaw, float relpitch ) override {}
	void IN_Move( float frametime, usercmd_t *cmd ) override;
	void IN_MouseEvent( int mstate ) override;
	void IN_ClearStates() override;
	void IN_ActivateMouse() override;
	void IN_DeactivateMouse() override;
	void IN_Accumulate() override;
	void IN_Commands() override;
	void IN_Shutdown() override;
	void IN_Init() override;
	void IN_ResetMouse() override;
	void Joy_AdvancedUpdate() override;
	void IgnoreNextMouseDelta() override;

protected:
	void IN_GetMouseDelta( int *pOutX, int *pOutY);
	void IN_MouseMove ( float frametime, usercmd_t *cmd);
	void IN_StartupMouse ();
	void IN_StartupJoystick ();
	int IN_ReadJoystick ();
	void IN_JoyMove ( float frametime, usercmd_t *cmd );
	bool UseSDL2Joystick();

	int         mouse_buttons;
	int         mouse_oldbuttonstate;
	POINT       current_pos;
	int         old_mouse_x, old_mouse_y, mx_accum, my_accum;
	int         mouseinitialized;
	void* sdl2Lib;
	bool ignoreNextDelta;
};
#endif

AbstractInput* CurrentMouseInput();

#endif
