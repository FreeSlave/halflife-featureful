#pragma once
#ifndef CMDKEYS_H
#define CMDKEYS_H

#include <string>
#include <map>

enum
{
	COMMANDKEY_SUCCESS,
	COMMANDKEY_UNKNOWNKEY,
	COMMANDKEY_OUTOFBOUNDS,
	COMMANDKEY_TOOMANYKEYS
};

const char* SpecialKeynumToString(int keynum);

class CmdKeys
{
public:
	struct KeySet
	{
		int keynums[3] = {0,0,0};
	};

	typedef std::map<std::string, KeySet> CommandToKeysMap;

	CommandToKeysMap::const_iterator begin() const { return _cmdsToKeySets.begin(); }
	CommandToKeysMap::const_iterator end() const { return _cmdsToKeySets.end(); }
	int AddDefaultKeyNumForCommand(const std::string& command, int keynum);
	int AddDefaultKeyNumForCommand(const std::string& command, const char* keyname);
	void Clear() { _cmdsToKeySets.clear(); }
private:
	CommandToKeysMap _cmdsToKeySets;
};

#endif
