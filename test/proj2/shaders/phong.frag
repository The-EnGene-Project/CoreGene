#version 410 core

out vec4 FragColor;

in vec3 v_worldPos;
in vec3 v_normal;
in vec3 v_tangent;
in vec2 v_texCoord;
in vec4 v_projTexCoord;

// ========== Scene Definitions ==========
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
    vec4 attenuation; // (const, linear, quad, cutoff_angle_degrees)
    int type;
    int pad1, pad2, pad3;
};

layout (std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
} sceneLights;

layout (std140) uniform CameraPosition {
    vec4 u_viewPos;
};

// ========== Material & Textures ==========
uniform vec3  u_material_ambient;
uniform vec3  u_material_diffuse;
uniform vec3  u_material_specular;
uniform float u_material_shininess;

uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_roughnessMap;

uniform bool u_hasDiffuseMap;
uniform bool u_hasNormalMap;
uniform bool u_hasRoughnessMap;

// ========== Project Extras ==========
// --- Fog ---
uniform vec3  fogcolor;
uniform float fogdensity;

// --- Reflection ---
uniform samplerCube u_skybox;
uniform bool u_enableReflection;
uniform float u_reflectionFactor; // Mix factor, e.g., 0.5

// --- Projective Texture ---
uniform sampler2D u_projectiveMap;
uniform bool u_enableProjectiveTex;

// ======================================================
// computeTBN from provided 'fragment_fog.glsl'
mat3 computeTBN(vec3 normal, vec3 tangent)
{
    tangent = normalize(tangent - normal * dot(normal, tangent));
    vec3 bitangent = normalize(cross(normal, tangent));
    return mat3(tangent, bitangent, normal);
}

void main()
{
    // --- 1. Get Normal ---
    vec3 n = normalize(v_normal);
    vec3 norm;
    if (u_hasNormalMap) {
        vec3 sampledNormal = texture(u_normalMap, v_texCoord).rgb;
        sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
        mat3 TBN = computeTBN(n, normalize(v_tangent));
        norm = normalize(TBN * sampledNormal);
    } else {
        norm = n;
    }

    // --- 2. Get Albedo (Diffuse Color) ---
    vec3 albedo = u_hasDiffuseMap ? texture(u_diffuseMap, v_texCoord).rgb : vec3(1.0);

    // --- 3. Get Shininess (from Roughness) ---
    // Lower roughness = higher shininess
    float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, v_texCoord).r : 0.5;
    float shininess = mix(512.0, 4.0, clamp(roughness, 0.0, 1.0));
    // Use material shininess if no roughness map
    if (!u_hasRoughnessMap) {
        shininess = u_material_shininess;
    }

    vec3 viewDir = normalize(u_viewPos.xyz - v_worldPos);
    vec3 totalLight = vec3(0.0);

    // ======================================================
    // --- 4. Calculate Lighting Loop ---
    for (int i = 0; i < sceneLights.active_light_count; ++i) {
        LightData light = sceneLights.lights[i];
        if (light.type == LIGHT_TYPE_INACTIVE) continue;

        vec3 lightDir;
        float attenuation = 1.0;

        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            lightDir = normalize(-light.direction.xyz);
        }
        else { // Point or Spot
            vec3 toLight = light.position.xyz - v_worldPos;
            float dist = length(toLight);
            lightDir = normalize(toLight);
            attenuation = 1.0 / (light.attenuation.x +
                                 light.attenuation.y * dist +
                                 light.attenuation.z * dist * dist);
            
            if (light.type == LIGHT_TYPE_SPOT) {
                float theta = dot(lightDir, normalize(-light.direction.xyz));
                float cutoff = cos(radians(light.attenuation.w));
                float epsilon = 0.05; // Smooth edge
                float spotFactor = clamp((theta - cutoff) / epsilon, 0.0, 1.0);
                attenuation *= spotFactor;
            }
        }

        // Blinn-Phong lighting
        vec3 halfway = normalize(lightDir + viewDir);
        float diff = max(dot(norm, lightDir), 0.0);
        float spec = (diff > 0.0) ? pow(max(dot(norm, halfway), 0.0), shininess) : 0.0;

        vec3 ambient  = light.ambient.xyz  * (u_material_ambient * albedo);
        vec3 diffuse  = light.diffuse.xyz  * diff * (u_material_diffuse * albedo);
        vec3 specular = light.specular.xyz * spec * u_material_specular;

        totalLight += (ambient + diffuse + specular) * attenuation;
    }
    
    vec3 baseColor = totalLight;

    // ======================================================
    // --- 5. Apply Extras ---

    // --- Projective Texture ---
    if (u_enableProjectiveTex) {
        // Convert from clip space to texture space [0, 1]
        vec3 projCoord = v_projTexCoord.xyz / v_projTexCoord.w;
        projCoord = projCoord * 0.5 + 0.5;

        // Apply texture only if it's in front of the projector
        if (projCoord.z < 1.0) {
            float shadow = texture(u_projectiveMap, projCoord.xy).r; // Assuming a grayscale "slide"
            baseColor *= shadow; // Modulate the light
        }
    }

    // --- Environment Reflection ---
    if (u_enableReflection) {
        vec3 reflectDir = reflect(-viewDir, norm);
        // Correct for EnGene's coordinate system if necessary (from skybox_fragment.glsl)
        reflectDir.z *= -1.0; 
        vec3 reflectColor = texture(u_skybox, reflectDir).rgb;
        
        // Use a fixed reflection factor for simplicity
        float factor = u_reflectionFactor > 0.0 ? u_reflectionFactor : 0.4;
        baseColor = mix(baseColor, reflectColor, factor);
    }

    // --- 6. Apply Fog (at the very end) ---
    // If fogdensity is 0, fogFactor becomes 1.0
    float distance = length(u_viewPos.xyz - v_worldPos);
    float fogFactor = exp(-pow(fogdensity * distance, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(fogcolor, baseColor, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}