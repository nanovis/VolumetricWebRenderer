#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Camera::Camera() //: 
    //Camera(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))
{
    this->position = glm::vec3(0.0f, 0.0f, -1.0f);
    this->viewCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    this->upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    this->nearPlane = 0.01f;
    this->farPlane = 5.0f;
    this->fieldOfView = 45.0f;
    this->aspectRatio = 1.33f;

    viewMatrixDirty = true;

    updateProjectionMatrix();
}

Camera::Camera(glm::vec3 position, glm::vec3 viewCenter, glm::vec3 upVector)
{
    this->position = position;
    this->viewCenter = viewCenter;
    this->upVector = upVector;

    this->nearPlane = 0.01f;
    this->farPlane = 5.0f;
    this->fieldOfView = 45.0f;
    this->aspectRatio = 1.33f;

    viewMatrixDirty = true;

    updateProjectionMatrix();
}

Camera::~Camera()
{

}

void Camera::translate(glm::vec3 localVector, Camera::CameraTranslationOption option)
{
    auto viewVector = viewCenter - position;

    glm::vec3 worldVector(0.0f);
    if (glm::abs(localVector.x) > 0.00001f)
    {
        glm::vec3 right = glm::normalize(glm::cross(viewVector, upVector));
        worldVector += localVector.x * right;
    }
    
    if (glm::abs(localVector.y) > 0.00001f)
    {
        worldVector += localVector.y * upVector;
    }
    if (glm::abs(localVector.z) > 0.00001f) {
        worldVector += localVector.z * glm::normalize(viewVector);
    }
    
    setPosition(position + worldVector);

    if (option == TranslateViewCenter) {
        setViewCenter(viewCenter + worldVector);
    }
       
    viewVector = viewCenter - position;
    auto right = glm::normalize(glm::cross(viewVector, upVector));

    setUpVector(glm::normalize(glm::cross(right, viewVector)));
}

void Camera::panAboutViewCenter(float angle)
{
    auto rotation = glm::angleAxis(glm::radians(angle), upVector);
    rotateAboutViewCenter(rotation);
}

void Camera::tiltAboutViewCenter(float angle)
{
    auto viewVector = viewCenter - position;
    auto xBasis = glm::normalize(glm::cross(upVector, glm::normalize(viewVector)));
    auto rotation = glm::angleAxis(glm::radians(angle), xBasis);
    rotateAboutViewCenter(rotation);
}

void Camera::rotateAboutViewCenter(glm::quat rotation)
{
    setUpVector(rotation * upVector);
    auto viewVector = viewCenter - position;
    auto cameraToCenter = rotation * viewVector;
    setPosition(viewCenter - cameraToCenter);
    setViewCenter(position + cameraToCenter);
}

void Camera::setViewCenter(glm::vec3 center)
{
    viewCenter = center;
    viewMatrixDirty = true;
}

void Camera::setPosition(glm::vec3 pos)
{
    position = pos;
    viewMatrixDirty = true;
}

void Camera::setNear(float near)
{
    nearPlane = near;
    updateProjectionMatrix();
}

void Camera::setFar(float near)
{
    farPlane = near;
    updateProjectionMatrix();
}

void Camera::setUpVector(glm::vec3 up)
{
    upVector = up;
    viewMatrixDirty = true;
}

glm::vec3 Camera::getViewVector()
{
    return glm::normalize(viewCenter - position);
}

void Camera::updateViewMatrix()
{
    viewMatrix = glm::lookAt(position, viewCenter, upVector);
    viewMatrixDirty = false;
}

glm::mat4 Camera::getViewMatrix()
{
    if (viewMatrixDirty) {
        updateViewMatrix();
    }
    return viewMatrix;
}

void Camera::updateProjectionMatrix()
{
    projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

glm::mat4 Camera::getProjectionMatrix()
{
    return projectionMatrix;
}

void Camera::setAspectRatio(float ratio)
{
    aspectRatio = ratio;
    updateProjectionMatrix();
}

float Camera::getAspectRatio()
{
    return aspectRatio;
}

void Camera::setFov(float fov)
{
    this->fieldOfView = fov;
    updateProjectionMatrix();
}

float Camera::getFov()
{
    return fieldOfView;
}