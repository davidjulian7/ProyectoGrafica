#version 330 core

#extension GL_NV_shadow_samplers_cube : enable

// Adaptation for OpenGL/GLSL by: PhD Sergio Teodoro-Vite
// Based on: The CG Tutorial, Nvidia developer zone
// Chapter 7. Environment Mapping Techniques
// Theory and Programming: https://developer.download.nvidia.com/CgTutorial/cg_tutorial_chapter07.html

layout (location = 0) in vec3  aPos;
layout (location = 1) in vec3  aNormal;
layout (location = 2) in vec2  aTexCoords;
layout (location = 3) in vec3  tangent;
layout (location = 4) in vec3  bitangent;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 cameraPosition;

// Fresnel parameters
// bias, scale and power are values exposed to allow control over the appearance of the Fresnel effect
uniform float mRefractionRatio;
uniform float _Bias;
uniform float _Scale;
uniform float _Power;

// Outgoing Fresnel reflection and refraction parameters
out vec3  vReflect;
out vec3  vRefract[3];
out float reflectionCoefficient;


void main(void)
{
    vec4 in_Position = vec4(aPos,1.0f);

    gl_Position = projection * view * model * in_Position;  

    // Position of the vertex, in worldspace : model * position
    vec3 posWorld = (model * in_Position).xyz;

    // worldNormal: is the world space normal of the current point
    vec3 normWorld = normalize( mat3( model[0].xyz, model[1].xyz, model[2].xyz ) * aNormal );

    // Fresnel calculations
    // I is the vector from the eye to a point on the surface
    vec3 I = normalize(posWorld - cameraPosition);

    vReflect = reflect( I, normWorld );
    vRefract[0] = refract( I, normWorld, mRefractionRatio * 1.0f ); // RED CHANNEL REFRACTION // 1.0
    vRefract[1] = refract( I, normWorld, mRefractionRatio * 0.99f ); // GREEN CHANNEL REFRACTION // 0.99
    vRefract[2] = refract( I, normWorld, mRefractionRatio * 0.98f ); // BLUE CHANNEL REFRACTION // 0.98

    // reflectionCoefficient:  Schlick Approximation
    // An Approximation of the Fresnel Equation
    reflectionCoefficient = max(0, min(1,_Bias + _Scale * pow( 1.0f + dot( I, normWorld ), _Power ))); // Fresnel equation
    
    TexCoords = aTexCoords;
}
