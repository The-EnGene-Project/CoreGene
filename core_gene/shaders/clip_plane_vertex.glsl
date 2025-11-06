#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;

out vec3 v_fragPos;
out vec3 v_normal;
out vec3 v_tangent;
out vec2 v_texCoord;

// === Matrizes padrão ===
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// === Multi Clip Planes ===
#define MAX_CLIP_PLANES 6
uniform vec4 clip_planes[MAX_CLIP_PLANES];
uniform int num_clip_planes;

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
