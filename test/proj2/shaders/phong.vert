#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTexCoord;

out vec3 v_worldPos;
out vec3 v_normal;
out vec3 v_tangent;
out vec2 v_texCoord;
out vec4 v_projTexCoord; // For projective texturing

// === Uniform Blocks ===
layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

// === Clip Planes ===
#define MAX_CLIP_PLANES 6
out float gl_ClipDistance[MAX_CLIP_PLANES];
uniform vec4 clip_planes[MAX_CLIP_PLANES];
uniform int num_clip_planes;

// === Model/Projector Uniforms ===
uniform mat4 u_model;
uniform mat4 u_projectorViewProj; // Projector's View * Projection matrix

void main()
{
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    vec4 viewPos = view * worldPos;
    
    v_worldPos = worldPos.xyz;
    v_texCoord = aTexCoord;

    // Calculate normals/tangents in world space
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    v_normal = normalize(normalMatrix * aNormal);
    v_tangent = normalize(normalMatrix * aTangent);

    // Calculate coordinate for projective texture
    // This is optional, but calculated for all objects
    v_projTexCoord = u_projectorViewProj * worldPos;

    gl_Position = projection * viewPos;

    // === Calculate clip distances in View Space ===
    // This matches the example file's behavior (clip_plane_vertex.glsl)
    // Note: If planes are defined in world space, you'd dot(worldPos, clip_planes[i])
    for (int i = 0; i < num_clip_planes; ++i) {
        gl_ClipDistance[i] = dot(viewPos, clip_planes[i]);
    }
}