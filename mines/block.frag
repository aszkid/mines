#version 330 core

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

in vec3 normal;
in vec3 fragpos;

out vec4 color;

void main() {
	float specularStrength = 0.5;
	float ambientStrength = 0.2;
	vec3 ambient = ambientStrength * lightColor;
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(lightPos - fragpos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;
	vec3 viewDir = normalize(viewPos - fragpos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
	vec3 specular = specularStrength * spec * lightColor;
	vec3 result = (ambient + diffuse + specular) * objectColor;
	color = vec4(result, 1.0);
}