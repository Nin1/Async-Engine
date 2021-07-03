#version 430 core
layout(location = 0) in vec3 vModelSpacePos;

uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(vModelSpacePos, 1);
}