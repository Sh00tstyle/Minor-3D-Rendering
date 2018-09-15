// main.cpp : Defines the entry point for the console application.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <glad\glad.h> //IMPORTANT: glad needs to the be included BEFORE glfw, throws errors otherwise!!!
#include <GLFW\glfw3.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"

#define STB_IMAGE_IMPLEMENTATION //IMPORTANT: has to be done once in the project BEFORE including std_image.h
#include "stb_image.h"

//functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height); //register a callback function when the window gets resized
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
void processInput(GLFWwindow* window);

//screen settings
unsigned int screenWidth = 800;
unsigned int screenHeight = 600;

//time
float deltaTime = 0.0f; //time between the current and the last frame
float lastFrameTime = 0.0f; //time of the last frame

//camera setup
Camera* camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastMouseX = screenWidth / 2.0f;
float lastMouseY = screenHeight / 2.0f;
bool firstMouse = true;

//lighting
glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);

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

	//enable depth test (z-buffer/depth buffer)
	glEnable(GL_DEPTH_TEST); 

	//create shader programs
	Shader* lightingShader = new Shader("assets/shader/colors.vs", "assets/shader/colors.fs");
	Shader* lampShader = new Shader("assets/shader/lamp.vs", "assets/shader/lamp.fs");

	//cube vertices
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	//generate cube vao
	unsigned int cubeVAO; //vertex array object, stores all configurations done after binding it
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);

	//generate vbo
	unsigned int VBO; //vertex buffer object, allocates memory for the vertex data
	glGenBuffers(1, &VBO);

	//copy vertices array into an OpenGL buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO); //bind the vbo to the GL_ARRAY_BUFFER target
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //copy data into the buffers memory

	//set vertex attribute pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); //position attribute
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); //normals, needoffset
	glEnableVertexAttribArray(1);

	//generate light vao
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO); //should still be bound, so no need to do it again

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//unbind
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//render loop
	while(!glfwWindowShouldClose(window)) {
		//update time
		float currentFrameTime = (float)glfwGetTime();
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		//input
		processInput(window);

		//rendering
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//set uniforms in the lighting shader
		glm::vec3 lightColor;
		lightColor.x = sin(glfwGetTime() * 2.0f);
		lightColor.y = sin(glfwGetTime() * 0.7f);
		lightColor.z = sin(glfwGetTime() * 1.3f);

		glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
		glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);

		lightingShader->use();
		lightingShader->setVec3("cameraPos", camera->position);

		lightingShader->setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
		lightingShader->setVec3("material.diffuse", 1.0f, 0.5f, 0.31f);
		lightingShader->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
		lightingShader->setFloat("material.shininess", 32.0f);

		lightingShader->setVec3("light.position", lightPos);
		lightingShader->setVec3("light.ambient", ambientColor);
		lightingShader->setVec3("light.diffuse", diffuseColor);
		lightingShader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);

		//mvp matrix
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = camera->getViewMatrix();
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera->zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

		lightingShader->setMat4("modelMatrix", modelMatrix);
		lightingShader->setMat4("viewMatrix", viewMatrix);
		lightingShader->setMat4("projectionMatrix", projectionMatrix);

		//render the cube
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//draw the lamp
		modelMatrix = glm::translate(modelMatrix, lightPos);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f)); //smaller cube

		lampShader->use();

		lampShader->setMat4("modelMatrix", modelMatrix);
		lampShader->setMat4("viewMatrix", viewMatrix);
		lampShader->setMat4("projectionMatrix", projectionMatrix);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//unbind (not necessarily needed)
		glBindVertexArray(0); //unbind vertex array

		//check and call events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	//cleanup resources
	glfwTerminate();
	delete lightingShader;
	delete lampShader;
	delete camera;

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
}