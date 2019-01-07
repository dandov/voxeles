#ifndef VOXEL_SHADERS
#define VOXEL_SHADERS
// TODO(dandov): Find a better way to do this because the way
// it is implemented right now will create a new copy of these
// strings for every compilation unit (files that includes this
// header).

namespace shaders {
	const GLchar* VERTEX_SHADER = R"(
#version 450

layout(location = 0) in vec3 posModel;
layout(location = 1) in vec3 color;

out vec3 oColor;

uniform mat4 uWorldFromModel;
uniform mat4 uViewFromWorld;
uniform mat4 uProjFromView;

void main(void) {
	gl_Position = uProjFromView * uViewFromWorld * uWorldFromModel * vec4(posModel, 1.0);
	oColor = posModel;
}
)";
	const GLchar* FRAGMENT_SHADER = R"(
#version 450

in vec3 oColor;
out vec4 fragColor;

void main() {
	fragColor = vec4(oColor, 1.0);
}
)";

	const GLchar* QUAD_VERTEX_SHADER = R"(
#version 450

layout(location = 0) in vec3 posModel;
layout(location = 1) in vec3 texCoords;

out vec2 oTexCoords;
out vec3 oColor;

uniform mat4 uWorldFromModel;
uniform mat4 uViewFromWorld;
uniform mat4 uProjFromView;

void main(void) {
	oTexCoords = texCoords.xy;
	gl_Position = uProjFromView * uViewFromWorld * uWorldFromModel * vec4(posModel, 1.0);
	oColor = posModel;
}
)";
	const GLchar* QUAD_FRAGMENT_SHADER = R"(
#version 450

in vec2 oTexCoords;
in vec3 oColor;
out vec4 fragColor;

uniform sampler2D firstPassSampler;

void main() {
	// vec2 uv = vec2(oTexCoords.x, oTexCoords.y);
	// vec4 tex_color = texture(firstPassSampler, uv);
	fragColor = vec4(oColor, 1.0);
}
)";
}  // namespace shaders

#endif  // VOXEL_SHADERS