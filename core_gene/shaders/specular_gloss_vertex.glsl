#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texCoords;

out vec3 v_fragPos;
out vec3 v_normal;
out vec2 v_texCoords;

layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

void main() {
    v_fragPos = vec3(u_model * vec4(a_pos, 1.0));
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;
    v_texCoords = a_texCoords;
    gl_Position = projection * view * vec4(v_fragPos, 1.0);
}
