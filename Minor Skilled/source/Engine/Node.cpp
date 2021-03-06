#include "Node.h"

#include <iterator>

#include "../Engine/Component.h"
#include "../Engine/Transform.h"

#include "../Utility/ComponentType.h"

Node::Node(glm::vec3 localPosition, std::string name):_transform(new Transform(localPosition)), _name(name), _parent(nullptr) {
}

Node::~Node() {
	//delete transform
	delete _transform;

	//delete components
	for(std::map<ComponentType, Component*>::iterator it = _components.begin(); it != _components.end(); it++) {
		delete it->second;
	}

	//delete children
	for(unsigned int i = 0; i < _children.size(); i++) {
		delete _children[i];
	}
}

std::string Node::getName() {
	return _name;
}

Node* Node::getParent() {
	return _parent;
}

Transform* Node::getTransform() {
	return _transform;
}

std::vector<Node*>* Node::getChildren() {
	return &_children;
}

Node* Node::getChildAt(unsigned int index) {
	return _children[index];
}

unsigned int Node::getChildCount() {
	return _children.size();
}

Component* Node::getComponent(ComponentType type) {
	return _components[type];
}

bool Node::hasComponent(ComponentType type) {
	std::bitset<8> typeMask = type;

	return ((_componentMask & typeMask) == typeMask);
}

void Node::addChild(Node* node) {
	node->_setParent(this);

	_children.push_back(node);
}

void Node::removeChild(Node* node) {
	for(unsigned int i = 0; i < _children.size(); i++) {
		if(_children[i] == node) {
			_children.erase(_children.begin() + i);
			return;
		}
	}
}

void Node::addComponent(Component * component) {
	_components[component->getComponentType()] = component;
	component->setOwner(this);

	_componentMask |= component->getComponentType();
}

void Node::update(glm::mat4& parentTransform, std::vector<Node*>& renderables, std::vector<Node*>& lights) {
	_transform->decompose();

	//fill collections
	if(hasComponent(ComponentType::Render)) renderables.push_back(this);
	if(hasComponent(ComponentType::Light)) lights.push_back(this);

	//update components
	Component* component;

	for(std::map<ComponentType, Component*>::iterator it = _components.begin(); it != _components.end(); it++) {
		component = it->second;

		if(component != nullptr) component->update();
	}

	//calculate model matrix
	_transform->worldTransform = parentTransform * _transform->localTransform;

	//update children
	for(unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->update(_transform->worldTransform, renderables, lights);
	}
}

void Node::_setParent(Node * node) {
	_parent = node;
}