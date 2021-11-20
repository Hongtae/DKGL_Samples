#version 450

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
    vec2 vl2 = vl * vl;

    vec2 f1 = vl2 * ellipse.outerRadiusSqInv;
    if (f1.x + f1.y > 1.0)
        discard;

    vec2 f2 = vl2 * ellipse.innerRadiusSqInv;
    if (f2.x + f2.y < 1.0)
        discard;

	outFragColor = color;
}
