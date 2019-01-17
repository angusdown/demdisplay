#version 330 core
in vec4 vertexColor;
in vec2 TexCoord;

out vec4 color;

uniform sampler2D satTexture;

void main()
{
	//color = vertexColor;
	color = texture(satTexture, TexCoord);
}