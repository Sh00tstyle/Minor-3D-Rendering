#ifndef LIGHTCOMPONENT_H
#define LIGHTCOMPONENT_H

#include <glm/glm.hpp>

#include "../Engine/Component.h"
#include "../Engine/GLLight.h"

#include "../Utility/LightType.h"

class Node;

class LightComponent : public Component {
	public:
		LightComponent(LightType type);
		~LightComponent();

		LightType lightType;

		glm::vec3 lightDirection;

		glm::vec3 lightAmbient;
		glm::vec3 lightDiffuse;
		glm::vec3 lightSpecular;

		float constantAttenuation;
		float linearAttenuation;
		float quadraticAttenuation;

		float innerCutoff;
		float outerCutoff;

		virtual void update();

		GLLight toGLLight(glm::vec3& position);
};

#endif