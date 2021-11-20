#version 450

layout (location=0) in vec2 position;
layout (location=1) in vec2 texcoord;
layout (location=2) in vec4 color;

layout (location=0) out vec2 outPosition;
layout (location=1) out vec2 outTexcoord;
layout (location=2) out vec4 outColor;

void main()
{
	outPosition = position;
	outTexcoord = texcoord;
	outColor = color;

    gl_Position = vec4(position, 0, 1);
}
