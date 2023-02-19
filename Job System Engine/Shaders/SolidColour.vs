#version 430 core
layout(location = 0) in vec3 vModelSpacePos;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
	gl_Position = projMat * viewMat * modelMat * vec4(vModelSpacePos, 1);
}