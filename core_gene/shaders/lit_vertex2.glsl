#version 410

// Vertex attributes
layout (location=0) in vec4 vertex;
layout (location=1) in vec3 normal;
layout (location=2) in vec4 color;

// Transformation matrices
uniform mat4 M;          // Model matrix
uniform mat4 V;          // View matrix
uniform mat4 P;          // Projection matrix
uniform mat3 normalMatrix; // Normal transformation matrix (inverse transpose of model)

// Output to fragment shader
out vec3 fragPos;
out vec3 fragNormal;
out vec4 fragColor;

void main() {
    // Transform vertex to world space
    vec4 worldPos = M * vertex;
    fragPos = worldPos.xyz;
    
    // Transform normal to world space
    fragNormal = normalize(normalMatrix * normal);
    
    // Pass through color
    fragColor = color;
    
    // Transform to clip space
    gl_Position = P * V * worldPos;
}
