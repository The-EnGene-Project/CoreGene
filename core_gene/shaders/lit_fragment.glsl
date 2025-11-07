#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;

// Maximum number of lights - must match C++ light_config.h
#define MAX_SCENE_LIGHTS 16

// Light type enumeration - must match C++ LightType enum
#define LIGHT_TYPE_INACTIVE 0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_SPOT 3

struct LightData {
    vec4 position;      // World-space position (w=1), unused for directional
    vec4 direction;     // World-space direction (w=0), unused for point
    vec4 ambient;       // Ambient color (RGB + padding)
    vec4 diffuse;       // Diffuse color (RGB + padding)
    vec4 specular;      // Specular color (RGB + padding)
    vec4 attenuation;   // (constant, linear, quadratic, cutoff_angle)
    int type;           // LightType enum value
    int padding1;       // 4 bytes (manual padding)
    int padding2;       // 4 bytes (manual padding)
    int padding3;       // 4 bytes (manual padding)
};

// Scene lights uniform block with std140 layout
layout(std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
} sceneLights;

layout (std140) uniform CameraPosition {
    vec4 u_viewPos;
};

// Estrutura para o material
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
Material u_material;

void main()
{
    // Definir valores padrão para o material aqui
    u_material.ambient = vec3(0.15, 0.0, 0.25);
    u_material.diffuse = vec3(0.3, 0.0, 0.5);
    u_material.specular = vec3(1.0, 1.0, 1.0);
    u_material.shininess = 32.0;
    // É mais seguro obter a luz dentro do main()
    LightData u_dirLight = sceneLights.lights[0];

    // Normal do fragmento
    vec3 norm = normalize(v_normal);
    
    // ======== CÁLCULO DA DIREÇÃO (O FIX) ========
    // O vetor de direção é calculado a partir da POSIÇÃO da luz
    // e da POSIÇÃO do fragmento.
    // (Vetor do fragmento PARA a luz)
    vec3 toLightVector = u_dirLight.position.xyz - v_fragPos;
    
    // Esta é a direção normalizada que você usará para os cálculos
    vec3 lightDir = normalize(toLightVector);

    // ======== COMPONENTE DIFUSA ========
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = u_dirLight.diffuse.xyz * (diff * u_material.diffuse); // <-- FIX

    // ======== COMPONENTE ESPECULAR ========
    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos); // <-- FIX
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_material.shininess);
    vec3 specular = u_dirLight.specular.xyz * (spec * u_material.specular); // <-- FIX

    // ======== COMPONENTE AMBIENTE ========
    vec3 ambient = u_dirLight.diffuse.xyz * u_material.ambient; // <-- FIX

    // ======== ATENUAÇÃO (CRÍTICO PARA LUZ DE PONTO) ========
    // Sem isso, a luz terá a mesma intensidade em toda a cena.
    // Usamos o toLightVector *antes* de normalizar para obter a distância
    float dist = length(toLightVector);
    float attenuation = 1.0 / (u_dirLight.attenuation.x + 
                                 u_dirLight.attenuation.y * dist + 
                                 u_dirLight.attenuation.z * dist * dist);

    // ======== RESULTADO FINAL ========
    vec3 result = ambient + (diffuse + specular) * attenuation;

    FragColor = vec4(result, 1.0);
}