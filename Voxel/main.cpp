#define GLEW_STATIC

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main(int argc, char* argv[]) {
	glewExperimental = GL_TRUE;

	glfwSetErrorCallback([] (int error_code, const char* error_message) {
		std::cout << "GLFW ERROR[" << error_code << "]: "
				  << error_message << "\n";
	});

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW.\n";
		return 0;
	}

	// Ask for desktop OpenGL 4.5.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	// Request only core functionality i.e. without pre 3.1 deprecated APIs.
	// Use GLFW_OPENGL_COMPAT_PROFILE to get that deprecated stuff.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// For the requested version, ask to keep the deprecated APIs.
	// Otherwise deprecated APIs (currently a hint that they will be removed in
	// the future)  will be removed. This is only used on MacOS (of course).
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
	// Ask for an RGB888 buffer.
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 0);
	// 4x antialiasing. Number of samples for multisampling. I think it means
	// the buffer is 4x size and that allows to do 4 samples per pixel.
	glfwWindowHint(GLFW_SAMPLES, 4);
	// Request a double frame buffer.
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Voxels", nullptr, nullptr);
	if (!window) {
		std::cout << "Failed to create GLFW Window.\n";
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window);
	const GLubyte* gl_renderer = glGetString(GL_RENDERER);
	const GLubyte* gl_version = glGetString(GL_VERSION);
	const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cout << "Created GL Context: " << gl_renderer
		      << "\n        Version: " << gl_version
		      << "\n        GLSL Version: " << glsl_version;

	// Load OpenGL extensions. Not needed for this demo.
	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW.\n";
		glfwDestroyWindow(window);
		glfwTerminate();
		return 0;
	}

	glfwSetKeyCallback(
		window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			// Request exit when ESC is pressed.
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
				glfwSetWindowShouldClose(window, GLFW_TRUE);

			std::cout << "Pressed key: " << key << ", at time: "
					  << glfwGetTime() << "\n";
		});

	// V-Sync: Wait for |1| screen refresh to swap the buffers. If 0 is used then
	// the buffers will swap immediately and some of them might not be used if
	// the fps is faster than the refresh rate of the monitor.
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		// Update logic.

		// Rendering logic.


		glfwSwapBuffers(window);
		// Process input events.
		glfwPollEvents();
	}
	
	// Cleanup on exit.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}