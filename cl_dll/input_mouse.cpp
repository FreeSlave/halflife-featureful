#include "input_mouse.h"
#include "exportdef.h"
#include "hud.h"
#include "cl_util.h"

// shared between backends
Vector dead_viewangles(0, 0, 0);
cvar_t      *sensitivity;
cvar_t  *in_joystick;

FWGSInput fwgsInput;

#if SUPPORT_GOLDSOURCE_INPUT
GoldSourceInput goldSourceInput;
AbstractInput* currentInput = &goldSourceInput;
#else
AbstractInput* currentInput = &fwgsInput;
#endif
extern "C"  void DLLEXPORT IN_ClientMoveEvent( float forwardmove, float sidemove )
{
	currentInput->IN_ClientMoveEvent(forwardmove, sidemove);
}

extern "C" void DLLEXPORT IN_ClientLookEvent( float relyaw, float relpitch )
{
	currentInput->IN_ClientLookEvent(relyaw, relpitch);
}

void IN_Move( float frametime, usercmd_t *cmd )
{
	currentInput->IN_Move(frametime, cmd);
}

extern "C" void DLLEXPORT IN_MouseEvent( int mstate )
{
	currentInput->IN_MouseEvent(mstate);
}

extern "C" void DLLEXPORT IN_ClearStates()
{
	currentInput->IN_ClearStates();
}

extern "C" void DLLEXPORT IN_ActivateMouse()
{
	currentInput->IN_ActivateMouse();
}

extern "C" void DLLEXPORT IN_DeactivateMouse()
{
	currentInput->IN_DeactivateMouse();
}

extern "C" void DLLEXPORT IN_Accumulate()
{
	currentInput->IN_Accumulate();
}

void IN_Commands()
{
	currentInput->IN_Commands();
}

void IN_Shutdown()
{
	currentInput->IN_Shutdown();
}

void IN_Init()
{
#if SUPPORT_GOLDSOURCE_INPUT
	if (IsXashFWGS()) {
		gEngfuncs.Con_Printf( "FWGS Xash3D input is in use\n" );
		currentInput = &fwgsInput;
	} else {
		gEngfuncs.Con_Printf( "GoldSource input is in use\n" );
		currentInput = &goldSourceInput;
	}
#else
	currentInput = &fwgsInput;
#endif
	currentInput->IN_Init();
}

void IN_ResetMouse()
{
	currentInput->IN_ResetMouse();
}

void IgnoreNextMouseDelta()
{
	currentInput->IgnoreNextMouseDelta();
}

AbstractInput* CurrentMouseInput()
{
	return currentInput;
}
