#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(samplerColor, fragTexCoord);
}