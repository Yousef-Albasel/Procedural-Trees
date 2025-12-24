#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
out vec3 worldPos;
out vec3 sunPosition;

uniform mat4 invView;
uniform mat4 invProjection;
uniform float time;

void main() {
    worldPos = position;
    gl_Position = vec4(position, 1.0);
    
    // Reconstruct world direction
    vec4 clip = vec4(position.xy, 1.0, 1.0);
    vec4 eye = invProjection * clip;
    eye.xyz /= eye.w;
    worldPos = normalize((invView * vec4(eye.xyz, 0.0)).xyz);
    
    // Animated sun position (slower for more stable lighting)
    sunPosition = vec3(0.0, sin(time * 0.005), cos(time * 0.005));
}

#shader fragment
#version 330 core
in vec3 worldPos;
in vec3 sunPosition;
out vec4 FragColor;

uniform float time;
uniform float cirrus;
uniform float cumulus;

// Atmospheric scattering constants - adjusted for pure blue sky
const float Br = 0.0015;  // Reduced even more
const float Bm = 0.0001;  // Reduced mie scattering
const float g = 0.70;     // Less forward scattering
const vec3 nitrogen = vec3(0.650, 0.570, 0.475);  // More blue dominant
const vec3 Kr = Br / pow(nitrogen, vec3(4.0));
const vec3 Km = Bm / pow(nitrogen, vec3(0.84));

// Hash function for noise
float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

// 3D Noise function
float noise(vec3 x) {
    vec3 f = fract(x);
    float n = dot(floor(x), vec3(1.0, 157.0, 113.0));
    return mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
                   mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
               mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
                   mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

// Fractal Brownian Motion
const mat3 m = mat3(0.0, 1.60, 1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
float fbm(vec3 p) {
    float f = 0.0;
    f += noise(p) / 2.0; p = m * p * 1.1;
    f += noise(p) / 4.0; p = m * p * 1.2;
    f += noise(p) / 6.0; p = m * p * 1.3;
    f += noise(p) / 12.0; p = m * p * 1.4;
    f += noise(p) / 24.0;
    return f;
}

void main() {
    vec3 fsun = normalize(sunPosition);
    
    // Atmospheric Scattering
    float mu = dot(normalize(worldPos), fsun);
    float rayleigh = 3.0 / (8.0 * 3.14159) * (1.0 + mu * mu);
    vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / 
                pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

    // Use absolute value of worldPos.y to make horizon calculation work for below horizon too
    float skyY = abs(worldPos.y);
    
    vec3 day_extinction = exp(-exp(-((skyY + fsun.y * 4.0) * 
                          (exp(-skyY * 16.0) + 0.1) / 80.0) / Br) * 
                          (exp(-skyY * 16.0) + 0.1) * Kr / Br) * 
                          exp(-skyY * exp(-skyY * 8.0) * 4.0) * 
                          exp(-skyY * 2.0) * 4.0;
    
    vec3 night_extinction = vec3(1.0 - exp(fsun.y)) * 0.2;
    vec3 extinction = mix(day_extinction, night_extinction, -fsun.y * 0.2 + 0.5);
    
    // Strong blue boost, remove yellow/red tints completely
    extinction = extinction * vec3(0.5, 0.7, 1.5);
    
    // Clamp out any brown/yellow at horizon
    extinction = max(extinction, vec3(0.4, 0.5, 0.7));
    
    vec3 color = rayleigh * mie * extinction;

    // Only render clouds above horizon
    if (worldPos.y > 0.0) {
        // Sparse Cirrus Clouds (high, wispy) - very few but denser
        float cirrusDensity = fbm(worldPos.xyz / worldPos.y * 1.5 + time * 0.005);
        cirrusDensity = smoothstep(1.0 - cirrus, 1.0, cirrusDensity);
        cirrusDensity = pow(cirrusDensity, 2.0);
        color = mix(color, extinction * 4.5, cirrusDensity * worldPos.y);

        // Sparse Cumulus Clouds (puffy) - fewer but much denser
        for (int i = 0; i < 2; i++) {
            float cumulusDensity = fbm((0.8 + float(i) * 0.02) * worldPos.xyz / worldPos.y + 
                            time * 0.05);
            cumulusDensity = smoothstep(1.0 - cumulus, 1.0, cumulusDensity);
            cumulusDensity = pow(cumulusDensity, 3.0);
            color = mix(color, extinction * 5.0, min(cumulusDensity, 1.0) * worldPos.y);
        }
    }

    // Subtle dithering
    color += noise(worldPos * 1000.0) * 0.005;

    // Gentler tone mapping for softer look
    color = color / (color + vec3(1.2));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}