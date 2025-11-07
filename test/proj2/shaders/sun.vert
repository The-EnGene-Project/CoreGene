#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoord; // Use location 3 for compatibility

out vec2 v_texCoord;

// === Uniform Blocks ===
layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

void main()
{
    v_texCoord = aTexCoord;
    gl_Position = projection * view * u_model * vec4(aPos, 1.0);
}