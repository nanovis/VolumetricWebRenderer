#include "CameraController.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include "Input.h"
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include "Hotkeys.h"

CameraController::CameraController(Camera* camera)
{
	this->camera = camera;
	translateSpeed = 1.0f;
	shiftSpeed = 0.005f;
	rotationSpeed = 0.3f;
	scrollSpeed = 0.2f;

	locCamera = new Camera(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

CameraController::~CameraController()
{

}


void CameraController::update(float timeElapsed)
{
	const float radius = 10.0f;
	float camX = sin(timeElapsed) * radius;
	float camZ = cos(timeElapsed) * radius;
	glm::mat4 view;
	view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	

	if (Input::getKey(GLFW_KEY_W))
	{
		camera->translate(glm::vec3(0.0f, 0.0f, 1.0f) * translateSpeed * timeElapsed, Camera::DontTranslateViewCenter);
	}

	if (Input::getKey(GLFW_KEY_S))
	{
		camera->translate(glm::vec3(0.0f, 0.0f, -1.0f) * translateSpeed * timeElapsed, Camera::DontTranslateViewCenter);
	}
	
	if (Input::getKey(GLFW_KEY_A))
	{
		camera->translate(glm::vec3(-1.0f, 0.0f, 0.0f) * translateSpeed * timeElapsed, Camera::DontTranslateViewCenter);
	}
	
	if (Input::getKey(GLFW_KEY_D))
	{
		camera->translate(glm::vec3(1.0f, 0.0f, 0.0f) * translateSpeed * timeElapsed, Camera::DontTranslateViewCenter);
	}

	auto mousePosTmp = Input::mousePosition();

	double deltaX = mousePos.first - mousePosTmp.first;
	double deltaY = mousePos.second - mousePosTmp.second;
	
	if (Input::getMouseButton(Hotkeys::getHotkey("rotation_mouse")) && !Input::getKey(Hotkeys::getHotkey("annotation_mode")))
	{
		//camera->panAboutViewCenter(deltaX * timeElapsed, panAxis);
		auto tmpViewCenter = camera->getViewCenter();
		//camera->setViewCenter(glm::vec3(0, 0, 0));
		//camera->panAboutViewCenter(deltaX * timeElapsed * rotationSpeed);


		//glm::vec3 up = glm::vec4(0.0, 1.0f, 0.0f, 0.0);
		modelRotationX = deltaX * rotationSpeed;
		modelRotationY = deltaY * rotationSpeed;
		
		//model = glm::rotate(glm::mat4(1.0f), glm::radians(modelRotationX), up);
		//glm::vec3 right = glm::normalize(glm::inverse(model) * glm::vec4(1.0, 0.0f, 0.0f, 0.0));
		//glm::vec3 right = glm::normalize(glm::vec4(1.0, 0.0f, 0.0f, 0.0));
		//model = glm::rotate(model, glm::radians(modelRotationY), right);

		
		locCamera->panAboutViewCenter(modelRotationX);
		locCamera->tiltAboutViewCenter(-modelRotationY);
		model = locCamera->getViewMatrix();
		// remove translation
		model = glm::column(model, 3, glm::vec4(0, 0, 0,1));
	}

	// camera panning
	if (Input::getMouseButton(GLFW_MOUSE_BUTTON_MIDDLE))
	{
		camera->translate(glm::vec3(deltaX * shiftSpeed, -deltaY * shiftSpeed, 0.0f));
	}

	// camera zooming
	float distance = 0.0f;
	if (Hotkeys::getHotkey("zoom_mouse") == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		distance = Input::mouseScrollDelta().second * scrollSpeed;
	}
	else
	{
		if (Input::getMouseButton(Hotkeys::getHotkey("zoom_mouse")))
		{
			distance = deltaY * 0.01f;
		}
	}

	auto viewVector = camera->viewCenter - camera->position;
	if (glm::length(viewVector) > distance) {
		camera->translate(glm::vec3(0.0f, 0.0f, distance), Camera::DontTranslateViewCenter);
	}

	mousePos = Input::mousePosition();
}