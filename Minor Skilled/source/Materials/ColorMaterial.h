#ifndef COLORMATERIAL_H
#define COLORMATERIAL_H

#include "../Engine/Material.h"

#include <glm/glm.hpp>

class Shader;
class LightComponent;

class ColorMaterial : public Material {
	public:
		ColorMaterial();
		ColorMaterial(glm::vec3 ambientColor, glm::vec3 diffuseColor, glm::vec3 specularColor, float shininess);
		~ColorMaterial();

		glm::vec3 getAmbientColor();
		glm::vec3 getDiffuseColor();
		glm::vec3 getSpecularColor();
		float getShininess();

		void setAmbientColor(glm::vec3 ambientColor);
		void setDiffuseColor(glm::vec3 diffuseColor);
		void setSpecularColor(glm::vec3 specularColor);
		void setShininess(float shininess);

		virtual void draw(glm::mat4& modelMatrix);

	private:
		static Shader* _Shader;

		glm::vec3 _ambientColor;
		glm::vec3 _diffuseColor;
		glm::vec3 _specularColor;
		float _shininess;

		virtual void _initShader();

};

#endif