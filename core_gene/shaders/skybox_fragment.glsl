#version 430 core

in vec3 v_texCoords;
out vec4 FragColor;

uniform samplerCube u_skybox;

void main() {
    // Coordinate system conversion for EnGene (if needed)
    vec3 texCoords = vec3(v_texCoords.x, v_texCoords.y, -v_texCoords.z);
    
    FragColor = texture(u_skybox, texCoords);
}
