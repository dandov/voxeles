// Required when linking GLEW as a static library.
#define GLEW_STATIC

#include <iostream>
#include <cassert>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
	GLuint vbo;
	GLuint ibo;
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

	const float width = 1080;
	const float height = 1080;
	const float aspect_ratio = width / height;
	// TODO(dandov): Disable resizing from because the aspect ratio is used to
	// calculate the perspective matrix.
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Voxels", nullptr, nullptr);
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

	// Use an identity matrix for |world_from_model|.
	glm::mat4 world_from_model = glm::mat4(1.0);
	// Set the camera parallel to the floor, in front and looking towards the
	// geometry from the +Z axis (outside the monitor).
	glm::mat4 view_from_world =
		glm::lookAt(
			glm::vec3(0.0f, 0.0f, 10.f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
	// FOVY of 45 degrees, precalculated aspect ratio from the window dimensions,
	// znear of 0.1 and zfar of 100 (relative values to the camera, z points
	// inside the screen).
	glm::mat4 proj_from_view =
		glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

	// Set the values of the uniforms.
	GLuint model_mat_loc =
		glGetUniformLocation(shader.program_id, "worldFromModel");
	glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &world_from_model[0][0]);
	GLuint view_mat_loc =
		glGetUniformLocation(shader.program_id, "viewFromWorld");
	glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &view_from_world[0][0]);
	GLuint proj_mat_loc =
		glGetUniformLocation(shader.program_id, "projFromView");
	glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_from_view[0][0]);
	assert(CheckGlError());
	
	const double PI = std::acos(-1);
	const double rotation_speed = PI / 2.0;
	float angle = 0.0;
	double previous_time = glfwGetTime();

	// Enable back face culling. Front faces are CCW.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// Set the color used to clear the screen.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	while (!glfwWindowShouldClose(window)) {
		// Clear the curren viewport using the current clear color. The value
		// passed to this function is a bitmask that defines which buffers
		// are cleared. In this case only the color buffer is cleared.
		glClear(GL_COLOR_BUFFER_BIT);

		// Update logic.
		glm::mat4 rot_matrix =
			glm::rotate(world_from_model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &rot_matrix[0][0]);

		double current_time = glfwGetTime();
		float dt = static_cast<float>(current_time - previous_time);
		previous_time = current_time;
		angle += rotation_speed * dt;

		// Rendering logic.
		//
		// Render using indices. Count and type refer to the IBO.
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);


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

	// Prepare the vertex and index buffer data. The default culled
	// face is CCW. Each vertex is 3 floats for position and 3 floats
	// for color.
	std::vector<GLfloat> vertices = {
		/* pos */ -1.0f, -1.0f, 0.0f, /* colors */ 0.0f, 0.0, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f
	};
	std::vector<GLubyte> indices = {
		0, 1, 2, 0, 2, 3
	};

	// Create the Vertex Array Object where all the buffers will be bound.
	glGenVertexArrays(1, &vertex_data->vao);
	glBindVertexArray(vertex_data->vao);

	// Create and upload the VBO for the positions.
	glGenBuffers(1, &vertex_data->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_data->vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		vertices.size() * sizeof(GLfloat),
		&vertices[0],
		GL_STATIC_DRAW);
	// The id of the attribute. Matches location of "posModel" in the shader.
	const int vertex_size = 6 * sizeof(GLfloat);
	const GLuint pos_attrib_id = 0;
	glVertexAttribPointer(pos_attrib_id, 3, GL_FLOAT, GL_FALSE, vertex_size,
		reinterpret_cast<GLvoid*>(0));
	glEnableVertexAttribArray(pos_attrib_id);
	// The id of the attribute. Matches location of "color" in the shader.
	const GLuint color_attrib_id = 1;
	// The color has an offset 3 floats within the vertex (last arg).
	glVertexAttribPointer(color_attrib_id, 3, GL_FLOAT, GL_FALSE, vertex_size,
		reinterpret_cast<GLvoid*>(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(color_attrib_id);

	// Create and upload the IBO for the indices of the triangles.
	glGenBuffers(1, &vertex_data->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_data->ibo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		indices.size() * sizeof(GLubyte),
		&indices[0],
		GL_STATIC_DRAW);

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

	// Unbind and destroy the VBO.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vertex_data->vbo);

	// Unbind and destroy the IBO.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vertex_data->ibo);

	// Unbind the VAO.
	glBindVertexArray(0);
	// Destroy the VAO.
	glDeleteVertexArrays(1, &vertex_data->vao);

	assert(CheckGlError());
}