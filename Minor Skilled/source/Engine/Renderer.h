#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <map>
#include <bitset>

#include <glm\glm.hpp>

#include "../Engine/IBLMaps.h"

class Node;
class Shader;
class Texture;
class RenderComponent;
class LightComponent;
class CameraComponent;
class VertexArray;
class Buffer;
class Framebuffer;
class Renderbuffer;
class Debug;

class Renderer {
	public:
		Renderer(Debug* profiler);
		~Renderer();

		void render(std::vector<Node*>& renderables, std::vector<Node*>& lights, Node* mainCamera, Node* directionalLight, Texture* skybox);
		void renderEnvironmentMaps(std::vector<Node*>& renderables, Node* directionalLight, Texture* skybox);

		Texture* convertEquiToCube(Texture* skybox);

	private:
		//profiler
		Debug* _profiler;

		//opengl settings
		bool _vSync;

		//vertex data
		static const std::vector<float> _SkyboxVertices;
		static const std::vector<float> _ScreenQuadVertices;

		//environment map data
		std::map<RenderComponent*, Texture*> _environmentMaps;
		std::map<RenderComponent*, IBLMaps> _iblMaps;
		Texture* _brdfLUT;

		//shaders
		Shader* _equiToCubeShader;
		Shader* _lightingShader;
		Shader* _lightingShaderPbr;
		Shader* _shadowShader;
		Shader* _shadowCubeShader;
		Shader* _depthShader;
		Shader* _environmentShader;
		Shader* _irradianceShader;
		Shader* _prefilterShader;
		Shader* _brdfShader;
		Shader* _skyboxShader;
		Shader* _ssaoShader;
		Shader* _ssaoBlurShader;
		Shader* _ssrShader;
		Shader* _bloomBlurShader;
		Shader* _postProcessingShader;

		//texture buffers
		Texture* _gPosition;
		Texture* _gNormal;
		Texture* _gAlbedo;
		Texture* _gEmissionSpec;

		Texture* _gEnvironmentShiny;

		Texture* _gMetalRoughAO;
		Texture* _gIrradiance;
		Texture* _gPrefilter;
		Texture* _gReflectance;

		Texture* _shadowMap;
		std::vector<Texture*> _shadowCubeMaps;
		Texture* _sceneDepthBuffer;
		Texture* _sceneColorBuffer;
		Texture* _brightColorBuffer;
		Texture* _blurColorBuffers[2];

		Texture* _ssaoNoiseTexture;
		Texture* _ssaoColorBuffer;
		Texture* _ssaoBlurColorBuffer;
		Texture* _ssrColorBuffer;

		//VAOs, VBOs
		VertexArray* _skyboxVAO;
		VertexArray* _screenQuadVAO;

		Buffer* _skyboxVBO;
		Buffer* _screenQuadVBO;

		//UBOs, SSBOs
		Buffer* _matricesUBO;
		Buffer* _dataUBO;

		Buffer* _lightsSSBO;

		//FBOs, RBOs
		Framebuffer* _gBuffer;
		Framebuffer* _gBufferPbr;

		Framebuffer* _conversionFBO;
		Framebuffer* _shadowFBO;
		Framebuffer* _shadowCubeFBO;
		Framebuffer* _depthFBO;
		Framebuffer* _environmentFBO;
		Framebuffer* _hdrFBO;
		Framebuffer* _bloomFBO;
		Framebuffer* _bloomBlurFBOs[2];
		Framebuffer* _ssaoFBO;
		Framebuffer* _ssaoBlurFBO;
		Framebuffer* _ssrFBO;

		Renderbuffer* _gRBO;

		Renderbuffer* _conversionRBO;
		Renderbuffer* _environmentRBO;
		Renderbuffer* _hdrRBO;

		//kernels
		std::vector<glm::vec3> _ssaoKernel;

		//init functions
		void _initShaders();

		void _initSkyboxVAO();
		void _initScreenQuadVAO();

		void _initUniformBuffers();
		void _initShaderStorageBuffers();

		void _initGBuffers();
		void _initConversionFBO();
		void _initShadowFBO();
		void _initShadowCubeFBO();
		void _initDepthFBO();
		void _initEnvironmentFBO();
		void _initHdrFBO();
		void _initBlurFBOs();
		void _initSSAOFBOs();
		void _initSSRFBO();
		
		//environment render functions
		Texture* _renderEnvironmentMap(std::vector<std::pair<RenderComponent*, glm::mat4>>& renderComponents, glm::mat4& environmentProjection, glm::vec3& renderPos, RenderComponent* currentRenderComponent, Texture* skybox, LightComponent* dirLight, bool pbr);
		Texture* _renderIrradianceMap(Texture* environmentMap, glm::mat4& irradianceProjection);
		Texture* _renderPrefilterMap(Texture* environmentMap, glm::mat4& prefilterProjection);
		void _renderBrdfLUT();

		//render functions
		void _renderShadowMaps(std::vector<std::pair<RenderComponent*, glm::mat4>>& renderComponents, std::vector<glm::vec3>& pointLights, glm::mat4& lightSpaceMatrix);
		void _renderDepth(std::vector<std::pair<RenderComponent*, glm::mat4>>& renderComponents);
		void _renderGeometry(std::vector<std::pair<RenderComponent*, glm::mat4>>& solidRenderComponents, bool pbr);
		void _renderSSAO();
		void _renderSSAOBlur();
		void _renderSSR(CameraComponent* cameraComponent);
		void _renderLighting(Texture* skybox, unsigned int pointLightCount, bool dirShadows, bool pbr);
		void _renderScene(std::vector<std::pair<RenderComponent*, glm::mat4>>& renderComponents, unsigned int pointLightCount, bool dirShadow, bool bindFBO);
		void _renderSkybox(glm::mat4& viewMatrix, glm::mat4& projectionMatrix, Texture* skybox);
		void _renderPostProcessingQuad();

		//helper functions
		void _getSortedRenderComponents(std::vector<Node*>& renderables, glm::vec3& cameraPos, std::vector<std::pair<RenderComponent*, glm::mat4>>& solidRenderables, std::vector<std::pair<RenderComponent*, glm::mat4>>& blendRenderables);
		std::vector<glm::vec3> _getClosestPointLights(glm::vec3 cameraPos, std::vector<std::pair<LightComponent*, glm::vec3>>& lightComponents);

		void _fillUniformBuffers(glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::mat4& previousViewProjection, glm::mat4& lightSpaceMatrix, glm::vec3& cameraPos, glm::vec3& directionalLightPos, bool dirShadows, std::vector<glm::vec3>& pointLightPositions);
		void _fillShaderStorageBuffers(std::vector<std::pair<LightComponent*, glm::vec3>>& lightComponents);

		void _generateSSAOKernel();
		void _generateNoiseTexture();

		void _blitGDepthToHDR(bool pbr);

		void _updateDimensions();
		void _applyCullMode();
};

#endif