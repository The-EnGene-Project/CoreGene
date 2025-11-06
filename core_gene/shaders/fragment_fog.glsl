#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;
in vec3 v_tangent;
in vec2 v_texCoord;

// ========== Definições da cena ==========
#define MAX_SCENE_LIGHTS 16
#define LIGHT_TYPE_INACTIVE    0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT       2
#define LIGHT_TYPE_SPOT        3

struct LightData {
    vec4 position;
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation; // (const, linear, quad, cutoff)
    int type;
    int pad1;
    int pad2;
    int pad3;
};

layout (std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
} sceneLights;

layout (std140) uniform CameraPosition {
    vec4 u_viewPos;
};

// ========== Material e texturas ==========
uniform vec3  u_material_ambient;
uniform vec3  u_material_diffuse;
uniform vec3  u_material_specular;
uniform float u_material_shininess;

// Texturas opcionais
uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_roughnessMap;

uniform bool u_hasNormalMap;
uniform bool u_hasRoughnessMap;
uniform bool u_hasDiffuseMap;

// === Fog uniforms ===
uniform vec3 fcolor;     // cor da fog
uniform float fdensity;  // densidade da fog

// ======================================================
// --- calcula TBN no fragment shader a partir das normais e tangentes interpoladas ---
mat3 computeTBN(vec3 normal, vec3 tangent)
{
    tangent = normalize(tangent - normal * dot(normal, tangent));
    vec3 bitangent = normalize(cross(normal, tangent));
    return mat3(tangent, bitangent, normal);
}

void main()
{
    // --- normal base ---
    vec3 n = normalize(v_normal);

    // --- normal perturbada (se disponível) ---
    vec3 norm;
    if (u_hasNormalMap) {
        vec3 sampledNormal = texture(u_normalMap, v_texCoord).rgb;
        sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
        mat3 TBN = computeTBN(n, normalize(v_tangent));
        norm = normalize(TBN * sampledNormal);
    } else {
        norm = n;
    }

    // --- albedo (com ou sem textura) ---
    vec3 albedo = u_hasDiffuseMap ? texture(u_diffuseMap, v_texCoord).rgb
                                  : vec3(1.0);

    // --- roughness controla o brilho especular ---
    float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, v_texCoord).r : 0.5;
    float shininess = mix(512.0, 4.0, clamp(roughness, 0.0, 1.0));

    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos);
    vec3 totalLight = vec3(0.0);

    // ======================================================
    // Loop sobre todas as luzes ativas
    for (int i = 0; i < sceneLights.active_light_count; ++i) {
        LightData light = sceneLights.lights[i];

        if (light.type == LIGHT_TYPE_INACTIVE)
            continue;

        vec3 lightDir;
        float attenuation = 1.0;
        float spotFactor = 1.0;

        // --- Seleção de tipo de luz ---
        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            lightDir = normalize(-light.direction.xyz);
        }
        else if (light.type == LIGHT_TYPE_POINT) {
            vec3 toLight = light.position.xyz - v_fragPos;
            float dist = length(toLight);
            lightDir = normalize(toLight);
            attenuation = 1.0 / (light.attenuation.x +
                                 light.attenuation.y * dist +
                                 light.attenuation.z * dist * dist);
        }
        else if (light.type == LIGHT_TYPE_SPOT) {
            vec3 toLight = light.position.xyz - v_fragPos;
            float dist = length(toLight);
            lightDir = normalize(toLight);

            float theta = dot(lightDir, normalize(-light.direction.xyz));
            float cutoff = cos(radians(light.attenuation.w)); // w = cutoff em graus
            float epsilon = 0.05;
            spotFactor = clamp((theta - cutoff) / epsilon, 0.0, 1.0);

            attenuation = 1.0 / (light.attenuation.x +
                                 light.attenuation.y * dist +
                                 light.attenuation.z * dist * dist);
            attenuation *= spotFactor;
        }

        // --- iluminação base ---
        vec3 halfway = normalize(lightDir + viewDir);
        float diff = max(dot(norm, lightDir), 0.0);
        float spec = (diff > 0.0) ? pow(max(dot(norm, halfway), 0.0), shininess) : 0.0;

        vec3 ambient  = light.ambient.xyz * (u_material_ambient * albedo);
        vec3 diffuse  = light.diffuse.xyz * diff * (u_material_diffuse * albedo);
        vec3 specular = light.specular.xyz * spec * u_material_specular;

        totalLight += (ambient + diffuse + specular) * attenuation;
    }

    // === Fog ===
    float distance = length(u_viewPos.xyz - v_fragPos);
    float fogFactor = exp(-pow(fdensity * distance, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(fcolor, totalLight, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
