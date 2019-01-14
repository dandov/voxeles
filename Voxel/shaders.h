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
#version 450

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
	// Discard pixels that are part of the background.
	if (oEntryPoint == exitPoint) {
		discard;
	}

	vec3 rayDir = exitPoint - oEntryPoint;
	vec3 normRayDir = normalize(rayDir);
	
	// TODO(dandov): Pass these as uniforms.
	int sampleCount = 1000;
	float stepSize = 1.0 / float(sampleCount);
	vec4 backgroundColor = vec4(1.0, 1.0, 1.0, 0.0);

	vec4 finalColor = vec4(0.0);
	for (int i = 0; i < sampleCount; i++) {
		vec3 currentPos = oEntryPoint + (normRayDir * (stepSize * i));
		float voxel = texture(voxelSampler, currentPos).r;
		// Convert the voxel value into a color using the transfer function.
		vec4 voxelColor = texture(tffSampler, voxel);
		
		// modulate the value of voxelColor.a
    	// front-to-back integration
    	if (finalColor.a > 0.0) {
    	    // accomodate for variable sampling rates (base interval defined by mod_compositing.frag)
    	    voxelColor.a = 1.0 - pow(1.0 - voxelColor.a, stepSize * 200.0f);
    	    finalColor.rgb += (1.0 - finalColor.a) * voxelColor.rgb * voxelColor.a;
    	    finalColor.a += (1.0 - finalColor.a) * voxelColor.a;
    	}
		if (i > 0) {
			finalColor.rgb = finalColor.rgb * finalColor.a +
				(1 - finalColor.a) * backgroundColor.rgb;
			break;
		} else if (finalColor.a > 1.0) {
			finalColor.a = 1.0;
			break;
		}
	}

	fragColor = texture(tffSampler, 255);
	// fragColor = finalColor;
	// fragColor = vec4(oEntryPoint, 1.0);
	// fragColor = vec4(exitPoint, 1.0);
}
)";
}  // namespace shaders

#endif  // VOXEL_SHADERS