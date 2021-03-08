#version 330

layout(location=0) in vec4 in_position;
layout(location=1) in vec2 in_texcoord;
layout(location=2) in vec4 in_normal;

uniform mat4 u_model;
uniform mat4 u_mvp;

out vec3 v;
out vec2 texcoord;
out vec3 n;

void main()
{
	gl_Position = u_mvp * in_position;
	v = vec3(u_model * in_position);
	n = normalize(transpose(inverse(mat3(u_model))) * vec3(in_normal));
	texcoord = in_texcoord;
}
