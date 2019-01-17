#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out vec4 vertexColor;
out vec2 TexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float Max;
uniform float Min;
void main()
{
	vec3 p = position;
    p.y *= Max;
	gl_Position = projection * view * model * vec4(position.x,position.y,position.z, 1.0f);
	TexCoord = uv;
	//vertexColor = vec4(0.0f, 0.4f, 0.0f, 1.0f);
}