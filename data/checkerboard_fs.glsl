#version 330

in vec3 v;
in vec2 texcoord;
in vec3 n;

uniform sampler2D surface_texture;

out vec4 color;

void main()
{
	vec3 nn = normalize(n);
	vec3 l = normalize(vec3(0, 0, 8) - v);
	vec3 r = -reflect(l, nn);
	vec3 diffuse = vec3(1, 1, 1) * max(dot(nn, l), 0);
	vec4 c = texture(surface_texture, texcoord.xy);
	color = vec4(diffuse, 1) * c;
}
