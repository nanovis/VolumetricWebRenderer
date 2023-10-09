#include "Input.h"
#include <iostream>

std::map<int, bool> Input::keys;
std::map<int, bool> Input::mouseButtons;
std::map<int, bool> Input::mouseButtonsDown;
std::map<int, bool> Input::mouseButtonsUp;
std::pair<double, double> Input::mousePos;
std::pair<double, double> Input::mouseScrollDeltaValue;

bool Input::getKey(int key)
{
    return keys[key];
}

std::pair<double, double> Input::mousePosition()
{
    return mousePos;
}

bool Input::getMouseButtonDown(int button)
{
    bool down = mouseButtonsDown[button];

    // Todo: remove workaround. Input class should get an update every frame and then refresh the buttons
    mouseButtonsDown[button] = false;

    return down;
}

bool Input::getMouseButtonUp(int button)
{
    bool up = mouseButtonsUp[button];

    // Todo: remove workaround. Input class should get an update every frame and then refresh the buttons
    mouseButtonsUp[button] = false;

    return up;
}

bool Input::getMouseButton(int button)
{
    return mouseButtons[button];
}

void Input::keyPressEvent(int key)
{
    keys[key] = true;
}

void Input::keyReleaseEvent(int key)
{
    keys[key] = false;
}

void Input::wheelEvent(double xOffset, double yOffset)
{
    mouseScrollDeltaValue = std::make_pair(xOffset, yOffset);
}
void Input::setMousePos(double xPos, double yPos)
{
    mousePos = std::make_pair(xPos, yPos);
}

void Input::mousePressEvent(int button) 
{
    mouseButtons[button] = true;
    mouseButtonsDown[button] = true;
}

void Input::mouseReleaseEvent(int button) 
{
    mouseButtons[button] = false;
    mouseButtonsUp[button] = true;
}

std::pair<double, double> Input::mouseScrollDelta()
{
    // ToDo: find better solution
    auto tmp = mouseScrollDeltaValue;
    mouseScrollDeltaValue = std::make_pair(0,0);
    return tmp;
}
