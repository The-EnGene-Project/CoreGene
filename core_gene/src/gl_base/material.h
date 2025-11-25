#ifndef MATERIAL_H
#define MATERIAL_H
#pragma once

#include <variant>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <iostream>

#include "gl_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declaration for shader integration
namespace shader {
    class Shader;
}

namespace material {

/**
 * @brief Type-safe variant for all non-sampler shader uniform types.
 *
 * Supports: float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4.
 * Excludes texture samplers (handled by TextureComponent).
 */
using PropertyVariant = std::variant<
    float,
    int,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat3,
    glm::mat4
>;

/**
 * @brief Raw data mapping between property names and values sent to shaders.
 *
 * Represents the semantic meaning of material property storage.
 */
using MaterialData = std::unordered_map<std::string, PropertyVariant>;

// Forward declaration for MaterialPtr type alias
class Material;

/**
 * @brief Type alias for shared pointer to Material.
 *
 * Provides convenient type name for material references throughout the system.
 */
using MaterialPtr = std::shared_ptr<Material>;

/**
 * @class Material
 * @brief Data container for material property recipes.
 *
 * Provides type-safe property storage and factory methods for common materials.
 * Stores material properties as name-value pairs using PropertyVariant for type safety.
 * Inherits from std::enable_shared_from_this to support method chaining with shared_ptr.
 */
class Material : public std::enable_shared_from_this<Material> {
private:
    /// @brief Storage for material properties as name-value pairs.
    MaterialData m_properties;

protected:
    /**
     * @brief Default constructor - creates a standard 'matte white' material.
     *
     * Initializes with standard PBR properties:
     * - Ambient: (0.2, 0.2, 0.2)
     * - Diffuse: (1.0, 1.0, 1.0)
     * - Specular: (0.5, 0.5, 0.5)
     * - Shininess: 32.0
     */
    Material() {
        m_properties[s_ambient_name] = glm::vec3(0.2f, 0.2f, 0.2f);
        m_properties[s_diffuse_name] = glm::vec3(1.0f, 1.0f, 1.0f);
        m_properties[s_specular_name] = glm::vec3(0.5f, 0.5f, 0.5f);
        m_properties[s_shininess_name] = 32.0f;
    }

    /**
     * @brief Protected factory method to create an empty material.
     * @return A MaterialPtr to a newly created empty Material.
     *
     * Creates a material with no properties. Used internally when an empty material is needed.
     */
    static MaterialPtr MakeEmpty() {
        auto mat = MaterialPtr(new Material());
        mat->m_properties.clear();
        return mat;
    }

public:
    /// @brief Static default uniform name for ambient material property.
    inline static std::string s_ambient_name = "u_material_ambient";
    
    /// @brief Static default uniform name for diffuse material property.
    inline static std::string s_diffuse_name = "u_material_diffuse";
    
    /// @brief Static default uniform name for specular material property.
    inline static std::string s_specular_name = "u_material_specular";
    
    /// @brief Static default uniform name for shininess material property.
    inline static std::string s_shininess_name = "u_material_shininess";

    /**
     * @brief Sets the global default uniform name for ambient property.
     * @param name The new default uniform name for ambient.
     */
    static void SetDefaultAmbientName(const std::string& name) {
        s_ambient_name = name;
    }

    /**
     * @brief Sets the global default uniform name for diffuse property.
     * @param name The new default uniform name for diffuse.
     */
    static void SetDefaultDiffuseName(const std::string& name) {
        s_diffuse_name = name;
    }

    /**
     * @brief Sets the global default uniform name for specular property.
     * @param name The new default uniform name for specular.
     */
    static void SetDefaultSpecularName(const std::string& name) {
        s_specular_name = name;
    }

    /**
     * @brief Sets the global default uniform name for shininess property.
     * @param name The new default uniform name for shininess.
     */
    static void SetDefaultShininessName(const std::string& name) {
        s_shininess_name = name;
    }

    /**
     * @brief Sets a material property with type-safe assignment.
     * @tparam T The type of the property value (must be a PropertyVariant type).
     * @param name The name of the property (uniform name).
     * @param value The value to assign to the property.
     * @return MaterialPtr to this material for method chaining.
     */
    template<typename T>
    MaterialPtr set(const std::string& name, const T& value) {
        m_properties[name] = value;
        return shared_from_this();
    }

    /**
     * @brief Sets the ambient color using the default uniform name.
     * @param color The ambient color as a vec3.
     * @return MaterialPtr to this material for method chaining.
     */
    MaterialPtr setAmbient(const glm::vec3& color) {
        set(s_ambient_name, color);
        return shared_from_this();
    }

    /**
     * @brief Sets the diffuse color using the default uniform name.
     * @param color The diffuse color as a vec3.
     * @return MaterialPtr to this material for method chaining.
     */
    MaterialPtr setDiffuse(const glm::vec3& color) {
        set(s_diffuse_name, color);
        return shared_from_this();
    }

    /**
     * @brief Sets the specular color using the default uniform name.
     * @param color The specular color as a vec3.
     * @return MaterialPtr to this material for method chaining.
     */
    MaterialPtr setSpecular(const glm::vec3& color) {
        set(s_specular_name, color);
        return shared_from_this();
    }

    /**
     * @brief Sets the shininess value using the default uniform name.
     * @param value The shininess value as a float.
     * @return MaterialPtr to this material for method chaining.
     */
    MaterialPtr setShininess(float value) {
        set(s_shininess_name, value);
        return shared_from_this();
    }

    /**
     * @brief Remaps a property's uniform name at the instance level.
     * @param oldName The current property name to remap.
     * @param newName The new property name to use.
     * @return MaterialPtr to this material for method chaining.
     *
     * Allows flexibility for materials to work with shaders expecting different uniform names.
     * Example: material->setUniformNameOverride("u_material.diffuse", "u_baseColor")
     * Validates that oldName exists before remapping. If oldName doesn't exist, logs a warning.
     */
    MaterialPtr setUniformNameOverride(const std::string& oldName, const std::string& newName) {
        auto it = m_properties.find(oldName);
        if (it == m_properties.end()) {
            std::cerr << "Warning: Cannot override uniform name '" << oldName 
                      << "' - property not found in material." << std::endl;
            return shared_from_this();
        }

        if (oldName == newName) {
            std::cerr << "Warning: Old name and new name are identical ('" << oldName 
                      << "'). No remapping performed." << std::endl;
            return shared_from_this();
        }

        // Move the value to the new key
        PropertyVariant value = std::move(it->second);
        m_properties.erase(it);
        m_properties[newName] = std::move(value);
        return shared_from_this();
    }

    /**
     * @brief Gets the material properties map.
     * @return A const reference to the MaterialData containing all properties.
     */
    const MaterialData& getProperties() const {
        return m_properties;
    }

    /**
     * @brief Factory method to create an empty material with no properties.
     * @return A MaterialPtr to a newly created empty Material.
     *
     * Creates a material with no default properties initialized.
     * Use this when you want to manually set all properties without defaults.
     */
    static MaterialPtr Make() {
        return MakeEmpty();
    }

    /**
     * @brief Factory method to create a standard PBR material from an RGB color.
     * @param rgb The base RGB color for the material.
     * @return A MaterialPtr to the newly created Material with PBR properties.
     *
     * Creates a material with:
     * - Ambient: 20% of base color
     * - Diffuse: full base color
     * - Specular: neutral gray (0.5, 0.5, 0.5)
     * - Shininess: moderate value (32.0)
     */
    static MaterialPtr Make(const glm::vec3& rgb) {
        auto mat = MaterialPtr(new Material());
        mat->setAmbient(rgb * 0.2f);
        mat->setDiffuse(rgb);
        mat->setSpecular(glm::vec3(0.5f));
        mat->setShininess(32.0f);
        return mat;
    }
};

// Forward declaration for stack accessor
class MaterialStack;
using MaterialStackPtr = std::shared_ptr<MaterialStack>;

/**
 * @class MaterialStack
 * @brief Hierarchical state machine for material property management.
 *
 * Follows Meyers singleton pattern, mirroring texture::stack() and shader::stack().
 * Maintains a stack of merged material property states with efficient O(1) operations.
 * The base state (index 0) contains default PBR properties and cannot be popped.
 */
class MaterialStack {
private:
    /// @brief Vector of merged property maps - each level contains the fully merged state.
    /// Index 0 is the base state that can never be popped.
    std::vector<MaterialData> m_stack;

    /**
     * @brief Private constructor following Meyers singleton pattern.
     *
     * Initializes with base material containing standard PBR properties at index 0.
     */
    MaterialStack() {
        MaterialData base_state;
        
        base_state[Material::s_ambient_name] = glm::vec3(0.2f, 0.2f, 0.2f);
        base_state[Material::s_diffuse_name] = glm::vec3(0.8f, 0.8f, 0.8f);
        base_state[Material::s_specular_name] = glm::vec3(0.5f, 0.5f, 0.5f);
        base_state[Material::s_shininess_name] = 32.0f;
        
        m_stack.push_back(std::move(base_state));
    }

    friend MaterialStackPtr stack();

public:
    MaterialStack(const MaterialStack&) = delete;
    MaterialStack& operator=(const MaterialStack&) = delete;
    ~MaterialStack() = default;

    /**
     * @brief Pushes a material onto the stack, merging with current state.
     * @param mat A MaterialPtr to the Material to push.
     *
     * O(1) operation - copies current state and merges new properties.
     * Incoming properties overwrite existing keys in the merged state.
     */
    void push(MaterialPtr mat) {
        if (!mat) {
            std::cerr << "Error: Cannot push null material to stack. "
                      << "Material pointer is null." << std::endl;
            return;
        }

        if (m_stack.empty()) {
            std::cerr << "Error: Material stack is corrupted (empty). "
                      << "Reinitializing with base state." << std::endl;
            m_stack.push_back(MaterialData());
        }

        MaterialData new_state = m_stack.back();
        
        const auto& incoming_props = mat->getProperties();
        if (incoming_props.empty()) {
            std::cerr << "Warning: Pushing material with no properties. "
                      << "Stack state unchanged." << std::endl;
        }
        
        for (const auto& [name, value] : incoming_props) {
            new_state[name] = value;
        }
        
        m_stack.push_back(std::move(new_state));
    }

    /**
     * @brief Pops material from stack with base state protection.
     *
     * Never allows popping index 0 (base state).
     * Validates stack size to prevent underflow.
     */
    void pop() {
        if (m_stack.size() <= 1) {
            std::cerr << "Warning: Cannot pop base material state from stack (size=" 
                      << m_stack.size() << "). Base state must always remain. "
                      << "Operation ignored." << std::endl;
            return;
        }
        m_stack.pop_back();
    }

    /**
     * @brief Gets the current value of a property with type safety.
     * @tparam T The type of the property value to retrieve.
     * @param name The name of the property to retrieve.
     * @return The property value, or a default-constructed value if not found or type mismatch.
     *
     * O(1) access to current merged state.
     * Returns default-constructed value if property not found or type mismatch.
     */
    template<typename T>
    T getValue(const std::string& name) const {
        const auto& current_state = m_stack.back();
        auto it = current_state.find(name);
        
        if (it == current_state.end()) {
            std::cerr << "Warning: Material property '" << name 
                      << "' not found in current state. Returning default value." << std::endl;
            return T{};
        }
        
        const T* value_ptr = std::get_if<T>(&it->second);
        if (value_ptr) {
            return *value_ptr;
        } else {
            std::cerr << "Warning: Type mismatch for property '" << name 
                      << "'. Requested type does not match stored type. "
                      << "Returning default value." << std::endl;
            return T{};
        }
    }

    /**
     * @brief Gets a provider function for a property.
     * @tparam T The type of the property value.
     * @param name The name of the property.
     * @return A std::function that returns the current property value when called.
     *
     * Returns std::function that captures stack reference and property name.
     * Provider executes getValue when called, enabling dynamic uniform binding.
     * Provider includes validation and returns default value on error.
     */
    template<typename T>
    std::function<T()> getProvider(const std::string& name) {
        if (name.empty()) {
            std::cerr << "Warning: Creating provider for empty property name. "
                      << "Provider will always return default value." << std::endl;
        }
        
        return [this, name]() -> T {
            return this->getValue<T>(name);
        };
    }

    /**
     * @brief Defines a global default value for a custom property.
     * @param name The uniform name (e.g., "u_roughness").
     * @param defaultValue The value to use when a material doesn't explicitly set this.
     * * Adds the property to the Base State (Index 0). 
     * Because push() copies the previous state, this ensures the key 
     * always exists in the map, silencing warnings and preventing state leaks.
     */
    template<typename T>
    void defineDefault(const std::string& name, const T& defaultValue) {
        if (m_stack.empty()) return; // Should never happen given constructor
        
        // Insert into the Base State (Index 0)
        // If it already exists, this does nothing (or you can force overwrite)
        MaterialData& base = m_stack[0];
        
        // We use assignment to ensure we overwrite if it exists, or insert if new
        base[name] = defaultValue; 
    }

    /**
     * @brief Configures shader with automatic uniform binding for all base state properties.
     * @param shader A shared pointer to the Shader to configure.
     *
     * Uses std::visit for type-safe provider creation.
     * Integrates with shader's configureDynamicUniform system.
     * Iterates over base state properties (index 0) and binds each to the shader.
     */
    void configureShaderDefaults(std::shared_ptr<shader::Shader> shader) {
        if (!shader) {
            std::cerr << "Error: Cannot configure null shader with material defaults. "
                      << "Shader pointer is null." << std::endl;
            return;
        }

        if (m_stack.empty()) {
            std::cerr << "Error: Material stack is empty. Cannot configure shader defaults." << std::endl;
            return;
        }

        const auto& base_state = m_stack[0];
        
        if (base_state.empty()) {
            std::cerr << "Warning: Base material state is empty. No properties to configure." << std::endl;
            return;
        }

        for (const auto& [name, variant_value] : base_state) {
            try {
                std::visit([this, &shader, &name](auto&& value) {
                    using T = std::decay_t<decltype(value)>;
                    try {
                        shader->configureDynamicUniform<T>(name, this->getProvider<T>(name));
                    } catch (const std::exception& e) {
                        std::cerr << "Error: Failed to configure dynamic uniform '" << name 
                                  << "': " << e.what() << std::endl;
                    }
                }, variant_value);
            } catch (const std::exception& e) {
                std::cerr << "Error: Exception while processing property '" << name 
                          << "': " << e.what() << std::endl;
            }
        }
    }
};

/**
 * @brief Provides access to the singleton instance of MaterialStack.
 * @return A shared pointer to the singleton MaterialStack instance.
 *
 * Follows Meyers singleton pattern, matching texture::stack() and shader::stack() design.
 */
inline MaterialStackPtr stack() {
    static MaterialStackPtr instance = MaterialStackPtr(new MaterialStack());
    return instance;
}

} // namespace material

#endif // MATERIAL_H
