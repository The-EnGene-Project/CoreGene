#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;
in vec2 v_texCoords;

struct LightData {
    vec4 position;
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation;
    int type;
    int padding1;
    int padding2;
    int padding3;
};

layout(std140) uniform SceneLights {
    LightData lights[16];
    int active_light_count;
} sceneLights;

layout (std140) uniform CameraPosition {
    vec4 u_viewPos;
};

uniform sampler2D u_specularMap;
uniform sampler2D u_glossMap;

void main()
{
    vec3 norm = normalize(v_normal);
    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos);

    vec3 result = vec3(0.0);

    for (int i = 0; i < sceneLights.active_light_count; i++)
    {
        vec3 lightDir;
        float attenuation = 1.0;

        if (sceneLights.lights[i].type == 1) // Directional light
        {
            lightDir = normalize(-sceneLights.lights[i].direction.xyz);
        }
        else // Point light
        {
            lightDir = normalize(sceneLights.lights[i].position.xyz - v_fragPos);
            float dist = length(sceneLights.lights[i].position.xyz - v_fragPos);
            attenuation = 1.0 / (sceneLights.lights[i].attenuation.x + sceneLights.lights[i].attenuation.y * dist + sceneLights.lights[i].attenuation.z * dist * dist);
        }

        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = sceneLights.lights[i].diffuse.xyz * diff;

        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float gloss = texture(u_glossMap, v_texCoords).r;
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0 * gloss);
        vec3 specularColor = texture(u_specularMap, v_texCoords).rgb;
        vec3 specular = sceneLights.lights[i].specular.xyz * spec * specularColor;

        result += (diffuse + specular) * attenuation;
    }

    FragColor = vec4(result, 1.0);
}
