#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "../Engine/Component.h"

class Node;
class Model;
class Material;

class RenderComponent : public Component {
	public:
		RenderComponent(Model* model, Material* Material);
		~RenderComponent();

		Model* model;
		Material* material;

		virtual void update();

};

#endif