#version 410

layout (location=0) in vec4 vertex;
layout (location=1) in vec2 textureCoord;

out vec2 vertexTexCoord;

uniform mat4 M;

void main (void)
{
  vertexTexCoord = textureCoord;
  gl_Position = M * vertex;
}

