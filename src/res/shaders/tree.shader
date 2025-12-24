#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec3 v_Color;

uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    // Direct vertex transformation (no instance matrix)
    vec4 worldPos = vec4(aPos, 1.0);
    v_FragPos = worldPos.xyz;
    
    // Direct normal (already in world space)
    v_Normal = aNormal;
    
    // Pass through vertex color
    v_Color = aColor;
    
    gl_Position = u_Projection * u_View * worldPos;
}

#shader fragment
#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec3 v_Color;

out vec4 FragColor;

uniform vec3 u_LightDir;

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightDir);

    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);

    // Strong ambient so it never goes dark
    float ambient = 0.6;

    // Final brightness
    float lighting = ambient + diff * 0.6;

    vec3 color = v_Color * lighting;

    // Clamp to avoid dark collapse
    color = max(color, v_Color * 0.7);

    FragColor = vec4(color, 1.0);
}