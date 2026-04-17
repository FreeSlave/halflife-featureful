#include "cmdkeys.h"

#include <cstring>

#include "arraysize.h"
#include "keydefs.h"
#include "parsetext.h"

struct KeyNameAndKeyNum
{
	const char* name;
	int keynum;
};

static const KeyNameAndKeyNum keynames[] =
{
	{"TAB",		K_TAB},
	{"ENTER",	K_ENTER},
	{"ESCAPE",	K_ESCAPE},
	{"SPACE",		K_SPACE},
	{"BACKSPACE",	K_BACKSPACE},
	{"UPARROW",	K_UPARROW},
	{"DOWNARROW",	K_DOWNARROW},
	{"LEFTARROW",	K_LEFTARROW},
	{"RIGHTARROW",	K_RIGHTARROW},
	{"ALT",		K_ALT},
	{"CTRL",		K_CTRL},
	{"SHIFT",		K_SHIFT},
	{"CAPSLOCK",	K_CAPSLOCK},
	{"SCROLLOCK",	K_SCROLLLOCK},
	{"F1",		K_F1},
	{"F2",		K_F2},
	{"F3",		K_F3},
	{"F4",		K_F4},
	{"F5",		K_F5},
	{"F6",		K_F6},
	{"F7",		K_F7},
	{"F8",		K_F8},
	{"F9",		K_F9},
	{"F10",		K_F10},
	{"F11",		K_F11},
	{"F12",		K_F12},
	{"INS",		K_INS},
	{"DEL",		K_DEL},
	{"PGDN",	K_PGDN},
	{"PGUP",	K_PGUP},
	{"HOME",	K_HOME},
	{"END",		K_END},

	// mouse buttouns
	{"MOUSE1",	K_MOUSE1},
	{"MOUSE2",	K_MOUSE2},
	{"MOUSE3",	K_MOUSE3},
	{"MOUSE4",	K_MOUSE4},
	{"MOUSE5",	K_MOUSE5},
	{"MWHEELUP",	K_MWHEELUP},
	{"MWHEELDOWN",	K_MWHEELDOWN},

	// digital keyboard
	{"KP_HOME",	K_KP_HOME},
	{"KP_UPARROW",	K_KP_UPARROW},
	{"KP_PGUP",	K_KP_PGUP},
	{"KP_LEFTARROW",	K_KP_LEFTARROW},
	{"KP_5",		K_KP_5},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW},
	{"KP_END",	K_KP_END},
	{"KP_DOWNARROW",	K_KP_DOWNARROW},
	{"KP_PGDN",	K_KP_PGDN},
	{"KP_ENTER",	K_KP_ENTER},
	{"KP_INS",	K_KP_INS},
	{"KP_DEL",	K_KP_DEL},
	{"KP_SLASH",	K_KP_SLASH},
	{"KP_MINUS",	K_KP_MINUS},
	{"KP_PLUS",	K_KP_PLUS},
	{"PAUSE",	K_PAUSE},

	// Gamepad
	// A/B X/Y names match the Xbox controller layout
	{"A_BUTTON", K_A_BUTTON},
	{"B_BUTTON", K_B_BUTTON},
	{"X_BUTTON", K_X_BUTTON},
	{"Y_BUTTON", K_Y_BUTTON}, // Flashlight
	{"BACK",   K_BACK_BUTTON}, // Menu
	{"MODE",   K_MODE_BUTTON},
	{"START",  K_START_BUTTON},
	{"STICK1", K_LSTICK},
	{"STICK2", K_RSTICK},
	{"L1_BUTTON",  K_L1_BUTTON},
	{"R1_BUTTON",  K_R1_BUTTON},
	{"DPAD_UP",	K_DPAD_UP}, // Spray
	{"DPAD_DOWN",	K_DPAD_DOWN},
	{"DPAD_LEFT",	K_DPAD_LEFT},
	{"DPAD_RIGHT",	K_DPAD_RIGHT},
	{"L2_BUTTON", K_L2_BUTTON},
	{"R2_BUTTON", K_R2_BUTTON},
	{"LTRIGGER" , K_JOY1}, // L2 in SDL2
	{"RTRIGGER" , K_JOY2}, // R2 in SDL2
	{"JOY3" , K_JOY3},
	{"JOY4" , K_JOY4},
	{"C_BUTTON", K_C_BUTTON},
	{"Z_BUTTON", K_Z_BUTTON},
	{"MISC_BUTTON", K_MISC_BUTTON},
	{"PADDLE1", K_PADDLE1_BUTTON},
	{"PADDLE2", K_PADDLE2_BUTTON},
	{"PADDLE3", K_PADDLE3_BUTTON},
	{"PADDLE4", K_PADDLE4_BUTTON},
	{"TOUCHPAD", K_TOUCHPAD},
	{"AUX26", K_AUX26}, // generic
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	// raw semicolon seperates commands
	{"SEMICOLON",	';'},
};

static unsigned char NibbleHex( char c )
{
	if ((c >= '0') && (c <= '9'))
	{
		return (unsigned char)(c - '0');
	}
	if ((c >= 'A') && (c <= 'F'))
	{
		return (unsigned char)(c - 'A' + 0x0A);
	}
	if ((c >= 'a') && (c <= 'f'))
	{
		return (unsigned char)(c - 'a' + 0x0A);
	}
	return '0';
}

static int Key_StringToKeynum(const char *str)
{
	if (!str || !str[0])
		return -1;

	if (!str[1])
		return str[0];

	if (str[0] == '0' && str[1] == 'x' && strlen(str) == 4)
		return NibbleHex(str[2]) << 4 | NibbleHex(str[3]);

	for (const auto& key : keynames)
	{
		if (stricmp(str, key.name) == 0)
			return key.keynum;
	}

	return -1;
}

const char* SpecialKeynumToString(int keynum)
{
	for (const auto& key : keynames)
	{
		if (keynum == key.keynum)
			return key.name;
	}
	return nullptr;
}

int CmdKeys::AddDefaultKeyNumForCommand(const std::string &command, int keynum)
{
	if (keynum < 0)
		return COMMANDKEY_OUTOFBOUNDS;
	if (keynum > 255)
		return COMMANDKEY_OUTOFBOUNDS;

	auto it = _cmdsToKeySets.find(command);
	if (it != _cmdsToKeySets.end())
	{
		for (size_t i=1; i<ARRAYSIZE(it->second.keynums); ++i)
		{
			if (it->second.keynums[i] == 0)
			{
				it->second.keynums[i] = keynum;
				return COMMANDKEY_SUCCESS;
			}
		}
		return COMMANDKEY_TOOMANYKEYS;
	}
	else
	{
		KeySet keySet;
		keySet.keynums[0] = keynum;
		_cmdsToKeySets[command] = keySet;
		return COMMANDKEY_SUCCESS;
	}
}

int CmdKeys::AddDefaultKeyNumForCommand(const std::string &command, const char *keyname)
{
	int keynum = Key_StringToKeynum(keyname);
	if (keynum == -1)
		return COMMANDKEY_UNKNOWNKEY;

	return AddDefaultKeyNumForCommand(command, keynum);
}
