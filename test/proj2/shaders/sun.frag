#version 410 core

out vec4 FragColor;
in vec2 v_texCoord;

uniform sampler2D u_diffuseMap;
uniform bool u_hasDiffuseMap;

uniform vec3  u_material_diffuse;

void main()
{
    vec3 color;
    if (u_hasDiffuseMap) {
        color = texture(u_diffuseMap, v_texCoord).rgb;
    } else {
        color = u_material_diffuse;
    }
    
    FragColor = vec4(color, 1.0);
}