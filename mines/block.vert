#version 330 core

layout(location=0) in vec3 vpos;
layout(location=1) in vec3 vnormal;

out vec3 normal;
out vec3 fragpos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * model * vec4(vpos, 1.0);
	fragpos = vec3(model * vec4(vpos, 1.0));
	normal = vnormal;
}