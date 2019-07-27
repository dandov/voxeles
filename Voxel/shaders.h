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

	vec4 finalColor = vec4(1.0);
	finalColor.a = 0.0;
	for (int i = 0; i < sampleCount; i++) {
		vec3 currentPos = oEntryPoint + (normRayDir * (stepSize * i));
		float voxel = texture(voxelSampler, currentPos).r;
		// Use this value to render different layers of the data. It seems that
		// the volume represents some kind of density. Mixing it first touch of
		// of the ray we can get parts like the head or skull.
		float threshold = 0.2;
		if (voxel > threshold) {
			// Don't use the transfer function because it requires a lot of knowledge
			// about what the volume data represents.
			finalColor = texture(tffSampler, voxel);
			// finalColor.rgb = vec3(1.0, 0.0, 0.0);
			finalColor.a = 1.0;
			break;
		}		
	}

	// This one samples the voxel at the center of the dataset in the xy entry point.
	// float voxel = texture(voxelSampler, vec3(oEntryPoint.xy, 0.5)).r;
	// fragColor = vec4(voxel, 0.0, 0.0, voxel);
	// fragColor = vec4(normRayDir * -1.0 , 1.0);
	fragColor = finalColor;
}
)";
}  // namespace shaders

#endif  // VOXEL_SHADERS