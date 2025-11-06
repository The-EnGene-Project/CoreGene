#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;
in vec3 v_tangent;
in vec2 v_texCoord;

// === Definições da cena ===
#define MAX_SCENE_LIGHTS 16
#define LIGHT_TYPE_INACTIVE    0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT       2
#define LIGHT_TYPE_SPOT        3

struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

// === Uniforms ===
uniform Light u_lights[MAX_SCENE_LIGHTS];
uniform int u_lightCount;

uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_roughnessMap;
uniform bool u_hasNormalMap;
uniform bool u_hasRoughnessMap;

uniform vec3 u_viewPos;

// === Fog uniforms ===
uniform vec3 fcolor;     // cor da fog
uniform float fdensity;  // densidade da fog

// === Funções auxiliares ===
mat3 computeTBN(vec3 normal, vec3 tangent)
{
    tangent = normalize(tangent - normal * dot(normal, tangent));
    vec3 bitangent = normalize(cross(normal, tangent));
    return mat3(tangent, bitangent, normal);
}

// === Cálculo de iluminação completa ===
vec3 applyLighting(vec3 norm, vec3 viewDir, vec3 fragPos, vec3 albedo, float roughness)
{
    vec3 result = vec3(0.0);

    for (int i = 0; i < u_lightCount; i++)
    {
        Light light = u_lights[i];
        if (light.type == LIGHT_TYPE_INACTIVE)
            continue;

        vec3 lightDir;
        float attenuation = 1.0;
        float intensity = 1.0;

        // ---------- Direcional ----------
        if (light.type == LIGHT_TYPE_DIRECTIONAL)
        {
            lightDir = normalize(-light.direction);
        }

        // ---------- Pontual ----------
        else if (light.type == LIGHT_TYPE_POINT)
        {
            lightDir = normalize(light.position - fragPos);
            float distance = length(light.position - fragPos);
            attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
        }

        // ---------- Spot ----------
        else if (light.type == LIGHT_TYPE_SPOT)
        {
            vec3 lightToFrag = normalize(fragPos - light.position);
            float theta = dot(lightToFrag, normalize(-light.direction));
            float epsilon = light.cutOff - light.outerCutOff;
            intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

            lightDir = normalize(light.position - fragPos);
            float distance = length(light.position - fragPos);
            attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
        }

        // ---------- Iluminação ----------
        float diff = max(dot(norm, lightDir), 0.0);

        // Reflexão especular Phong
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0),
                         mix(8.0, 64.0, 1.0 - roughness));

        vec3 ambient = light.ambient * albedo;
        vec3 diffuse = light.diffuse * diff * albedo;
        vec3 specular = light.specular * spec * vec3(1.0);

        vec3 lightResult = ambient + diffuse + specular;
        lightResult *= attenuation * intensity;

        result += lightResult;
    }

    return result;
}

void main()
{
    vec3 normal = normalize(v_normal);
    mat3 TBN = computeTBN(normal, normalize(v_tangent));

    if (u_hasNormalMap)
    {
        vec3 mapNormal = texture(u_normalMap, v_texCoord).rgb;
        mapNormal = normalize(mapNormal * 2.0 - 1.0);
        normal = normalize(TBN * mapNormal);
    }

    vec3 albedo = texture(u_diffuseMap, v_texCoord).rgb;
    float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, v_texCoord).r : 0.5;

    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec3 lighting = applyLighting(normal, viewDir, v_fragPos, albedo, roughness);

    // === Fog ===
    float distance = length(u_viewPos - v_fragPos);
    float fogFactor = exp(-pow(fdensity * distance, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(fcolor, lighting, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
