#version 330 core

out vec4 FragColor;

in vec4 vxColor;

uniform vec4 objectColor;
uniform int useObjectColor;

void main()
{    
    if (useObjectColor == 1)
        FragColor = objectColor;
    else
        FragColor = vxColor;
}