#version 330 core

uniform vec4 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float chunkSize;

in vec3 normal;
in vec3 fragpos;
in vec3 objectColor;

out vec4 color;

void main() {
	vec3 lightDir;
	// directional?
	if (lightPos.w == 0.0) {
		lightDir = normalize(-lightPos.xyz);
	} else {
		lightDir = normalize(lightPos.xyz - fragpos);
	}

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;
	vec3 norm = normalize(normal);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;
	vec3 result = (ambient + diffuse) * objectColor;

	float posx = mod(fragpos.x, chunkSize);
	float posy = mod(fragpos.y, chunkSize);
	float posz = mod(fragpos.z, chunkSize);
	const float epsilon = 0.5;
	if (posx < epsilon || posz < epsilon) {
		color = vec4(1.0, 0.0, 0.0, 1.0);
	} else {
		color = vec4(result, 1.0);
	}
}