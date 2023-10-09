#include "Hotkeys.h"

#include <iostream>
#include <fstream>

// define is used so that json throws exceptions
#define __EXCEPTIONS
#include <json/json.hpp>
using json = nlohmann::json;

std::map<std::string, int> Hotkeys::hotkeys;

void Hotkeys::setHotkey(std::string name, int key)
{
    hotkeys[name] = key;
}

int Hotkeys::getHotkey(std::string name)
{
    return hotkeys[name];
}

void Hotkeys::loadConfig(std::string configFile)
{
	std::ifstream file(configFile);
	if (file.is_open())
	{
		try
		{
			json config;
			file >> config;
			for (json::iterator it = config.begin(); it != config.end(); ++it)
			{
				const auto name = it.key();
				if (!name.rfind("_", 0) == 0)
				{
					int hotkey = it.value();
					setHotkey(name, hotkey);
				}
			}
		}
		catch (json::exception& e)
		{
			std::cout << e.what() << '\n';
		}
	}
	else
	{
		std::cout << "Unable to open file: " << configFile << std::endl;
	}
}