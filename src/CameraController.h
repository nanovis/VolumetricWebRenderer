#pragma once

#include <glm/glm.hpp>
#include "Camera.h"
#include <utility> 

class CameraController
{
public:

	CameraController(Camera *camera);
	virtual ~CameraController();

	void update(float timeElapsed);

	float shiftSpeed;
	float translateSpeed;
	float rotationSpeed;
	float scrollSpeed;
	
	Camera *camera;
	Camera *locCamera;
	
	glm::mat4 model = glm::mat3(1.0);
	float modelRotationX = 0.0f;
	float modelRotationY = 0.0f;
	
private:
	std::pair<double, double> mousePos;
};

