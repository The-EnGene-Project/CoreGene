#version 410

// Maximum number of lights - must match C++ light_config.h
#define MAX_SCENE_LIGHTS 16

// Light type enumeration - must match C++ LightType enum
#define LIGHT_TYPE_INACTIVE 0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_SPOT 3

// GPU-compatible light data structure matching C++ LightData
struct LightData {
    vec4 position;      // World-space position (w=1), unused for directional
    vec4 direction;     // World-space direction (w=0), unused for point
    vec4 ambient;       // Ambient color (RGB + padding)
    vec4 diffuse;       // Diffuse color (RGB + padding)
    vec4 specular;      // Specular color (RGB + padding)
    vec4 attenuation;   // (constant, linear, quadratic, cutoff_angle)
    int type;           // LightType enum value
    int padding[3];     // Align to 16-byte boundary
};

// Scene lights uniform block with std140 layout
layout(std140, binding = 0) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
    int padding[3];
} sceneLights;

// Material properties
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;
uniform float materialShininess;

// Camera position for specular calculations
uniform vec3 viewPos;

// Input from vertex shader
in vec3 fragPos;
in vec3 fragNormal;
in vec4 fragColor;

// Output color
out vec4 outColor;

// Calculate directional light contribution
vec3 calculateDirectionalLight(LightData light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction.xyz);
    
    // Ambient
    vec3 ambient = light.ambient.rgb * materialAmbient.rgb;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse.rgb * (diff * materialDiffuse.rgb);
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    vec3 specular = light.specular.rgb * (spec * materialSpecular.rgb);
    
    return ambient + diffuse + specular;
}

// Calculate point light contribution
vec3 calculatePointLight(LightData light, vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float distance = length(light.position.xyz - fragPos);
    
    // Attenuation
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);
    
    // Ambient
    vec3 ambient = light.ambient.rgb * materialAmbient.rgb;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse.rgb * (diff * materialDiffuse.rgb);
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    vec3 specular = light.specular.rgb * (spec * materialSpecular.rgb);
    
    // Apply attenuation
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}

// Calculate spot light contribution
vec3 calculateSpotLight(LightData light, vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float distance = length(light.position.xyz - fragPos);
    
    // Spotlight intensity (smooth edges)
    float theta = dot(lightDir, normalize(-light.direction.xyz));
    float cutoff = light.attenuation.w;
    float epsilon = cutoff * 0.1; // Smooth transition zone
    float intensity = clamp((theta - cutoff + epsilon) / epsilon, 0.0, 1.0);
    
    // If outside spotlight cone, return no contribution
    if (theta < cutoff) {
        return vec3(0.0);
    }
    
    // Attenuation
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);
    
    // Ambient
    vec3 ambient = light.ambient.rgb * materialAmbient.rgb;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse.rgb * (diff * materialDiffuse.rgb);
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    vec3 specular = light.specular.rgb * (spec * materialSpecular.rgb);
    
    // Apply attenuation and spotlight intensity
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return ambient + diffuse + specular;
}

// Main lighting calculation function
vec3 calculateLighting(vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 result = vec3(0.0);
    
    // Loop through all active lights
    for (int i = 0; i < sceneLights.active_light_count; i++) {
        LightData light = sceneLights.lights[i];
        
        // Branch based on light type
        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            result += calculateDirectionalLight(light, normal, viewDir);
        }
        else if (light.type == LIGHT_TYPE_POINT) {
            result += calculatePointLight(light, fragPos, normal, viewDir);
        }
        else if (light.type == LIGHT_TYPE_SPOT) {
            result += calculateSpotLight(light, fragPos, normal, viewDir);
        }
        // LIGHT_TYPE_INACTIVE (0) is ignored
    }
    
    return result;
}

void main() {
    // Normalize interpolated normal
    vec3 normal = normalize(fragNormal);
    
    // Calculate view direction
    vec3 viewDir = normalize(viewPos - fragPos);
    
    // Calculate lighting
    vec3 lighting = calculateLighting(fragPos, normal, viewDir);
    
    // Combine with base color
    outColor = vec4(lighting * fragColor.rgb, fragColor.a);
}
