#version 450

layout (binding=0) uniform sampler2D image;

layout (location=0) in vec2 position;
layout (location=1) in vec2 texcoord;
layout (location=2) in vec4 color;

layout (location=0) out vec4 outFragColor;

void main(void)
{
	outFragColor = vec4(color.rgb, texture(image, texcoord).r * color.a);
}
