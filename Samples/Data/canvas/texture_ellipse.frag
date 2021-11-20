#version 450

layout (binding=0) uniform sampler2D image;

layout (push_constant) uniform Ellipse
{
	vec2 outerRadiusSqInv; // vec2(1/A^2, 1/B^2) where X^2 / A^2 + Y^2 / B^2 = 1
    vec2 innerRadiusSqInv;
	vec2 center;	// center of ellipse
} ellipse;

layout (location=0) in vec2 position;
layout (location=1) in vec2 texcoord;
layout (location=2) in vec4 color;

layout (location=0) out vec4 outFragColor;

void main()
{
	vec2 vl = position - ellipse.center;
	float form = vl.x * vl.x * ellipse.outerRadiusSqInv.x + vl.y * vl.y * ellipse.outerRadiusSqInv.y;
	if (form > 1.0)
		discard;
	outFragColor = texture(image, texcoord) * color;
}
