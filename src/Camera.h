#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 viewCenter, glm::vec3 upVec);

	virtual ~Camera();

	enum CameraTranslationOption {
		TranslateViewCenter,
		DontTranslateViewCenter
	};

	// Translate relative to camera orientation axes
	void translate(glm::vec3 localVector, Camera::CameraTranslationOption option = TranslateViewCenter);

	// Translate relative to world axes
	//void translateWorld(glm::vec3 &worldVector, CameraTranslationOption option = TranslateViewCenter);

	void tiltAboutViewCenter(float angle);
	
	void panAboutViewCenter(float angle);

	void rotateAboutViewCenter(glm::quat rotation);

	void setViewCenter(glm::vec3 center);

	void setPosition(glm::vec3 pos);

	void setUpVector(glm::vec3 up);

	void setAspectRatio(float ratio);

	float getAspectRatio();

	void setFov(float ratio);

	float getFov();

	void setNear(float near);

	void setFar(float near);

	inline glm::vec3 getViewCenter() { return viewCenter; }

	inline glm::vec3 getPosition() { return position; }

	inline glm::vec3 getUpVec() { return upVector; }

	glm::vec3 getViewVector();

	glm::mat4 getViewMatrix();

	glm::mat4 getProjectionMatrix();

public:
	void updateViewMatrix();
	void updateProjectionMatrix();

	bool viewMatrixDirty;

	float nearPlane;
	float farPlane;
	float fieldOfView;
	float aspectRatio;

	glm::vec3 position;
	glm::vec3 upVector;
	glm::vec3 viewCenter;

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

};