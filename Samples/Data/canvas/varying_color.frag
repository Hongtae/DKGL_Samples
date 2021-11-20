#version 450

layout (location=0) in vec2 position;
layout (location=1) in vec2 texcoord;
layout (location=2) in vec4 color;

layout (location=0) out vec4 outFragColor;

void main(void)
{
	outFragColor = color;
}
