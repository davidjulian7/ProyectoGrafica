#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vPos;
void main()
{
    vec4 m = model * vec4(aPos, 1.0);
    vPos = m.xyz;
    gl_Position = projection * view * m;
}
