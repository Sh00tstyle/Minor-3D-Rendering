#ifndef INPUT_H
#define INPUT_H

#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../Utility/Key.h"
#include "../Utility/MouseButton.h"

class Input {
	public:
		static void Initialize(GLFWwindow* window);

		static void ProcessInput();
		static void ResetMousePos();

		static bool GetKey(Key key);
		static bool GetKeyDown(Key key);
		static bool GetKeyUp(Key key);

		static bool GetMouse(MouseButton mouseButton);
		static bool GetMouseDown(MouseButton mouseButton);
		static bool GetMouseUp(MouseButton mouseButton);

		static glm::vec2 GetLastMousePos();
		static glm::vec2 GetCurrentMousePos();

	private:
		static std::map<Key, bool> _KeysReleased;

		static GLFWwindow* _Window;

		static bool _FirstMouse;
		static glm::vec2 _LastMousePos; //movement offset since the last frame
		static glm::vec2 _CurrentMousePos;

		static void _MouseCallback(GLFWwindow* window, double xPos, double yPos);
		static void _CheckKeyStatus();
};

#endif