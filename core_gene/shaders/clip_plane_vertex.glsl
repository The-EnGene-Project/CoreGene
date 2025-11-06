#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;

out vec3 v_fragPos;
out vec3 v_N;
out vec3 v_T;
out vec2 v_texCoord;

// === Matrizes padrão ===
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// === Multi Clip Planes ===
#define MAX_CLIP_PLANES 6
uniform vec4 clip_planes[MAX_CLIP_PLANES];
uniform int num_clip_planes;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    v_fragPos = worldPos.xyz;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    v_N = normalize(normalMatrix * aNormal);
    v_T = normalize(normalMatrix * aTangent);

    v_texCoord = aTexCoord;

    // === Calcula distância para cada plano ativo ===
    for (int i = 0; i < num_clip_planes; ++i) {
        gl_ClipDistance[i] = dot(worldPos, clip_planes[i]);
    }

    gl_Position = projection * view * worldPos;
}
