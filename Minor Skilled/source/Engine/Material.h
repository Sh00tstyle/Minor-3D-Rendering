#ifndef MATERIAL_H
#define MATERIAL_H

#include <vector>

#include <glm/glm.hpp>

#include "../Utility/MaterialType.h"
#include "../Utility/BlendMode.h"

class Shader;
class LightComponent;

class Material {
	public:
		~Material();

		MaterialType getMaterialType();
		BlendMode getBlendMode();
		void setBlendMode(BlendMode blendMode);

		void setCastsShadows(bool value);
		bool& getCastsShadows();

		virtual void drawSimple(Shader* shader) = 0;
		virtual void drawForward(glm::mat4& modelMatrix) = 0;
		virtual void drawDeferred(glm::mat4& modelMatrix) = 0;

	protected:
		Material(MaterialType materialType, BlendMode blendMode, bool castsShadows);

		MaterialType _materialType;
		BlendMode _blendMode;
		bool _castsShadows;

		virtual void _initShader() = 0;

};

#endif