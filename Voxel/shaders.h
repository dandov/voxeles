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

uniform mat4 worldFromModel;
uniform mat4 viewFromWorld;
uniform mat4 projFromView;

void main(void) {
	gl_Position = projFromView * viewFromWorld * worldFromModel * vec4(posModel, 1.0);
	oColor = vec3(posModel.xy, 1.0 - posModel.z);
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
layout(location = 1) in vec3 color;

out vec3 oColor;

void main(void) {
	gl_Position = vec4(posModel, 1.0);
	oColor = vec3(1.0, 0.0, 0.0);
}
)";
	const GLchar* QUAD_FRAGMENT_SHADER = R"(
#version 450

in vec3 oColor;
out vec4 fragColor;

void main() {
	fragColor = vec4(oColor, 1.0);
}
)";
}  // namespace shaders

#endif  // VOXEL_SHADERS