#pragma once

#include <map>
#include <utility>
#include "GLFW/glfw3.h"

class Input
{
public:
	static bool getKey(int button);
	static bool getMouseButton(int button);
	static std::pair<double, double> mousePosition();

	static void keyPressEvent(int key);
	static void keyReleaseEvent(int key);
	
	static void setMousePos(double xPos, double yPos);
	static void wheelEvent(double xOffset, double yOffset);
	static void mousePressEvent(int button);
	static void mouseReleaseEvent(int button);

	static bool getMouseButtonDown(int button);
	static bool getMouseButtonUp(int button);
	static std::pair<double, double> mouseScrollDelta();


private:

	static std::map<int, bool> keys;
	static std::map<int, bool> mouseButtons;
	static std::map<int, bool> mouseButtonsDown;
	static std::map<int, bool> mouseButtonsUp;
	static std::pair<double, double> mousePos;
	static std::pair<double, double> mouseScrollDeltaValue;
};

