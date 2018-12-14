#include "OverlayUI.h"

#include <iostream>
#include <vector>
#include <iomanip>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "../../dependencies/imgui/imgui.h"
#include "../../dependencies/imgui/imgui_impl_opengl3.h"
#include "../../dependencies/imgui/imgui_impl_glfw.h"

#include "../Engine/Window.h"
#include "../Engine/Debug.h"
#include "../Engine/World.h"
#include "../Engine/Node.h"
#include "../Engine/Transform.h"
#include "../Engine/Material.h"
#include "../Engine/Model.h"
#include "../Engine/Texture.h"

#include "../Components/CameraComponent.h"
#include "../Components/LightComponent.h"
#include "../Components/RenderComponent.h"

#include "../Materials/ColorMaterial.h"
#include "../Materials/TextureMaterial.h"
#include "../Materials/PBRMaterial.h"

#include "../Utility/RenderSettings.h"
#include "../Utility/Input.h"

OverlayUI::OverlayUI(Window* window, Debug* profiler) : _renderUI(true), _profiler(profiler), _activeNode(nullptr) {
	_initImgui(window);
}

OverlayUI::~OverlayUI() {
	//free resrouces
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void OverlayUI::setupFrame(World* world) {
	//start new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if(Input::GetKeyDown(Key::ENTER)) _renderUI = !_renderUI; //toggle UI

	//setup ui windows
	if(_renderUI) {
		_setupProfiler();
		_setupConsole();
		_setupInspector();
		_setupSettings();
		_setupHierarchy(world);
	}
}

void OverlayUI::render() {
	//render the data we set up
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OverlayUI::_initImgui(Window* window) {
	//initialize imgui for OpenGL in the GLFW window context
	ImGui::CreateContext(); //create imgui context
	ImGui_ImplGlfw_InitForOpenGL(window->getGlfwWindow(), true); //tell imgui to use the glfw window
	ImGui_ImplOpenGL3_Init("#version 460"); //init imgui for opengl with the glsl version 4.6.0

	ImGui::StyleColorsDark();

	std::cout << "Initialized ImGui Overlay" << std::endl;
}

void OverlayUI::_setupProfiler() {
	//setup profiler window
	ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowPos(ImVec2(Window::ScreenWidth * 2.0f / 3.0f, Window::ScreenHeight * 4.8f / 6.0f));
	ImGui::SetWindowSize(ImVec2(Window::ScreenWidth * 1.0f / 3.0f, Window::ScreenHeight * 1.2f / 6.0f));

	//Info
	const char* glInfo = (const char*)glGetString(GL_VERSION);
	const char* hardwareInfo = (const char*)glGetString(GL_RENDERER);

	std::string glInfoString(glInfo);
	glInfoString = "OpenGL Version: \t\t\t\t\t" + glInfoString;

	std::string hardwareInfoString(hardwareInfo);
	hardwareInfoString = "Hardware:   \t\t\t\t\t\t" + hardwareInfoString;

	std::string resolutionInfoString = "Resolution: \t\t\t\t\t\t" + std::to_string(Window::ScreenWidth) + " x " + std::to_string(Window::ScreenHeight);

	ImGui::Text(glInfoString.c_str());
	ImGui::Text(hardwareInfoString.c_str());

	ImGui::Text(resolutionInfoString.c_str());

	ImGui::Text("\nFramerate:  \t\t\t\t\t\t%.2f FPS", ImGui::GetIO().Framerate);
	ImGui::Text("Frametime:  \t\t\t\t\t\t%.4f ms", 1000.0f / ImGui::GetIO().Framerate);

	//CPU
	ImGui::Text("Graph Update:   \t\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::Update));

	//GPU
	ImGui::Text("Rendering:  \t\t\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::Rendering));

	ImGui::Indent();

	if(ImGui::TreeNode("Details:")) {
		if(RenderSettings::IsEnabled(RenderSettings::Shadows)) {
			ImGui::Text("Shadow Pass:  \t\t\t\t%.4f ms", _profiler->getQuery(QueryType::Shadow));
		} else {
			ImGui::Text("Shadow Pass:  \t\t\t\t0.0000 ms");
		}

		if(RenderSettings::IsEnabled(RenderSettings::Deferred)) {
			ImGui::Text("Geometry Pass:\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::Geometry));
			ImGui::Text("Lighting Pass:\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::Lighting));

			if(RenderSettings::IsEnabled(RenderSettings::SSAO)) {
				ImGui::Text("SSAO Pass:\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::SSAO));
			} else {
				ImGui::Text("SSAO Pass:\t\t\t\t0.0000 ms");
			}
		} else {
			ImGui::Text("Geometry Pass:\t\t\t\t0.0000 ms");
			ImGui::Text("Lighting Pass:\t\t\t\t0.0000 ms");
			ImGui::Text("SSAO Pass:\t\t\t\t\t0.0000 ms");
		}

		ImGui::Text("Blending Pass:\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::Blending));
		ImGui::Text("Post Processing Pass: \t\t%.4f ms", _profiler->getQuery(QueryType::PostProcessing));
		ImGui::Text("UI Pass:  \t\t\t\t\t%.4f ms", _profiler->getQuery(QueryType::UI));

		ImGui::Text("Environment Prerender Pass:   %.4f ms", _profiler->getQuery(QueryType::Environment));

		ImGui::TreePop();
	}

	ImGui::End();
}

void OverlayUI::_setupConsole() {
	//setup console window
	ImGui::Begin("Console", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowPos(ImVec2(0, Window::ScreenHeight * 4.8f / 6.0f));
	ImGui::SetWindowSize(ImVec2(Window::ScreenWidth * 1.0f / 3.0f, Window::ScreenHeight * 1.2f / 6.0f));

	//post all messages from debug
	std::vector<std::string> logMessages = Debug::GetLogs();

	for(unsigned int i = 0; i < logMessages.size(); i++) {
		ImGui::Text(logMessages[i].c_str());
	}

	ImGui::End();
}

void OverlayUI::_setupSettings() {
	//setup render settings window
	ImGui::Begin("Render Settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowPos(ImVec2(Window::ScreenWidth * 1.0f / 3.0f, Window::ScreenHeight * 4.8f / 6.0f));
	ImGui::SetWindowSize(ImVec2(Window::ScreenWidth * 1.0f / 3.0f, Window::ScreenHeight * 1.2f / 6.0f));

	//disable deferred only settings if we are forward rendering
	if(!RenderSettings::IsEnabled(RenderSettings::Deferred)) {
		RenderSettings::Disable(RenderSettings::SSAO);
		RenderSettings::Disable(RenderSettings::PBR);
	}

	unsigned int settings = (unsigned int)(RenderSettings::Options.to_ulong()); //optain settings and convert to uint
	int bloomBlur = (int)RenderSettings::BloomBlurAmount;

	ImGui::Text("General");
	ImGui::CheckboxFlags("Shadows", &settings, RenderSettings::Shadows);
	ImGui::CheckboxFlags("Bloom", &settings, RenderSettings::Bloom);
	ImGui::CheckboxFlags("FXAA", &settings, RenderSettings::FXAA);
	ImGui::CheckboxFlags("Motion Blur", &settings, RenderSettings::MotionBlur);
	ImGui::Checkbox("vSync", &RenderSettings::VSync);
	ImGui::CheckboxFlags("Deferred", &settings, RenderSettings::Deferred);

	ImGui::Indent();
	ImGui::CheckboxFlags("SSAO", &settings, RenderSettings::SSAO);
	ImGui::CheckboxFlags("PBR", &settings, RenderSettings::PBR);
	ImGui::Unindent();

	ImGui::Text("\nPost Processing Settings");
	ImGui::InputFloat("Gamma", &RenderSettings::Gamma);
	ImGui::InputFloat("Exposure", &RenderSettings::Exposure);
	ImGui::InputInt("Bloom Blur", &bloomBlur);

	ImGui::Text("\nSSAO Settings");
	ImGui::InputFloat("Radius", &RenderSettings::SsaoRadius);
	ImGui::InputFloat("Bias", &RenderSettings::SsaoBias);
	ImGui::InputFloat("Power", &RenderSettings::SsaoPower);

	ImGui::Text("\nFXAA Settings");
	ImGui::InputFloat("Max Spam", &RenderSettings::FxaaSpanMax);
	ImGui::InputFloat("Min Reduce", &RenderSettings::FxaaReduceMin);
	ImGui::InputFloat("Mul Reduce", &RenderSettings::FxaaReduceMul);

	//apply settings
	RenderSettings::Options = settings; 
	RenderSettings::BloomBlurAmount = bloomBlur;

	ImGui::End();
}

void OverlayUI::_setupHierarchy(World* world) {
	//setup hierarchy window
	ImGui::Begin("Hierarchy", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::SetWindowSize(ImVec2(Window::ScreenWidth * 1.0f / 6.0f, Window::ScreenHeight * 4.8f / 6.0f));

	//draw all children and their children onto the UI
	Node* currentNode;

	for(unsigned int i = 0; i < world->getChildCount(); i++) {
		currentNode = world->getChildAt(i);

		_drawHierarchyNodes(currentNode, 0); //current depth is 0
	}

	ImGui::End();
}

void OverlayUI::_setupInspector() {
	//setup inspector window
	ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowPos(ImVec2(Window::ScreenWidth * 5.0f / 6.0f, 0));
	ImGui::SetWindowSize(ImVec2(Window::ScreenWidth * 1.0f / 6.0f, Window::ScreenHeight * 4.8f / 6.0f));

	//draw all properties of the nodes components
	bool nodeSelected = _activeNode != nullptr;

	if(nodeSelected) {
		//draw node components
		bool hasCamera = _activeNode->hasComponent(ComponentType::Camera);

		//draw name
		unsigned int indentions = 5;

		for(unsigned int i = 0; i < indentions; i++) ImGui::Indent();
		ImGui::Text((_activeNode->getName()).c_str()); //draw name with intentions
		for(unsigned int i = 0; i < indentions; i++) ImGui::Unindent();

		//draw transform when the node has no camera component
		if(!hasCamera && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			Transform* transform = _activeNode->getTransform();
			glm::vec3 localPosition = transform->getLocalPosition();
			glm::vec3 localScale = transform->getLocalScale();
			glm::quat localRotationQuat = transform->getLocalRotation();
			glm::vec3 localRotationEuler = glm::eulerAngles(localRotationQuat);

			ImGui::InputFloat3("Position", &localPosition.x);
			ImGui::InputFloat3("Rotation", &localRotationEuler.x);
			ImGui::InputFloat3("Scale", &localScale.x);

			//apply new transform
			glm::mat4 newLocalTransform = glm::mat4(1.0f);
			newLocalTransform = glm::eulerAngleYXZ(localRotationEuler.y, localRotationEuler.x, localRotationEuler.z); //rotate
			newLocalTransform = glm::translate(newLocalTransform, localPosition); //tanslate
			newLocalTransform = glm::scale(newLocalTransform, localScale); //scale

			transform->localTransform = newLocalTransform;
		}

		//draw camera component
		if(hasCamera) {
			if(ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen)) {
				CameraComponent* cameraComponent = (CameraComponent*)_activeNode->getComponent(ComponentType::Camera);

				ImGui::InputFloat("Field of View", &cameraComponent->fieldOfView);
				ImGui::InputFloat("Movement Speed", &cameraComponent->movementSpeed);
				ImGui::InputFloat("Rotation Speed", &cameraComponent->rotationSpeed);
			}
		}

		//draw light component
		if(_activeNode->hasComponent(ComponentType::Light)) {
			if(ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen)) {
				LightComponent* lightComponent = (LightComponent*)_activeNode->getComponent(ComponentType::Light);

				LightType lightType = lightComponent->lightType;

				switch(lightType) {
					case LightType::Directional:
						ImGui::Text("General");
						ImGui::Text("Light Type   \tDirectional Light\n");
						ImGui::ColorEdit3("Diffuse", &lightComponent->lightDiffuse.x);
						ImGui::ColorEdit3("Ambient", &lightComponent->lightAmbient.x);
						ImGui::ColorEdit3("Specular", &lightComponent->lightSpecular.x);
						ImGui::InputFloat3("Direction", &lightComponent->lightDirection.x);
						break;

					case LightType::Point:
						ImGui::Text("General");
						ImGui::Text("Light Type   \tPoint Light\n");
						ImGui::ColorEdit3("Diffuse", &lightComponent->lightDiffuse.x);
						ImGui::ColorEdit3("Ambient", &lightComponent->lightAmbient.x);
						ImGui::ColorEdit3("Specular", &lightComponent->lightSpecular.x);
						ImGui::Text("\nAttenuation");
						ImGui::InputFloat("Constant", &lightComponent->constantAttenuation);
						ImGui::InputFloat("Linear", &lightComponent->linearAttenuation);
						ImGui::InputFloat("Quadratic", &lightComponent->quadraticAttenuation);
						break;

					case LightType::Spot:
						ImGui::Text("General");
						ImGui::Text("Light Type   \tSpotlight\n");
						ImGui::ColorEdit3("Diffuse", &lightComponent->lightDiffuse.x);
						ImGui::ColorEdit3("Ambient", &lightComponent->lightAmbient.x);
						ImGui::ColorEdit3("Specular", &lightComponent->lightSpecular.x);
						ImGui::InputFloat3("Direction", &lightComponent->lightDirection.x);
						ImGui::Text("\nAttenuation");
						ImGui::InputFloat("Constant", &lightComponent->constantAttenuation);
						ImGui::InputFloat("Linear", &lightComponent->linearAttenuation);
						ImGui::InputFloat("Quadratic", &lightComponent->quadraticAttenuation);
						ImGui::Text("\nCutoff Angles");
						ImGui::InputFloat("Inner", &lightComponent->innerCutoff);
						ImGui::InputFloat("Outer", &lightComponent->outerCutoff);
						break;
				}
			}
		}

		//draw render component
		if(_activeNode->hasComponent(ComponentType::Render)) {
			if(ImGui::CollapsingHeader("Render Component", ImGuiTreeNodeFlags_DefaultOpen)) {
				RenderComponent* renderComponent = (RenderComponent*)_activeNode->getComponent(ComponentType::Render);

				//draw model information
				Model* model = renderComponent->model;

				std::string modelString = "Model\nFile\t" + _getNameFromPath(model->filepath);

				ImGui::Text(modelString.c_str());

				//draw material information
				Material* material = renderComponent->material;
				MaterialType materialType = material->getMaterialType();

				std::string blendModeString;

				switch(material->getBlendMode()) {
					case BlendMode::Opaque:
						blendModeString = "Blend Mode\t\tOpaque";
						break;

					case BlendMode::Cutout:
						blendModeString = "Blend Mode\t\tCutout";
						break;

					case BlendMode::Transparent:
						blendModeString = "Blend Mode\t\tTransparent";
						break;
				}

				ColorMaterial* colorMaterial;

				TextureMaterial* textureMaterial;
				std::string diffuseMapString;
				std::string specularMapString;
				std::string normalMapString;
				std::string emissionMapString;
				std::string heightMapString;
				std::string reflectionMapString;

				PBRMaterial* pbrMaterial;
				std::string albedoMapString;
				std::string metallicMapString;
				std::string roughnessMapString;
				std::string aoMapString;

				switch(materialType) {
					case MaterialType::Color:
						colorMaterial = (ColorMaterial*)material;

						ImGui::Text("\nColor Material");
						ImGui::Text(blendModeString.c_str());
						ImGui::Checkbox("Casts shadows", &colorMaterial->getCastsShadows());
						ImGui::InputFloat("Shininess", &colorMaterial->getShininess());
						ImGui::ColorEdit3("Diffuse Color", &colorMaterial->getDiffuseColor().x);
						ImGui::ColorEdit3("Ambient Color", &colorMaterial->getAmbientColor().x);
						ImGui::ColorEdit3("Specular Color", &colorMaterial->getSpecularColor().x);
						break;

					case MaterialType::Textures:
						textureMaterial = (TextureMaterial*)material;

						ImGui::Text("\nTexture Material");
						ImGui::Text(blendModeString.c_str());
						ImGui::Checkbox("Casts shadows", &textureMaterial->getCastsShadows());
						ImGui::InputFloat("Shininess", &textureMaterial->getShininess());
						ImGui::Text(diffuseMapString.c_str());
						ImGui::Image(ImTextureID(textureMaterial->getDiffuseMap()->getID()), ImVec2(64, 64));

						if(textureMaterial->getSpecularMap() != nullptr) {
							specularMapString = "Specular Map\t" + _getNameFromPath(textureMaterial->getSpecularMap()->filepath);
							
							ImGui::Text(specularMapString.c_str());
							ImGui::Image(ImTextureID(textureMaterial->getSpecularMap()->getID()), ImVec2(64, 64));
			
						}

						if(textureMaterial->getNormalMap() != nullptr) {
							normalMapString = "Normal Map\t" + _getNameFromPath(textureMaterial->getNormalMap()->filepath);
							ImGui::Text(normalMapString.c_str());
							ImGui::Image(ImTextureID(textureMaterial->getNormalMap()->getID()), ImVec2(64, 64));
						}

						if(textureMaterial->getEmissionMap() != nullptr) {
							emissionMapString = "Emission Map\t" + _getNameFromPath(textureMaterial->getEmissionMap()->filepath);
							ImGui::Text(emissionMapString.c_str());
							ImGui::Image(ImTextureID(textureMaterial->getEmissionMap()->getID()), ImVec2(64, 64));
						}

						if(textureMaterial->getHeightMap() != nullptr) {
							heightMapString = "Height Map\t" + _getNameFromPath(textureMaterial->getHeightMap()->filepath);

							ImGui::Text(heightMapString.c_str());
							ImGui::Image(ImTextureID(textureMaterial->getHeightMap()->getID()), ImVec2(64, 64));
							ImGui::InputFloat("Height Scale", &textureMaterial->getHeightScale());
						}

						if(textureMaterial->getReflectionMap() != nullptr) {
							reflectionMapString = "Reflection Map\t" + _getNameFromPath(textureMaterial->getReflectionMap()->filepath);
							
							ImGui::Text(reflectionMapString.c_str());
							ImGui::Image(ImTextureID(textureMaterial->getReflectionMap()->getID()), ImVec2(64, 64));
							ImGui::InputFloat("Refraction", &textureMaterial->getRefractionFactor());
						}

						break;

					case MaterialType::PBR:
						pbrMaterial = (PBRMaterial*)material;

						albedoMapString = "Albedo Map\t" + _getNameFromPath(pbrMaterial->getAlbedoMap()->filepath);
						normalMapString = "Normal Map\t" + _getNameFromPath(pbrMaterial->getNormalMap()->filepath);
						metallicMapString = "Metallic Map\t" + _getNameFromPath(pbrMaterial->getMetallicMap()->filepath);
						roughnessMapString = "Roughness Map\t" + _getNameFromPath(pbrMaterial->getRoughnessMap()->filepath);
						aoMapString = "Ambient Occlusion Map\t" + _getNameFromPath(pbrMaterial->getAoMap()->filepath);

						ImGui::Text("\nPBR Material");
						ImGui::Text(blendModeString.c_str());
						ImGui::Checkbox("Casts shadows", &pbrMaterial->getCastsShadows());
						ImGui::InputFloat("Refraction", &pbrMaterial->getRefractionFactor());
						ImGui::ColorEdit3("F0 Color", &pbrMaterial->getF0().x);

						ImGui::Text(albedoMapString.c_str());
						ImGui::Image(ImTextureID(pbrMaterial->getAlbedoMap()->getID()), ImVec2(64, 64));
						ImGui::Text(normalMapString.c_str());
						ImGui::Image(ImTextureID(pbrMaterial->getNormalMap()->getID()), ImVec2(64, 64));
						ImGui::Text(metallicMapString.c_str());
						ImGui::Image(ImTextureID(pbrMaterial->getMetallicMap()->getID()), ImVec2(64, 64));
						ImGui::Text(roughnessMapString.c_str());
						ImGui::Image(ImTextureID(pbrMaterial->getRoughnessMap()->getID()), ImVec2(64, 64));
						ImGui::Text(aoMapString.c_str());
						ImGui::Image(ImTextureID(pbrMaterial->getAoMap()->getID()), ImVec2(64, 64));

						if(pbrMaterial->getEmissionMap() != nullptr) {
							emissionMapString = "Emission Map\t" + _getNameFromPath(pbrMaterial->getEmissionMap()->filepath);
							ImGui::Text(emissionMapString.c_str());
							ImGui::Image(ImTextureID(pbrMaterial->getEmissionMap()->getID()), ImVec2(64, 64));
						}

						if(pbrMaterial->getHeightMap() != nullptr) {
							heightMapString = "Height Map\t" + _getNameFromPath(pbrMaterial->getHeightMap()->filepath);

							ImGui::Text(heightMapString.c_str());
							ImGui::Image(ImTextureID(pbrMaterial->getHeightMap()->getID()), ImVec2(64, 64));
							ImGui::InputFloat("Height Scale", &pbrMaterial->getHeightScale());
						}

						break;
				}
			}
		}
	}

	ImGui::End();
}

void OverlayUI::_selectInspectorNode(Node* node) {
	_activeNode = node; //select node to draw
}

void OverlayUI::_drawHierarchyNodes(Node* node, unsigned int depth) {
	//add intendions based on the tree depth
	std::string name = "";

	for(unsigned int i = 0; i < depth; i++) {
		name += "\t";
	}

	name += node->getName();

	if(ImGui::Button(name.c_str())) {
		_selectInspectorNode(node);
	}

	ImGui::AlignTextToFramePadding();

	//draw child nodes as well recursively
	Node* currentNode;

	for(unsigned int i = 0; i < node->getChildCount(); i++) {
		currentNode = node->getChildAt(i);

		_drawHierarchyNodes(currentNode, depth + 1);
	}
}

std::string OverlayUI::_getVectorString(glm::vec3 vector, bool rgbFormat) {
	std::string x = _getFloatString(vector.x, 2);
	std::string y = _getFloatString(vector.y, 2);
	std::string z = _getFloatString(vector.z, 2);

	std::string formattedVectorString;

	if(rgbFormat) formattedVectorString = "r: " + x + " g: " + y + " b: " + z;
	else formattedVectorString = "x: " + x + " y: " + y + " z: " + z;

	return formattedVectorString;
}

std::string OverlayUI::_getFloatString(float value, unsigned int decimalPoints) {
	std::string formattedValue;

	_stream << std::fixed << std::setprecision(decimalPoints) << value;
	formattedValue = _stream.str();
	_stream.str("");

	return formattedValue;
}

std::string OverlayUI::_getNameFromPath(std::string path) {
	std::stringstream ss(path);
	std::string substring;

	while(std::getline(ss, substring, '/')) {}

	return substring; //last result
}