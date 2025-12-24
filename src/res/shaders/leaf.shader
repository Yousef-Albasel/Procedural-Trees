#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;      // Quad vertex position
layout(location = 1) in vec2 a_TexCoord;      // UV coordinates
layout(location = 2) in vec3 a_InstancePos;   // Instance position
layout(location = 3) in vec3 a_InstanceNormal; // Spherical normal for lighting
layout(location = 4) in vec2 a_InstanceScale;  // Scale (XY)
layout(location = 5) in float a_InstanceRotation; // Rotation around view axis
layout(location = 6) in vec3 a_InstanceColor;  // Color tint

out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_WorldPos;
out vec3 v_Color;

uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_InstanceColor;
    
    // The spherical normal (points from tree center outward)
    v_Normal = normalize(a_InstanceNormal);
    
    // Extract camera right and up vectors directly from view matrix
    // View matrix transforms world to camera space, so we extract the inverse directions
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);
    
    // Apply rotation around the view direction (for variety)
    float cosRot = cos(a_InstanceRotation);
    float sinRot = sin(a_InstanceRotation);
    
    vec3 rotatedRight = cameraRight * cosRot - cameraUp * sinRot;
    vec3 rotatedUp = cameraRight * sinRot + cameraUp * cosRot;
    
    // Scale the vertex
    vec3 scaledPos = a_Position.x * rotatedRight * a_InstanceScale.x
                   + a_Position.y * rotatedUp * a_InstanceScale.y;
    
    // World position
    v_WorldPos = a_InstancePos + scaledPos;
    
    gl_Position = u_Projection * u_View * vec4(v_WorldPos, 1.0);
}

#shader fragment
#version 330 core

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_WorldPos;
in vec3 v_Color;

out vec4 FragColor;

uniform sampler2D u_LeafTexture;
uniform vec3 u_LightDir;

void main() {
    // Sample the leaf texture
    vec4 texColor = texture(u_LeafTexture, v_TexCoord);
    
    // Alpha test - discard fully transparent pixels
    if (texColor.a < 0.1) {
        discard;
    }
    
    // Use the spherical normal for lighting
    // This creates the "volume lighting" effect where the entire canopy
    // lights up as one cohesive mass
    vec3 normal = normalize(v_Normal);
    
    // Check if we're looking at the back face
    // For double-sided lighting, flip the normal if needed
    if (!gl_FrontFacing) {
        normal = -normal;
    }
    
    // Diffuse lighting with softer falloff
    float diffuse = max(dot(normal, u_LightDir), 0.0);
    diffuse = pow(diffuse, 0.7); // Soften the transition
    
    // Add ambient light so leaves in shadow aren't completely black
    float ambient = 0.35;
    float lightIntensity = ambient + diffuse * 0.65;
    
    // Use texture color as base, modulate with instance color and lighting
    vec3 leafColor = texColor.rgb * v_Color;
    vec3 finalColor = leafColor * lightIntensity;
    
    // Subsurface scattering effect (leaves glow a bit when backlit)
    float backlight = max(dot(normal, -u_LightDir), 0.0);
    vec3 subsurfaceColor = vec3(0.4, 0.7, 0.3); // Bright green-yellow
    finalColor += subsurfaceColor * backlight * 0.25 * texColor.a;
    
    // Add slight edge lighting for more definition
    float fresnel = pow(1.0 - abs(dot(normal, normalize(v_WorldPos))), 2.0);
    finalColor += vec3(0.3, 0.5, 0.2) * fresnel * 0.15;
    
    FragColor = vec4(finalColor, texColor.a);
}