#include "CameraComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Engine/Node.h"
#include "../Engine/Transform.h"

#include "../Utility/Input.h"
#include "../Utility/Time.h"

CameraComponent::CameraComponent(glm::mat4 projectionMatrix, float fieldOfView, float movementSpeed, float rotationSpeed): Component(ComponentType::Camera),
projectionMatrix(projectionMatrix), fieldOfView(fieldOfView), movementSpeed(movementSpeed), rotationSpeed(rotationSpeed), rotX(glm::mat4(1.0f)), rotY(glm::mat4(1.0f)) {
}

CameraComponent::~CameraComponent() {
}

void CameraComponent::update(std::vector<Node*>& renderables, std::vector<Node*>& lights, std::vector<Node*>& cameras) {
	cameras.push_back(_owner);

	//mouse offset
	glm::vec2 mouseOffset = Input::GetLastMousePos() - Input::GetCurrentMousePos();

	//movement
	glm::vec3 translation = glm::vec3(0.0f);

	if(Input::GetKey(Key::W)) translation.z = -movementSpeed * Time::DeltaTime;
	if(Input::GetKey(Key::S)) translation.z = movementSpeed * Time::DeltaTime;
	if(Input::GetKey(Key::A)) translation.x = -movementSpeed * Time::DeltaTime;
	if(Input::GetKey(Key::D)) translation.x = movementSpeed * Time::DeltaTime;
	if(Input::GetKey(Key::SPACE)) translation.y = movementSpeed * Time::DeltaTime;
	if(Input::GetKey(Key::LSHIFT)) translation.y = -movementSpeed * Time::DeltaTime;

	//rotation
	glm::mat4 yRotation = rotY;
	glm::mat4 xRotation = rotX;

	if(std::abs(mouseOffset.x) >= 1.0f) {
		yRotation = glm::rotate(yRotation, glm::radians(mouseOffset.x * rotationSpeed * Time::DeltaTime), glm::vec3(0.0f, 1.0f, 0.0f));
		rotY = yRotation;
	}

	if(std::abs(mouseOffset.y) >= 1.0f) {
		xRotation = glm::rotate(xRotation, glm::radians(mouseOffset.y * rotationSpeed * Time::DeltaTime), glm::vec3(1.0f, 0.0f, 0.0f));
		rotX = xRotation;
	}

	//reconstruct transform and apply to component
	Transform* transform = _owner->getTransform();
	glm::vec3 localPos = transform->getLocalPosition();

	glm::mat4 newTransform = glm::mat4(1.0f);

	newTransform = glm::translate(newTransform, localPos);
	newTransform = newTransform * yRotation * xRotation;
	newTransform = glm::translate(newTransform, translation);

	transform->localTransform = newTransform;
}
