#ifndef VOXEL_SHADERS
#define VOXEL_SHADERS
// TODO(dandov): Find a better way to do this because the way
// it is implemented right now will create a new copy of these
// strings for every compilation unit (files that includes this
// header).

namespace shaders {
	const GLchar* VERTEX_SHADER = R"(
#version 400

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
#version 400

in vec3 oColor;
out vec4 fragColor;

void main() {
	fragColor = vec4(oColor, 1.0);
}
)";

	const GLchar* QUAD_VERTEX_SHADER = R"(
#version 400

layout(location = 0) in vec3 posModel;
layout(location = 1) in vec3 texCoords;

out vec3 oEntryPoint;

uniform mat4 uWorldFromModel;
uniform mat4 uViewFromWorld;
uniform mat4 uProjFromView;

void main(void) {
	gl_Position = uProjFromView * uViewFromWorld * uWorldFromModel * vec4(posModel, 1.0);
	oEntryPoint = posModel;
}
)";
	const GLchar* QUAD_FRAGMENT_SHADER = R"(

#version 400

in vec3 oEntryPoint;

out vec4 fragColor;

uniform sampler1D tffSampler;
uniform sampler2D firstPassSampler;
uniform sampler3D voxelSampler;

void main() {
	// TODO(dandov): Pass this as uniform.
	vec2 screenSize = vec2(1080.0, 1080.0);
	// Calculate the texture coordinates by dividing by the screen size.
	vec2 uv = gl_FragCoord.xy / screenSize;
	// Sample the first pass texture to obtain the exit point of the ray.
	vec3 exitPoint = texture(firstPassSampler, uv).rgb;

	vec3 rayDir = exitPoint - oEntryPoint;
	vec3 normRayDir = normalize(rayDir);
	
	// TODO(dandov): Pass these as uniforms.
	float sampleCount = 1000.0;
	// TODO(dandov): Maybe do something smarter here because it is a waste to
	// to sample so many times in fragments that have small lenghts.
	float stepSize = length(rayDir) / sampleCount;
	vec4 backgroundColor = vec4(1.0, 1.0, 1.0, 0.0);

	vec3 finalColor = vec3(0.0);
	float finalAlpha = 0.0;
	for (int i = 0; i < sampleCount; i++) {
		if (finalAlpha > 1.0) {
			break;
		}
		// Update the ray and sample the volume.
		vec3 currentPos = oEntryPoint + (normRayDir * (stepSize * i));
		float voxel = texture(voxelSampler, currentPos).r;
	
		// Transform the voxel into a color using the transfer function.
		vec4 voxelColor = texture(tffSampler, voxel);
		// Don't forget to premultiply the alpha. This fixes overflow issues when
		// compositing.
		voxelColor.rgb *= voxelColor.a;

		// Now just do front-to-back compositing.
		finalColor = (1 - finalAlpha) * voxelColor.rgb + finalColor;
		finalAlpha = (1 - finalAlpha) * voxelColor.a + finalAlpha;
	}

	// This one samples the voxel at the center of the dataset in the xy entry point.
	// float voxel = texture(voxelSampler, vec3(oEntryPoint.xy, 0.5)).r;
	// fragColor = vec4(voxel, 0.0, 0.0, voxel);
	// fragColor = vec4(normRayDir * -1.0 , 1.0);
	fragColor = vec4(finalColor, finalAlpha);
}
)";
}  // namespace shaders

#endif  // VOXEL_SHADERS