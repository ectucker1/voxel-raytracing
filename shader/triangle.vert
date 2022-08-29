#version 450

layout (location = 0) out vec3 vertColor;

void main() {
	const vec3 positions[3] = vec3[3](
		vec3(1.0f, 1.0f, 0.0f),
		vec3(-1.0f, 1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f)
	);

	const vec3 colors[3] = vec3[3](
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(00.f, 0.0f, 1.0f)
	);

	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
	vertColor = colors[gl_VertexIndex];
}
