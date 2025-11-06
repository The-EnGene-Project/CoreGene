#ifndef ENVIRONMENT_MAPPING_H
#define ENVIRONMENT_MAPPING_H

#include <memory>
#include <functional>
#include <iostream>
#include <glm/glm.hpp>
#include "../core_gene/src/gl_base/cubemap.h"
#include "../core_gene/src/gl_base/shader.h"

namespace environment {

/**
 * @enum MappingMode
 * @brief Defines the type of environment mapping effect to apply.
 */
enum class MappingMode {
    REFLECTION,              ///< Pure reflection effect
    REFRACTION,              ///< Pure refraction effect
    FRESNEL,                 ///< Angle-dependent reflection/refraction blend
    CHROMATIC_DISPERSION     ///< Color separation during refraction (prism effect)
};

/**
 * @struct EnvironmentMappingConfig
 * @brief Configuration parameters for environment mapping effects.
 * 
 * This structure contains all parameters needed to configure the various
 * environment mapping modes (reflection, refraction, Fresnel, chromatic dispersion).
 */
struct EnvironmentMappingConfig {
    texture::CubemapPtr cubemap;                    ///< The environment cubemap texture
    MappingMode mode = MappingMode::REFLECTION;     ///< Current effect mode
    
    // Effect parameters
    float reflection_coefficient = 0.8f;            ///< Blend factor for reflection (0.0 to 1.0)
    float index_of_refraction = 1.5f;               ///< Index of refraction for materials
    glm::vec3 ior_rgb = glm::vec3(1.51f, 1.52f, 1.53f);  ///< Per-channel IOR for chromatic dispersion
    float fresnel_power = 3.0f;                     ///< Fresnel effect intensity
    glm::vec3 base_color = glm::vec3(1.0f);         ///< Base material color
};

// ============================================================================
// Shader Source Code (Raw Strings)
// ============================================================================

/**
 * @brief Common vertex shader for all environment mapping modes.
 * 
 * Transforms vertices and passes world-space position and normal to fragment shader.
 * Uses Camera UBO for view and projection matrices.
 */
static const char* ENV_MAPPING_VERTEX_SHADER = R"(
#version 430 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_worldPos;
out vec3 v_worldNormal;

// Camera Matrices UBO (already established in EnGene)
layout(std140, binding = 0) uniform CameraMatrices {
    mat4 u_view;
    mat4 u_projection;
};

uniform mat4 u_model;

void main() {
    v_worldPos = vec3(u_model * vec4(a_position, 1.0));
    v_worldNormal = mat3(transpose(inverse(u_model))) * a_normal;
    
    gl_Position = u_projection * u_view * vec4(v_worldPos, 1.0);
}
)";

/**
 * @brief Fragment shader for reflection mode.
 * 
 * Calculates reflection vector and samples the environment cubemap.
 * Blends reflected color with base material color using reflection coefficient.
 */
static const char* REFLECTION_FRAGMENT_SHADER = R"(
#version 430 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
out vec4 FragColor;

// Camera Position UBO (already established in EnGene)
layout(std140, binding = 1) uniform CameraPosition {
    vec3 u_cameraPos;
};

uniform samplerCube u_environmentMap;
uniform float u_reflectionCoefficient;
uniform vec3 u_baseColor;

void main() {
    vec3 N = normalize(v_worldNormal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    
    // Calculate reflection vector
    vec3 R = reflect(-V, N);
    
    // Sample environment map
    vec3 reflectedColor = texture(u_environmentMap, R).rgb;
    
    // Blend with base color
    vec3 finalColor = mix(u_baseColor, reflectedColor, u_reflectionCoefficient);
    
    FragColor = vec4(finalColor, 1.0);
}
)";

/**
 * @brief Fragment shader for refraction mode.
 * 
 * Calculates refraction vector using index of refraction.
 * Handles total internal reflection cases.
 */
static const char* REFRACTION_FRAGMENT_SHADER = R"(
#version 430 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
out vec4 FragColor;

// Camera Position UBO (already established in EnGene)
layout(std140, binding = 1) uniform CameraPosition {
    vec3 u_cameraPos;
};

uniform samplerCube u_environmentMap;
uniform float u_indexOfRefraction;
uniform vec3 u_baseColor;

void main() {
    vec3 N = normalize(v_worldNormal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    
    // Calculate refraction vector
    vec3 R = refract(-V, N, 1.0 / u_indexOfRefraction);
    
    // Handle total internal reflection
    if (length(R) < 0.001) {
        R = reflect(-V, N);
    }
    
    // Sample environment map
    vec3 refractedColor = texture(u_environmentMap, R).rgb;
    
    // Blend with base color
    vec3 finalColor = mix(u_baseColor, refractedColor, 0.9);
    
    FragColor = vec4(finalColor, 1.0);
}
)";

/**
 * @brief Fragment shader for Fresnel mode.
 * 
 * Calculates both reflection and refraction, blending based on viewing angle.
 * Produces physically-based angle-dependent reflection intensity.
 */
static const char* FRESNEL_FRAGMENT_SHADER = R"(
#version 430 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
out vec4 FragColor;

// Camera Position UBO (already established in EnGene)
layout(std140, binding = 1) uniform CameraPosition {
    vec3 u_cameraPos;
};

uniform samplerCube u_environmentMap;
uniform float u_fresnelPower;
uniform float u_indexOfRefraction;
uniform vec3 u_baseColor;

void main() {
    vec3 N = normalize(v_worldNormal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    
    // Calculate Fresnel coefficient
    float fresnel = pow(1.0 - max(dot(V, N), 0.0), u_fresnelPower);
    
    // Calculate reflection and refraction
    vec3 R_reflect = reflect(-V, N);
    vec3 R_refract = refract(-V, N, 1.0 / u_indexOfRefraction);
    
    // Handle total internal reflection
    if (length(R_refract) < 0.001) {
        R_refract = R_reflect;
    }
    
    // Sample environment map
    vec3 reflectedColor = texture(u_environmentMap, R_reflect).rgb;
    vec3 refractedColor = texture(u_environmentMap, R_refract).rgb;
    
    // Blend based on Fresnel
    vec3 envColor = mix(refractedColor, reflectedColor, fresnel);
    vec3 finalColor = mix(u_baseColor, envColor, 0.9);
    
    FragColor = vec4(finalColor, 1.0);
}
)";

/**
 * @brief Fragment shader for chromatic dispersion mode.
 * 
 * Calculates separate refraction vectors for R, G, B channels.
 * Creates rainbow prism effect by separating colors during refraction.
 */
/**
 * @brief Fragment shader for chromatic dispersion mode.
 * 
 * Calculates separate refraction vectors for R, G, B channels.
 * Creates rainbow prism effect by separating colors during refraction.
 */
static const char* CHROMATIC_DISPERSION_FRAGMENT_SHADER = R"(
#version 430 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
out vec4 FragColor;

// Camera Position UBO (already established in EnGene)
layout(std140, binding = 1) uniform CameraPosition {
    vec3 u_cameraPos;
};

uniform samplerCube u_environmentMap;
uniform vec3 u_iorRGB;  // (ior_red, ior_green, ior_blue)
uniform vec3 u_baseColor;

void main() {
    vec3 N = normalize(v_worldNormal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    
    // Calculate three refraction vectors (one per color channel)
    vec3 R_red = refract(-V, N, 1.0 / u_iorRGB.r);
    vec3 R_green = refract(-V, N, 1.0 / u_iorRGB.g);
    vec3 R_blue = refract(-V, N, 1.0 / u_iorRGB.b);
    
    // Handle total internal reflection for each channel
    if (length(R_red) < 0.001) R_red = reflect(-V, N);
    if (length(R_green) < 0.001) R_green = reflect(-V, N);
    if (length(R_blue) < 0.001) R_blue = reflect(-V, N);
    
    // Sample environment map three times
    float red = texture(u_environmentMap, R_red).r;
    float green = texture(u_environmentMap, R_green).g;
    float blue = texture(u_environmentMap, R_blue).b;
    
    // Reconstruct color from separated channels
    vec3 dispersedColor = vec3(red, green, blue);
    vec3 finalColor = mix(u_baseColor, dispersedColor, 0.9);
    
    FragColor = vec4(finalColor, 1.0);
}
)";

// ============================================================================
// EnvironmentMapping Class
// ============================================================================

/**
 * @class EnvironmentMapping
 * @brief Utility class for applying environment mapping effects to objects.
 * 
 * This class manages shaders and configuration for reflection, refraction,
 * Fresnel, and chromatic dispersion effects. It provides a simple interface
 * for switching between effect modes and configuring parameters.
 * 
 * @note This is a utility class, not a component. It provides configured
 *       shaders that can be used with ShaderComponent or directly.
 */
class EnvironmentMapping {
private:
    EnvironmentMappingConfig m_config;
    
    // Pre-built shaders for each mode
    shader::ShaderPtr m_reflection_shader;
    shader::ShaderPtr m_refraction_shader;
    shader::ShaderPtr m_fresnel_shader;
    shader::ShaderPtr m_chromatic_shader;
    
    /**
     * @brief Initialize all shader variants with proper uniform configuration.
     * 
     * Creates shaders from raw string sources and configures Camera UBOs
     * and dynamic uniforms for each mode.
     */
    void initializeShaders();
    
    /**
     * @brief Get the shader for the current mode.
     * @return Shader pointer for the active effect mode.
     */
    shader::ShaderPtr getCurrentShader() const;
    
public:
    /**
     * @brief Construct environment mapping system with configuration.
     * @param config Configuration parameters for effects.
     */
    EnvironmentMapping(const EnvironmentMappingConfig& config);
    
    /**
     * @brief Get configured shader for current mode.
     * @return Shader pointer ready for use with ShaderComponent.
     */
    shader::ShaderPtr getShader() const { return getCurrentShader(); }
    
    // Uniform provider methods (for Tier 3 dynamic uniforms)
    std::function<float()> getReflectionCoefficientProvider() const;
    std::function<float()> getIndexOfRefractionProvider() const;
    std::function<glm::vec3()> getIorRGBProvider() const;
    std::function<float()> getFresnelPowerProvider() const;
    std::function<glm::vec3()> getBaseColorProvider() const;
    
    // Configuration setter methods
    void setMode(MappingMode mode);
    void setReflectionCoefficient(float coeff);
    void setIndexOfRefraction(float ior);
    void setIndexOfRefractionRGB(const glm::vec3& ior_rgb);
    void setFresnelPower(float power);
    void setBaseColor(const glm::vec3& color);
    void setCubemap(texture::CubemapPtr cubemap);
    
    /**
     * @brief Get current configuration.
     * @return Reference to configuration structure.
     */
    const EnvironmentMappingConfig& getConfig() const { return m_config; }
};

using EnvironmentMappingPtr = std::shared_ptr<EnvironmentMapping>;

// ============================================================================
// EnvironmentMapping Implementation
// ============================================================================

inline EnvironmentMapping::EnvironmentMapping(const EnvironmentMappingConfig& config)
    : m_config(config) {
    initializeShaders();
}

inline void EnvironmentMapping::initializeShaders() {
    // Create reflection shader
    m_reflection_shader = shader::Shader::Make(
        ENV_MAPPING_VERTEX_SHADER,
        REFLECTION_FRAGMENT_SHADER
    );
    
    // Bind Camera UBOs
    m_reflection_shader->addResourceBlockToBind("CameraMatrices");
    m_reflection_shader->addResourceBlockToBind("CameraPosition");
    
    // Configure dynamic uniforms for model matrix
    m_reflection_shader->configureDynamicUniform<glm::mat4>("u_model", 
        []() { return transform::current(); });
    
    // Configure dynamic uniforms for effect parameters
    m_reflection_shader->configureDynamicUniform<float>("u_reflectionCoefficient",
        [this]() { return m_config.reflection_coefficient; });
    m_reflection_shader->configureDynamicUniform<glm::vec3>("u_baseColor",
        [this]() { return m_config.base_color; });
    
    // Configure cubemap texture uniform using sampler provider
    m_reflection_shader->configureDynamicUniform<uniform::detail::Sampler>("u_environmentMap",
        texture::getSamplerProvider("environmentMap"));
    
    // Bake the shader to link and bind UBOs
    m_reflection_shader->Bake();
    
    // Create refraction shader
    m_refraction_shader = shader::Shader::Make(
        ENV_MAPPING_VERTEX_SHADER,
        REFRACTION_FRAGMENT_SHADER
    );
    
    // Bind Camera UBOs
    m_refraction_shader->addResourceBlockToBind("CameraMatrices");
    m_refraction_shader->addResourceBlockToBind("CameraPosition");
    
    // Configure dynamic uniforms
    m_refraction_shader->configureDynamicUniform<glm::mat4>("u_model",
        []() { return transform::current(); });
    m_refraction_shader->configureDynamicUniform<float>("u_indexOfRefraction",
        [this]() { return m_config.index_of_refraction; });
    m_refraction_shader->configureDynamicUniform<glm::vec3>("u_baseColor",
        [this]() { return m_config.base_color; });
    
    // Configure cubemap texture uniform using sampler provider
    m_refraction_shader->configureDynamicUniform<uniform::detail::Sampler>("u_environmentMap",
        texture::getSamplerProvider("environmentMap"));
    
    // Bake the shader
    m_refraction_shader->Bake();
    
    // Create Fresnel shader
    m_fresnel_shader = shader::Shader::Make(
        ENV_MAPPING_VERTEX_SHADER,
        FRESNEL_FRAGMENT_SHADER
    );
    
    // Bind Camera UBOs
    m_fresnel_shader->addResourceBlockToBind("CameraMatrices");
    m_fresnel_shader->addResourceBlockToBind("CameraPosition");
    
    // Configure dynamic uniforms
    m_fresnel_shader->configureDynamicUniform<glm::mat4>("u_model",
        []() { return transform::current(); });
    m_fresnel_shader->configureDynamicUniform<float>("u_fresnelPower",
        [this]() { return m_config.fresnel_power; });
    m_fresnel_shader->configureDynamicUniform<float>("u_indexOfRefraction",
        [this]() { return m_config.index_of_refraction; });
    m_fresnel_shader->configureDynamicUniform<glm::vec3>("u_baseColor",
        [this]() { return m_config.base_color; });
    
    // Configure cubemap texture uniform using sampler provider
    m_fresnel_shader->configureDynamicUniform<uniform::detail::Sampler>("u_environmentMap",
        texture::getSamplerProvider("environmentMap"));
    
    // Bake the shader
    m_fresnel_shader->Bake();
    
    // Create chromatic dispersion shader
    m_chromatic_shader = shader::Shader::Make(
        ENV_MAPPING_VERTEX_SHADER,
        CHROMATIC_DISPERSION_FRAGMENT_SHADER
    );
    
    // Bind Camera UBOs
    m_chromatic_shader->addResourceBlockToBind("CameraMatrices");
    m_chromatic_shader->addResourceBlockToBind("CameraPosition");
    
    // Configure dynamic uniforms
    m_chromatic_shader->configureDynamicUniform<glm::mat4>("u_model",
        []() { return transform::current(); });
    m_chromatic_shader->configureDynamicUniform<glm::vec3>("u_iorRGB",
        [this]() { return m_config.ior_rgb; });
    m_chromatic_shader->configureDynamicUniform<glm::vec3>("u_baseColor",
        [this]() { return m_config.base_color; });
    
    // Configure cubemap texture uniform using sampler provider
    m_chromatic_shader->configureDynamicUniform<uniform::detail::Sampler>("u_environmentMap",
        texture::getSamplerProvider("environmentMap"));
    
    // Bake the shader
    m_chromatic_shader->Bake();
}

inline shader::ShaderPtr EnvironmentMapping::getCurrentShader() const {
    switch (m_config.mode) {
        case MappingMode::REFLECTION:
            return m_reflection_shader;
        case MappingMode::REFRACTION:
            return m_refraction_shader;
        case MappingMode::FRESNEL:
            return m_fresnel_shader;
        case MappingMode::CHROMATIC_DISPERSION:
            return m_chromatic_shader;
        default:
            return m_reflection_shader;
    }
}

inline std::function<float()> EnvironmentMapping::getReflectionCoefficientProvider() const {
    return [this]() { return m_config.reflection_coefficient; };
}

inline std::function<float()> EnvironmentMapping::getIndexOfRefractionProvider() const {
    return [this]() { return m_config.index_of_refraction; };
}

inline std::function<glm::vec3()> EnvironmentMapping::getIorRGBProvider() const {
    return [this]() { return m_config.ior_rgb; };
}

inline std::function<float()> EnvironmentMapping::getFresnelPowerProvider() const {
    return [this]() { return m_config.fresnel_power; };
}

inline std::function<glm::vec3()> EnvironmentMapping::getBaseColorProvider() const {
    return [this]() { return m_config.base_color; };
}

inline void EnvironmentMapping::setMode(MappingMode mode) {
    m_config.mode = mode;
}

inline void EnvironmentMapping::setReflectionCoefficient(float coeff) {
    if (coeff < 0.0f || coeff > 1.0f) {
        std::cerr << "Warning: Reflection coefficient should be between 0.0 and 1.0. "
                  << "Clamping value " << coeff << std::endl;
        coeff = glm::clamp(coeff, 0.0f, 1.0f);
    }
    m_config.reflection_coefficient = coeff;
}

inline void EnvironmentMapping::setIndexOfRefraction(float ior) {
    if (ior <= 0.0f) {
        std::cerr << "Warning: Invalid index of refraction " << ior 
                  << ". Using default value 1.5" << std::endl;
        ior = 1.5f;
    }
    m_config.index_of_refraction = ior;
}

inline void EnvironmentMapping::setIndexOfRefractionRGB(const glm::vec3& ior_rgb) {
    if (ior_rgb.r <= 0.0f || ior_rgb.g <= 0.0f || ior_rgb.b <= 0.0f) {
        std::cerr << "Warning: Invalid IOR RGB values (" 
                  << ior_rgb.r << ", " << ior_rgb.g << ", " << ior_rgb.b 
                  << "). Using default values (1.51, 1.52, 1.53)" << std::endl;
        m_config.ior_rgb = glm::vec3(1.51f, 1.52f, 1.53f);
    } else {
        m_config.ior_rgb = ior_rgb;
    }
}

inline void EnvironmentMapping::setFresnelPower(float power) {
    if (power < 0.0f) {
        std::cerr << "Warning: Fresnel power should be positive. "
                  << "Using absolute value of " << power << std::endl;
        power = std::abs(power);
    }
    m_config.fresnel_power = power;
}

inline void EnvironmentMapping::setBaseColor(const glm::vec3& color) {
    m_config.base_color = color;
}

inline void EnvironmentMapping::setCubemap(texture::CubemapPtr cubemap) {
    m_config.cubemap = cubemap;
}

} // namespace environment

#endif // ENVIRONMENT_MAPPING_H
