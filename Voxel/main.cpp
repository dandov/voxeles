// Required when linking GLEW as a static library.
#define GLEW_STATIC

#include <iostream>
#include <cassert>
#include <cstdint>
#include <math.h>
#include <fstream>
#include <sstream>
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

class Window {
  public:
	  ~Window() {
		  DestroyWindow(this);
	  }

	  static bool CreateWindow(Window* window, int width, int height);

	  GLFWwindow* handle;

  private:
	  static void DestroyWindow(Window* window);
};

// Struct that holds the ids of the different shader objects.
class Shader {
  public:
	~Shader() {
		DestroyShaders(this);
	}

	// Creates and uploads the shaders in shaders.h and stores their IDs
	// in |shader|.
	static bool CreateShaders(Shader* shader, const GLchar* vertex,
							  const GLchar* fragment);

	GLuint program_id;
	GLuint vertex_id;
	GLuint fragment_id;

  private:
	// Destroys the ids in |shader|.
	static void DestroyShaders(Shader* shader);
};

// Struct that holds the VAO and VBO ids for the vertex data.
class VertexData {
  public:
	~VertexData() {
		DestroyVertexData(this);
	}

	// Creates and sets up the VAO with its VBOs bound. |vertex_data| will hold
	// the id of the generated VAO and VBOs.
	static bool CreateAndUploadVertexData(
		VertexData* vertex_data,
		const std::vector<GLfloat> vertices,
		const std::vector<GLubyte> indices);

	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	size_t index_length;

  private:
	// Disables and destroys the VAO and associated VBOs of |vertex_data|.
	static void DestroyVertexData(VertexData* vertex_data);
};

class Texture {
  public:
	  ~Texture() {
		  DestroyTexture(this);
	  }

	  static bool CreateTexture(Texture* texture, int width, int height, void* data);

	  GLuint id;

  private:
	  static void DestroyTexture(Texture* texture);
};

class FrameBuffer {
  public:
	  ~FrameBuffer() {
		  DestroyFrameBuffer(this);
	  }

	  static bool CreateFrameBuffer(FrameBuffer* frame_buffer, int width, int height);

	  GLuint id;
	  GLuint depth_stencil_renderbuffer_id;
	  Texture texture;

  private:
	  static void DestroyFrameBuffer(FrameBuffer* frame_buffer);
};

// Creates the geometry data of a cube and sets it in |data|.
bool CreateCube(VertexData* data);

// Sets the camera uniforms of |shader|.
void SetCameraUniforms(const Shader& shader, float aspect_ratio);

// Creates a new FrameBuffer that will be stored in |frame_buffer| and
// sets it as a texture uniform in |shader|.
bool CreateFrameBufferTexture(const Shader& shader, int width, int height,
	FrameBuffer* frame_buffer);

int main(int argc, char* argv[]) {
	glfwSetErrorCallback([](int error_code, const char* error_message) {
		std::cout << "GLFW ERROR[" << error_code << "]: "
			<< error_message << "\n";
	});

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW.\n";
		return 0;
	}

	constexpr int width = 1080;
	constexpr int height = 1080;
	constexpr float aspect_ratio = static_cast<float>(width) / height;
	Window window;
	if (!Window::CreateWindow(&window, width, height)) {
		assert(false);
		return 0;
	}

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
		return 0;
	}
	// GLEW causes a GL error when initializing so all GL errors need to be
	// flushed. "GL ERROR[1280]: invalid enumerantAssertion failed".
	CheckGlError();

	Shader back_shader;
	if (!Shader::CreateShaders(
		&back_shader, shaders::VERTEX_SHADER, shaders::FRAGMENT_SHADER)) {
		assert(CheckGlError());
		return 0;
	}

	Shader front_shader;
	if (!Shader::CreateShaders(
		&front_shader, shaders::QUAD_VERTEX_SHADER,
		shaders::QUAD_FRAGMENT_SHADER)) {
		assert(CheckGlError());
		return 0;
	}

	VertexData vertex_data;
	if (!CreateCube(&vertex_data)) {
		assert(false);
		return 0;
	}

	// Initialize both shaders uniforms to their defaults.
	SetCameraUniforms(back_shader, aspect_ratio);
	SetCameraUniforms(front_shader, aspect_ratio);

	// Create an offscreen framebuffer.
	FrameBuffer back_face_buffer;
	if (!CreateFrameBufferTexture(
		front_shader, width, height, &back_face_buffer)) {
		return 0;
	}

	GLuint tff_tex_id;
	{
		// Read transfer function data.
		std::string tff_data;
		std::stringstream ss;
		std::ifstream file;
		file.open(
			"tff.dat", std::ifstream::in | std::ifstream::binary);
		if (!file.is_open()) {
			assert(false);
			return 0;
		}
		ss << file.rdbuf();
		file.close();
		tff_data = ss.str();
		for (int i = 0; i < 256; i++) {
			std::cout << "("
				<< +static_cast<uint8_t>(tff_data[i * 4]) << ", "
				<< +static_cast<uint8_t>(tff_data[i * 4 + 1]) << ", "
				<< +static_cast<uint8_t>(tff_data[i * 4 + 2]) << ", "
				<< +static_cast<uint8_t>(tff_data[i * 4 + 3]) << ")\n";
		}

		// Create texture and upload data to GPU.
		glGenTextures(1, &tff_tex_id);
		glBindTexture(GL_TEXTURE_1D, tff_tex_id);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		// For debugging GL_NEAREST makes 1.0 wrap to the initial value (a purplish color).
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// Sets how to read pixels. In this case reading 1 byte pixels.
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, tff_data.data());

		// Set the TFF texture uniform and bind it to texture unit 1.
		glUseProgram(front_shader.program_id);
		const GLuint tff_mat_loc =
			glGetUniformLocation(front_shader.program_id, "tffSampler");
		glActiveTexture(GL_TEXTURE0 + 1); // This is the same as GL_TEXTURE1.
		glBindTexture(GL_TEXTURE_1D, tff_tex_id);
		// Assign the texture unit to the sampler. 0 matches the active texture
		// GL_TEXTURE1.
		glUniform1i(tff_mat_loc, 1);
		glUseProgram(0);
		assert(CheckGlError());
	}

	GLuint voxel_tex_id;
	{
		// Read voxel data.
		std::string voxel_data;
		std::stringstream ss;
		std::ifstream file;
		file.open(
			"head256.raw", std::ifstream::in | std::ifstream::binary);
		if (!file.is_open()) {
			assert(false);
			return 0;
		}
		ss << file.rdbuf();
		file.close();
		voxel_data = ss.str();
		// Create texture and upload data to GPU.
		glGenTextures(1, &voxel_tex_id);
		glBindTexture(GL_TEXTURE_3D, voxel_tex_id);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		assert(CheckGlError());
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, 256, 256, 225, 0, GL_RED, GL_UNSIGNED_BYTE, voxel_data.data());
		assert(CheckGlError());
		// Set the voxel texture uniform and bind it to texture unit 2.
		glUseProgram(front_shader.program_id);
		const GLuint voxel_tex_loc =
			glGetUniformLocation(front_shader.program_id, "voxelSampler");
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, voxel_tex_id);
		// Assign the texture unit to the sampler. 0 matches the active texture
		// GL_TEXTURE1.
		glUniform1i(voxel_tex_loc, 2);
		glUseProgram(0);
		assert(CheckGlError());
	}

	const double PI = std::acos(-1);
	
	const glm::mat4 world_from_model =
		glm::scale(glm::mat4(1.0), glm::vec3(3.0)) *
		// Rotate the cube 90 deg on the X axis to make it face the camera.
		glm::rotate(glm::mat4(1.0f), static_cast<float>(PI) / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
		// The cube is located at (0, 0, 0) to (1, 1, 1) so move it to the center
		// of the screen i.e. (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5).
		glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, -0.5f));
	const GLuint model_mat_loc =
		glGetUniformLocation(back_shader.program_id, "uWorldFromModel");
	const GLuint model_mat_loc2 =
		glGetUniformLocation(front_shader.program_id, "uWorldFromModel");

	// Logic for rotating the cube.
	const double rotation_speed = PI / 2.0;
	float angle = 0.0;
	double previous_time = glfwGetTime();
	// Set the color used to clear the screen.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable blending. This allows the empty voxel of the volume to be transparent.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window.handle)) {
		// Update logic.
		double current_time = glfwGetTime();
		float dt = static_cast<float>(current_time - previous_time);
		previous_time = current_time;
		angle += rotation_speed * dt;
		glm::mat4 rot_matrix =
			glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
		rot_matrix = rot_matrix * world_from_model;

		// First render pass.
		//
		// Bind the first pass framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, back_face_buffer.id);
		// Clear the curren viewport using the current clear color. The value
		// passed to this function is a bitmask that defines which buffers
		// are cleared. In this case only the color buffer is cleared.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		// Setup the first pass.
		glUseProgram(back_shader.program_id);
		glBindVertexArray(vertex_data.vao);
		// Enable back face culling. Front faces are CCW.
		glEnable(GL_CULL_FACE);
		// To render the inside of the cube, cull the front faces.
		glCullFace(GL_FRONT);
		glFrontFace(GL_CCW);
		// Rotate.
		glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &rot_matrix[0][0]);
		// Render first pass to texture.
		glDrawElements(
			GL_TRIANGLES, vertex_data.index_length, GL_UNSIGNED_BYTE, nullptr);


		// Second render pass.
		//
		// Bind the window framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		// Setup the second pass.
		glUseProgram(front_shader.program_id);
		glBindVertexArray(vertex_data.vao);
		// Rotate.
		glUniformMatrix4fv(model_mat_loc2, 1, GL_FALSE, &rot_matrix[0][0]);
		// To render the outside of the cube, cull the back faces.
		glCullFace(GL_BACK);
		// Render the second pass to the main framebuffer.
		glDrawElements(
			GL_TRIANGLES, vertex_data.index_length, GL_UNSIGNED_BYTE, nullptr);


		// End of the frame.
		glfwSwapBuffers(window.handle);
		// Process input events.
		glfwPollEvents();
	}
	
	return 0;
}

bool CreateCube(VertexData* data) {
	// Prepare the vertex and index buffer data of the second pass. The
	// default culled face is CCW. Each vertex is 3 floats for position
	// and 3 floats for color.
	std::vector<GLfloat> vertices = {
		// Front face.
		0.0f, 0.0f, 1.0f, /* color = */1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, /* color = */1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, /* color = */1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 1.0f, /* color = */1.0f, 0.0f, 0.0f,
		// Back face
		0.0f, 0.0f, 0.0f, /* color = */0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, /* color = */0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, /* color = */0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, /* color = */0.0f, 0.0f, 1.0f,
	};
	std::vector<GLubyte> indices = {
		// Front face.
		0, 1, 2, 0, 2, 3,
		// Back face.
		5, 4, 7, 5, 7, 6,
		// Top Face.
		3, 2, 6, 3, 6, 7,
		// Bottom face.
		4, 5, 1, 4, 1, 0,
		// Right face.
		1, 5, 6, 1, 6, 2,
		// Left face.
		4, 0, 3, 4, 3, 7,
	};

	return VertexData::CreateAndUploadVertexData(data, vertices, indices);
}

void SetCameraUniforms(const Shader& shader, float aspect_ratio) {
	// Use an identity matrix for |world_from_model|.
	glm::mat4 world_from_model(1.0f);
	// Set the camera parallel to the floor, in front and looking towards the
	// geometry from the +Z axis (outside the monitor).
	glm::mat4 view_from_world =
		glm::lookAt(
			/* eye_pos = */ glm::vec3(0.0f, 0.0f, 10.f),
			/* look_at = */ glm::vec3(0.0f, 0.0f, 0.0f),
			/* up = */ glm::vec3(0.0f, 1.0f, 0.0f));
	// FOVY of 45 degrees, precalculated aspect ratio from the window dimensions,
	// znear of 0.1 and zfar of 100 (relative values to the camera, z points
	// inside the screen).
	glm::mat4 proj_from_view =
		glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);
	// Set the values of the uniforms of |shader|.
	glUseProgram(shader.program_id);
	GLuint model_mat_loc =
		glGetUniformLocation(shader.program_id, "uWorldFromModel");
	glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &world_from_model[0][0]);
	GLuint view_mat_loc =
		glGetUniformLocation(shader.program_id, "uViewFromWorld");
	glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &view_from_world[0][0]);
	GLuint proj_mat_loc =
		glGetUniformLocation(shader.program_id, "uProjFromView");
	glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_from_view[0][0]);
	glUseProgram(0);
	assert(CheckGlError());
}

bool CreateFrameBufferTexture(
	const Shader& shader, int width, int height, FrameBuffer* frame_buffer) {
	if (!FrameBuffer::CreateFrameBuffer(frame_buffer, width, height)) {
		assert(false);
		return false;
	}
	glUseProgram(shader.program_id);
	// Set the framebuffer as a texture in the sampler uniform.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buffer->texture.id);
	const GLuint tex_uniform_loc =
		glGetUniformLocation(shader.program_id, "firstPassSampler");
	// Assign the texture unit to the sampler. 0 matches the active texture
	// GL_TEXTURE0.
	glUniform1i(tex_uniform_loc, 0);
	// Unbind the program for cleanup.
	// Don't unbind the texture from the unit 0.
	glUseProgram(0);
	assert(CheckGlError());
}

// TODO(dandov): Modify this to have mipmaps and other stuff when needed.
bool Texture::CreateTexture(Texture* texture, int width, int height, void* data) {
	// Create the texture.
	glGenTextures(1, &texture->id);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	glTexImage2D(GL_TEXTURE_2D, /* level = */ 0, GL_RGB, width, height,
		/* border = */ 0, GL_RGB, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Clean before leaving this function.
	glBindTexture(GL_TEXTURE_2D, 0);

	return CheckGlError();
}

void Texture::DestroyTexture(Texture* texture) {
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &texture->id);
}


bool FrameBuffer::CreateFrameBuffer(FrameBuffer* frame_buffer, int width, int height) {
	glGenFramebuffers(1, &frame_buffer->id);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer->id);

	if (!Texture::CreateTexture(&frame_buffer->texture, width, height, nullptr)) {
		assert(false);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, frame_buffer->texture.id);
	// Set the texture that was just created as the data of the frame buffer.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		frame_buffer->texture.id, /* level = */ 0);

	// Create the depth and stencil attachments.
	glGenRenderbuffers(1, &frame_buffer->depth_stencil_renderbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, frame_buffer->depth_stencil_renderbuffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glEnable(GL_DEPTH_TEST);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	// Add the attachments to the frame buffer now that they are allocated.
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
		frame_buffer->depth_stencil_renderbuffer_id);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Make sure that everything is correct before unbinding the frame buffer.
	const bool success =
		glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

	// Clean before returning.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return success && CheckGlError();
}

void FrameBuffer::DestroyFrameBuffer(FrameBuffer* frame_buffer) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &frame_buffer->id);
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

bool Shader::CreateShaders(Shader* shader, const GLchar* vertex,
						 const GLchar* fragment) {
	// Create the vertex shader.
	shader->vertex_id = glCreateShader(GL_VERTEX_SHADER);
	// Sets the source of |shader->vertex_id| (only 1 shader) to
	// |shaders::VERTEX_SHADER|. The last argument is an array of string lengths
	// and nullptr tells GL that the strings are null terminated.
	glShaderSource(shader->vertex_id, 1, &vertex, nullptr);
	glCompileShader(shader->vertex_id);
	bool success = CheckShaderStatus(shader->vertex_id);
	assert(success);

	// Create the fragment shader.
	shader->fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
	// Same as in the vertex shader but for the fragment source.
	glShaderSource(shader->fragment_id, 1, &fragment, nullptr);
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
		return false;

	// Set the program as current so that it is used when rendering.
	glUseProgram(shader->program_id);
	assert(CheckGlError());

	return true;
}

void Shader::DestroyShaders(Shader* shader) {
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

bool VertexData::CreateAndUploadVertexData(
	VertexData* vertex_data, const std::vector<GLfloat> vertices,
	const std::vector<GLubyte> indices) {
	assert(vertex_data);

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
	vertex_data->index_length = indices.size();

	return CheckGlError();
}

void VertexData::DestroyVertexData(VertexData* vertex_data) {
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

bool Window::CreateWindow(Window* window, int width, int height) {
	// Ask for desktop OpenGL 4.5.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// Request only core functionality i.e. without pre 3.1 deprecated APIs.
	// Use GLFW_OPENGL_COMPAT_PROFILE to get that deprecated stuff.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// For the requested version, ask to keep the deprecated APIs.
	// Otherwise deprecated APIs (currently a hint that they will be removed in
	// the future)  will be removed. This is only used on MacOS (of course).
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
	// Ask for an RGB8888 buffer.
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);
	// 4x antialiasing. Number of samples for multisampling. I think it means
	// the buffer is 4x size and that allows to do 4 samples per pixel.
	glfwWindowHint(GLFW_SAMPLES, 4);
	// Request a double frame buffer.
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	const float aspect_ratio = static_cast<float>(width) / height;
	// TODO(dandov): Disable resizing from because the aspect ratio is used to
	// calculate the perspective matrix.
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window->handle = glfwCreateWindow(width, height, "Voxels", nullptr, nullptr);
	if (!window) {
		std::cout << "Failed to create GLFW Window.\n";
		glfwTerminate();
		return false;
	}

	// Callback function that gets notified when the window is resized,
	// that sets the OpenGL viewport accordingly.
	glfwSetWindowSizeCallback(
		window->handle, [](GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);
	});

	glfwSetKeyCallback(window->handle,
		[](GLFWwindow* window, int key, int scancode, int action, int mods) {
		// Request exit when ESC is pressed.
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		std::cout << "Pressed key: " << key << ", at time: "
			<< glfwGetTime() << "\n";
	});

	// Enable the OpenGL context.
	glfwMakeContextCurrent(window->handle);

	// V-Sync: Wait for |1| screen refresh to swap the buffers. If 0 is used then
	// the buffers will swap immediately and some of them might not be used if
	// the fps is faster than the refresh rate of the monitor.
	//
	// TODO(dandov): In the surface pro this value has to be 0, otherwise, the
	// frame rate is very choppy.
	glfwSwapInterval(0);

	return true;
}

void Window::DestroyWindow(Window* window) {
	glfwDestroyWindow(window->handle);
	glfwTerminate();
}