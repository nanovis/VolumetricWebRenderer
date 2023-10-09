#pragma once

#include <map>
#include <string>

class Hotkeys
{
public:
	static void setHotkey(std::string name, int key);

	static int getHotkey(std::string name);

	static void loadConfig(std::string configFile);

private:

	static std::map<std::string, int> hotkeys;

};

