#version 410 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec3 a_tangent;
layout (location = 3) in vec2 a_texCoord;

out vec3 v_fragPos;
out vec3 v_normal;
out vec3 v_tangent;
out vec2 v_texCoord;

// === blocos uniformes ===
layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

void main()
{
    // posição do fragmento em espaço do mundo
    v_fragPos = vec3(u_model * vec4(a_pos, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    v_normal = normalize(normalMatrix * aNormal);
    v_tangent = normalize(normalMatrix * aTangent);

    v_texCoord = a_texCoord;

    gl_Position = projection * view * u_model * vec4(a_pos, 1.0);
}
