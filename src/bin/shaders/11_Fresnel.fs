#version 330 core

#extension GL_NV_shadow_samplers_cube : enable

// Adaptation for OpenGL/GLSL by: PhD Sergio Teodoro-Vite
// Based on: The CG Tutorial, Nvidia developer zone
// Chapter 7. Environment Mapping Techniques
// Theory and Programming: https://developer.download.nvidia.com/CgTutorial/cg_tutorial_chapter07.html

in vec2 TexCoords;

out vec4 FragColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// incoming Fresnel reflection and refraction parameters
in vec3  vReflect;
in vec3  vRefract[3];
in float reflectionCoefficient;

// the cubemap texture
uniform samplerCube cubetex;
uniform sampler2D texture_diffuse1;

void main(void)
{

	// Fresnel
	vec4 reflectedColor = textureCube( cubetex, vec3( vReflect.x, vReflect.yz ) );
	vec4 refractedColor = vec4( 0.0f );

	refractedColor.r = textureCube( cubetex, vec3( vRefract[0].x, vRefract[0].yz ) ).r;
	refractedColor.g = textureCube( cubetex, vec3( vRefract[1].x, vRefract[1].yz ) ).g;
	refractedColor.b = textureCube( cubetex, vec3( vRefract[2].x, vRefract[2].yz ) ).b;

	vec4 fresnelColor = reflectionCoefficient * reflectedColor + (1 - reflectionCoefficient) * refractedColor;
	vec4 texel = texture(texture_diffuse1, TexCoords);

	FragColor = fresnelColor;
	FragColor.a = 1.0f;
}
