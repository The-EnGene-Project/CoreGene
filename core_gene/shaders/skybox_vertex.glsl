#version 430 core

layout(location = 0) in vec3 a_position;

out vec3 v_texCoords;

uniform mat4 u_viewProjection;

void main() {
    vec4 pos = u_viewProjection * vec4(a_position, 1.0);
    
    // Set z = w to ensure skybox is always at maximum depth
    gl_Position = pos.xyww;
    
    // Use local position as texture coordinate
    v_texCoords = a_position;
}
