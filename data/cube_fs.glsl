#version 330

in vec3 v;
in vec3 n;

out vec4 color;

void main()
{
	vec3 nn = normalize(n);
	vec3 l = normalize(vec3(16, 32, 64) - v);
	vec3 r = -reflect(l, nn);
	vec3 ambient = vec3(0.1, 0.0, 0.0);
	vec3 diffuse = vec3(0.5, 0.1, 0.1) * max(dot(nn, l), 0);
	color = vec4(ambient + diffuse, 1);
}
