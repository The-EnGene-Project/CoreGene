#version 410

layout (location=0) in vec4 vertex;
layout (location=1) in vec4 icolor;

out vec4 vertexColor;

uniform mat4 M;

void main (void)
{
  vertexColor = icolor;
  gl_Position = M * vertex;
}

