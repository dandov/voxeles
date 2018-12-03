// Required when linking GLEW as a static library.
#define GLEW_STATIC

#include <iostream>
#include <cassert>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shaders.h"

// Checks for GL errors, returns true if no errors are found and false
// otherwise. All GL errors are flushed.
bool CheckGlError() {
	bool result = true;
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR) {
		std::cout << "GL ERROR[" << gl_error << "]: "
				  << gluErrorString(gl_error) << "\n";
		gl_error = glGetError();
		result = false;
	}

	return result;
}

// Struct that holds the ids of the different shader objects.
struct Shader {
	GLuint program_id;
	GLuint vertex_id;
	GLuint fragment_id;
};

// Struct that holds the VAO and VBO ids for the vertex data.
struct VertexData {
	GLuint vao;
	GLuint vbo_pos;
	GLuint vbo_color;
};

// Creates and uploads the shaders in shaders.h and stores their IDs
// in |shader|.
void CreateShaders(Shader* shader);

// Destroys the ids in |shader|.
void DestroyShaders(Shader* shader);

// Creates and sets up the VAO with its VBOs bound. |vertex_data| will hold
// the id of the generated VAO and VBOs.
void CreateAndUploadVertexData(VertexData* vertex_data);

// Disables and destroys the VAO and associated VBOs of |vertex_data|.
void DestroyVertexData(VertexData* vertex_data);

int main(int argc, char* argv[]) {
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
	assert(CheckGlError());
	std::cout << "Created GL Context: " << gl_renderer
		      << "\n        Version: " << gl_version
		      << "\n        GLSL Version: " << glsl_version << "\n";

	// Load OpenGL extensions. Not needed for this demo.
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW.\n";
		glfwDestroyWindow(window);
		glfwTerminate();
		return 0;
	}
	// GLEW causes a GL error when initializing so all GL errors need to be
	// flushed. "GL ERROR[1280]: invalid enumerantAssertion failed".
	CheckGlError();


	// Callback function that gets notified when the window is resized,
	// that sets the OpenGL viewport accordingly.
	glfwSetWindowSizeCallback(
		window, [] (GLFWwindow* window, int width, int height) {
			glViewport(0, 0, width, height);
		});

	glfwSetKeyCallback(
		window, [] (GLFWwindow* window, int key, int scancode, int action, int mods) {
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

	Shader shader;
	CreateShaders(&shader);

	VertexData vertex_data;
	CreateAndUploadVertexData(&vertex_data);
	
	// Set the color used to clear the screen.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	while (!glfwWindowShouldClose(window)) {
		// Clear the curren viewport using the current clear color. The value
		// passed to this function is a bitmask that defines which buffers
		// are cleared. In this case only the color buffer is cleared.
		glClear(GL_COLOR_BUFFER_BIT);

		// Update logic.

		// Rendering logic.
		glDrawArrays(GL_TRIANGLES, 0, 3);


		glfwSwapBuffers(window);
		// Process input events.
		glfwPollEvents();
	}

	DestroyVertexData(&vertex_data);
	DestroyShaders(&shader);
	
	// Cleanup on exit.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

bool CheckShaderStatus(GLuint shader) {
	GLint is_compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled)
		return true;

	GLint max_length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

	// The maxLength includes the null character
	std::vector<GLchar> error_log(max_length);
	glGetShaderInfoLog(shader, max_length, &max_length, &error_log[0]);
	const std::string error_log_str(error_log.begin(), error_log.end());
	std::cout << "Shader compiler error: " << error_log_str.c_str() << "\n";

	// Don't leak the shader and destroy the shader.
	// TODO(dandov): Destroy other shaders before this one?
	glDeleteShader(shader);
	return false;
}

void CreateShaders(Shader* shader) {
	// Create the vertex shader.
	shader->vertex_id = glCreateShader(GL_VERTEX_SHADER);
	// Sets the source of |shader->vertex_id| (only 1 shader) to
	// |shaders::VERTEX_SHADER|. The last argument is an array of string lengths
	// and nullptr tells GL that the strings are null terminated.
	glShaderSource(shader->vertex_id, 1, &shaders::VERTEX_SHADER, nullptr);
	glCompileShader(shader->vertex_id);
	bool success = CheckShaderStatus(shader->vertex_id);
	assert(success);

	// Create the fragment shader.
	shader->fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
	// Same as in the vertex shader but for the fragment source.
	glShaderSource(shader->fragment_id, 1, &shaders::FRAGMENT_SHADER, nullptr);
	glCompileShader(shader->fragment_id);
	success = CheckShaderStatus(shader->fragment_id);
	assert(success);

	// Create a GL program.
	shader->program_id = glCreateProgram();
	// Attach the vertex and fragment shader of the program.
	glAttachShader(shader->program_id, shader->vertex_id);
	glAttachShader(shader->program_id, shader->fragment_id);
	// Link the program. At this stage the GLSL compiler will verify that the outputs
	// and corresponding inputs of the different stages match.
	glLinkProgram(shader->program_id);
	GLint program_success;
	glGetProgramiv(shader->program_id, GL_LINK_STATUS, &program_success);
	if (!program_success) {
		GLint length = 0;
		glGetProgramiv(shader->program_id, GL_INFO_LOG_LENGTH, &length);
		std::vector<GLchar> error_log(length);
		glGetProgramInfoLog(shader->program_id, length, &length, &error_log[0]);
		const std::string error_log_str(error_log.begin(), error_log.end());
		std::cout << "Program linker error: " << error_log_str.c_str() << "\n";

		success = false;
	}

	// Check for GL errors one last time.
	success = CheckGlError();

	// If the shader creation process failed at some stage clean everything up.
	if (!success)
		DestroyShaders(shader);

	// Set the program as current so that it is used when rendering.
	glUseProgram(shader->program_id);
	assert(CheckGlError());
}

void DestroyShaders(Shader* shader) {
	// Unbind the program in case it is being used.
	glUseProgram(0);
	// Detach the shaders from the program and delete them.
	glDetachShader(shader->program_id, shader->vertex_id);
	glDetachShader(shader->program_id, shader->fragment_id);
	glDeleteShader(shader->vertex_id);
	glDeleteShader(shader->fragment_id);
	// Now the program can be deleted.
	glDeleteProgram(shader->program_id);

	assert(CheckGlError());
}

void CreateAndUploadVertexData(VertexData* vertex_data) {
	assert(vertex_data);

	// Prepare the vertex buffer data. The default culled face is CCW.
	std::vector<GLfloat> vertices = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
	};

	// Match the location of the vertices above and do a gradient from left
	// to right, blue to red.
	std::vector<GLfloat> colors = {
		0.0f, 0.0, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0, 1.0f,
	};

	// Create the Vertex Array Object where all the buffers will be bound.
	glGenVertexArrays(1, &vertex_data->vao);
	glBindVertexArray(vertex_data->vao);

	// Create and upload the VBO for the positions.
	glGenBuffers(1, &vertex_data->vbo_pos);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_data->vbo_pos);
	glBufferData(
		GL_ARRAY_BUFFER,
		vertices.size() * sizeof(GLfloat),
		&vertices[0],
		GL_STATIC_DRAW);
	// The id of the attribute. Matches location of "posModel" in the shader.
	const GLuint pos_attrib_id = 0;
	glVertexAttribPointer(pos_attrib_id, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(pos_attrib_id);

	// Create and upload the VBO for the colors.
	glGenBuffers(1, &vertex_data->vbo_color);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_data->vbo_color);
	glBufferData(
		GL_ARRAY_BUFFER,
		colors.size() * sizeof(GLfloat),
		&colors[0],
		GL_STATIC_DRAW);
	// The id of the attribute. Matches location of "color" in the shader.
	const GLuint color_attrib_id = 1;
	glVertexAttribPointer(color_attrib_id, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(color_attrib_id);

	assert(CheckGlError());
}

void DestroyVertexData(VertexData* vertex_data) {
	// Disable the vertex attributes. These values match the ones used in
	// the matching functions "glVertexAttribPointer" and
	// "glEnableVertexAttribArray".
	const GLuint pos_attrib_id = 0;
	const GLuint color_attrib_id = 1;
	glDisableVertexAttribArray(pos_attrib_id);
	glDisableVertexAttribArray(color_attrib_id);

	// Unbind any of the VBOs.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Destroy the VBOs.
	glDeleteBuffers(1, &vertex_data->vbo_pos);
	glDeleteBuffers(1, &vertex_data->vbo_color);

	// Unbind the VAO.
	glBindVertexArray(0);
	// Destroy the VAO.
	glDeleteVertexArrays(1, &vertex_data->vao);

	assert(CheckGlError());
}