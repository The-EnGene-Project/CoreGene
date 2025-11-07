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

// === Multi Clip Planes ===
#define MAX_CLIP_PLANES 6
uniform vec4 clip_planes[MAX_CLIP_PLANES];
uniform int num_clip_planes;

uniform mat4 u_model;

void main()
{
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    v_fragPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    v_normal = normalize(normalMatrix * aNormal);
    v_tangent = normalize(normalMatrix * aTangent);


    v_texCoord = aTexCoord;

    // === Calcula distância para cada plano ativo ===
    for (int i = 0; i < num_clip_planes; ++i) {
        gl_ClipDistance[i] = dot(worldPos, clip_planes[i]);
    }

    gl_Position = u_projection * u_view * worldPos;

}
