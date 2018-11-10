#include "TextureMaterial.h"

#include <glm/glm.hpp>

#include "../Engine/Texture.h"
#include "../Engine/Shader.h"

#include "../Components/LightComponent.h"

#include "../Utility/Filepath.h"

Shader* TextureMaterial::_shader = nullptr;

TextureMaterial::TextureMaterial(Texture* diffuseMap, float shininess) :Material(BlendMode::Opaque), _diffuseMap(diffuseMap), _specularMap(nullptr),
_normalMap(nullptr), _emissionMap(nullptr), _heightMap(nullptr), _shininess(shininess), _heightScale(1.0f) {
	_initShader();
}

TextureMaterial::TextureMaterial(Texture * diffuseMap, Texture * specularMap, Texture * normalMap, Texture* emissionMap, Texture* heightMap, float shininess, float heightScale, BlendMode blendMode) :Material(blendMode),
_diffuseMap(diffuseMap), _specularMap(specularMap), _normalMap(normalMap), _heightMap(heightMap), _emissionMap(emissionMap), _shininess(shininess), _heightScale(heightScale) {
	_initShader();
}

TextureMaterial::~TextureMaterial() {
	delete _diffuseMap;
	delete _specularMap;
	delete _normalMap;
	delete _emissionMap;
	delete _heightMap;
}

Texture * TextureMaterial::getDiffuseMap() {
	return _diffuseMap;
}

Texture * TextureMaterial::getSpecularMap() {
	return _specularMap;
}

Texture * TextureMaterial::getNormalMap() {
	return _normalMap;
}

Texture * TextureMaterial::getEmissionMap() {
	return _emissionMap;
}

Texture * TextureMaterial::getHeightMap() {
	return _heightMap;
}

float TextureMaterial::getShininess() {
	return _shininess;
}

float TextureMaterial::getHeightScale() {
	return _heightScale;
}

void TextureMaterial::setDiffuseMap(Texture* diffuseMap) {
	_diffuseMap = diffuseMap;
}

void TextureMaterial::setSpecularMap(Texture* specularMap) {
	_specularMap = specularMap;
}

void TextureMaterial::setNormalMap(Texture* normalMap) {
	_normalMap = normalMap;
}

void TextureMaterial::setEmissionMap(Texture * emissionMap) {
	_emissionMap = emissionMap;
}

void TextureMaterial::setHeightMap(Texture * heightMap) {
	_heightMap = heightMap;
}

void TextureMaterial::setShininess(float shininess) {
	_shininess = shininess;
}

void TextureMaterial::setHeightScale(float heightScale) {
	_heightScale = heightScale;
}

void TextureMaterial::draw(glm::mat4& modelMatrix, glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3& cameraPos, std::vector<std::pair<LightComponent*, glm::vec3>>& lights) {
	_shader->use();

	//set mvp matrix
	_shader->setMat4("modelMatrix", modelMatrix);
	_shader->setMat4("viewMatrix", viewMatrix);
	_shader->setMat4("projectionMatrix", projectionMatrix);

	//set material textures and bools
	if(_diffuseMap != nullptr) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _diffuseMap->getID());
	} else {
		std::cout << "ERROR: No diffuse map in the texture material. Ensure that there is at least a diffuse map present!" << std::endl;
	}
	
	if(_specularMap != nullptr) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, _specularMap->getID());

		_shader->setBool("material.hasSpecular", true);
	} else {
		_shader->setBool("material.hasSpecular", false);
	}

	if(_normalMap != nullptr) {
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, _normalMap->getID());

		_shader->setBool("material.hasNormal", true);
	} else {
		_shader->setBool("material.hasNormal", false);
	}

	if(_emissionMap != nullptr) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, _emissionMap->getID());
	}

	if(_heightMap != nullptr) {
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, _heightMap->getID());

		_shader->setBool("material.hasHeight", true);
	} else {
		_shader->setBool("material.hasHeight", false);
	}

	//set material properties
	_shader->setFloat("material.shininess", _shininess);
	_shader->setFloat("material.heightScale", _heightScale);
	_shader->setInt("material.blendMode", _blendMode);

	//set camera pos
	_shader->setVec3("cameraPos", cameraPos);

	//set lights (should only be done here in forward rendering)
	LightComponent* currentLight;

	for(unsigned int i = 0; i < lights.size(); i++) {
		currentLight = lights[i].first;

		//set light properties (vertex shader)
		_shader->setVec3("vertLights[" + std::to_string(i) + "].position", lights[i].second);
		_shader->setVec3("vertLights[" + std::to_string(i) + "].direction", currentLight->lightDirection);

		//set light properties (fragment shader)
		_shader->setInt("fragLights[" + std::to_string(i) + "].type", currentLight->lightType);

		_shader->setVec3("fragLights[" + std::to_string(i) + "].diffuse", currentLight->lightDiffuse);
		_shader->setVec3("fragLights[" + std::to_string(i) + "].ambient", currentLight->lightAmbient);
		_shader->setVec3("fragLights[" + std::to_string(i) + "].specular", currentLight->lightSpecular);

		_shader->setFloat("fragLights[" + std::to_string(i) + "].constant", currentLight->constantAttenuation);
		_shader->setFloat("fragLights[" + std::to_string(i) + "].linear", currentLight->linearAttenuation);
		_shader->setFloat("fragLights[" + std::to_string(i) + "].quadratic", currentLight->quadraticAttenuation);
		_shader->setFloat("fragLights[" + std::to_string(i) + "].innerCutoff", currentLight->innerCutoff);
		_shader->setFloat("fragLights[" + std::to_string(i) + "].outerCutoff", currentLight->outerCutoff);

		if(i >= LightComponent::LightAmount) break; //right now the light array is capped to a maximum of 20 lights
	}
}

void TextureMaterial::_initShader() {
	//lazy initialize the shader for all texture materials (there is no need to create one for each material, since they are all the same)
	if(_shader == nullptr) {
		_shader = new Shader(Filepath::ShaderPath + "material shader/texture.vs", Filepath::ShaderPath + "material shader/texture.fs");

		_shader->use();
		_shader->setInt("material.diffuse", 0);
		_shader->setInt("material.specular", 1);
		_shader->setInt("material.normal", 2);
		_shader->setInt("material.emission", 3);
		_shader->setInt("material.height", 4);
	}
}