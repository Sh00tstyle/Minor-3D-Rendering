//main.cpp : Defines the entry point for the console application.

#include <iostream>
#include <map>

#include <glad\glad.h> //IMPORTANT: glad needs to the be included BEFORE glfw, throws errors otherwise!!!
#include <GLFW\glfw3.h>

#define STB_IMAGE_IMPLEMENTATION //IMPORTANT: has to be done once in the project BEFORE including std_image.h
#include "stb_image.h"

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Mesh.h"

//functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height); //register a callback function when the window gets resized
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* filepath, bool gammaCorrection);
unsigned int loadCubemap(std::vector<std::string> faces);

//screen settings
unsigned int screenWidth = 800;
unsigned int screenHeight = 600;
bool useGamma = false;
bool gammaKeyPressed = false;

//time
float deltaTime = 0.0f; //time between the current and the last frame
float lastFrameTime = 0.0f; //time of the last frame

//camera setup
Camera* camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastMouseX = screenWidth / 2.0f;
float lastMouseY = screenHeight / 2.0f;
bool firstMouse = true;

int main() {
	//initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//create a window object
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Minor Skilled Rendering", NULL, NULL);

	if(window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	//register callbacks
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //register the window resize callback function															
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//set input mode
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //"captures" the cursor and makes it invisible

	//initialize GLAD
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST); //enable depth test (z-buffer/depth buffer)

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//create shader programs
	Shader* gammaShader = new Shader("assets/shader/gamma.vs", "assets/shader/gamma.fs");

	float planeVertices[] = {
		//positions            //normals         //texcoords
		10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
		-10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

		10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
		-10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
		10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
	};

	//plane VAO
	unsigned int planeVAO, planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	glBindVertexArray(0);

	//load textures
	unsigned int floorTexture = loadTexture("assets/textures/wood.png", false);
	unsigned int floorTextureGammaCorrected = loadTexture("assets/textures/wood.png", true);

	//shader configuration
	gammaShader->use();
	gammaShader->setInt("floorTexture", 0);

	//light information
	glm::vec3 lightPositions[] = {
		glm::vec3(-3.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(3.0f, 0.0f, 0.0f)
	};

	glm::vec3 lightColors[] = {
		glm::vec3(0.25),
		glm::vec3(0.50),
		glm::vec3(0.75),
		glm::vec3(1.00)
	};

	//render loop
	while(!glfwWindowShouldClose(window)) {
		//update time
		float currentFrameTime = (float)glfwGetTime();
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		//input
		processInput(window);

		//clear relevant buffers
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//setup MVP matrix
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = camera->getViewMatrix();
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera->zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

		//set shader uniforms
		gammaShader->use();
		gammaShader->setMat4("viewMatrix", viewMatrix);
		gammaShader->setMat4("projectionMatrix", projectionMatrix);

		glUniform3fv(glGetUniformLocation(gammaShader->ID, "lightPositions"), 4, &lightPositions[0][0]); //pássing uniform array
		glUniform3fv(glGetUniformLocation(gammaShader->ID, "lightColors"), 4, &lightColors[0][0]);
		gammaShader->setVec3("cameraPos", camera->position);
		gammaShader->setInt("useGamma", useGamma);

		//draw plane
		glBindVertexArray(planeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, useGamma ? floorTextureGammaCorrected : floorTexture); //use the corrected texture or the uncorrected one based on the useGamma bool
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		//check and call events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	//cleanup resources
	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	//update screen height und width
	screenWidth = width;
	screenHeight = height;
	glViewport(0, 0, width, height); //change the viewport everytime the window is resized
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
	if(firstMouse) {
		//initialize
		lastMouseX = xPos;
		lastMouseY = yPos;
		firstMouse = false;
	}

	//calculate offset movement between the last and the current frame
	float xOffset = xPos - lastMouseX;
	float yOffset = lastMouseY - yPos; //reversed since y-coordinates range from bottom to top

	lastMouseX = xPos;
	lastMouseY = yPos;

	camera->processMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
	camera->processMouseScroll((float)yOffset);
}

void processInput(GLFWwindow* window) {
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true); //tell the window to close when pressign escape

	float cameraSpeed = 2.5f * deltaTime;

	//camera movement input
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera->processKeyboard(FORWARD, deltaTime);
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera->processKeyboard(BACKWARD, deltaTime);
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera->processKeyboard(LEFT, deltaTime);
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera->processKeyboard(RIGHT, deltaTime);

	//lighting mode toggle
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !gammaKeyPressed) {
		useGamma = !useGamma;
		gammaKeyPressed = true;

		std::cout << (useGamma ? "Gamma enabled" : "Gamma disabled") << std::endl; //outputs the lighting model we are using at the moment
	}

	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
		gammaKeyPressed = false;
	}
}

unsigned int loadTexture(char const* filepath, bool gammaCorrection) {
	//create opengl texture object
	unsigned int textureID;
	glGenTextures(1, &textureID);

	//load texture from file
	int width, height, nrComponents;
	unsigned char* textureData = stbi_load(filepath, &width, &height, &nrComponents, 0);

	if(textureData) {
		//identify format
		GLenum internalFormat;
		GLenum dataFormat;

		if(nrComponents == 1) {
			internalFormat = dataFormat = GL_RED;
		} else if(nrComponents == 3) {
			internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		} else if(nrComponents == 4) {
			internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
			dataFormat = GL_RGBA;
		}

		//load texture into opengl
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);

		//set texture filter options
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(textureData); //free memory

	} else {
		std::cout << "Texture failed to load at path: " << filepath << std::endl;
		stbi_image_free(textureData); //free memory
	}

	return textureID; //texture id
}

unsigned int loadCubemap(std::vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	std::string filename;

	for(unsigned int i = 0; i < faces.size(); i++) {
		filename = "assets/skybox/" + faces[i];

		unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
		if(data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		} else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}