#include "Sky.h"
#include <vector>
#include <cmath>
#include <algorithm> // For std::clamp
#include <Shader.h>

// --- Simplified Noise Functions (Can be moved to a utility header if desired) ---

// Utility function to generate a pseudo-random value based on two integer coordinates
inline float hash2D(int a, int b) {
    int h = a * 374761393 + b * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return (h & 0x7fffffff) / float(0x7fffffff) * 2.0f - 1.0f;
}

// Simple 2D Interpolated Value Noise
float noise2D(float x, float y) {
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));
    float xf = x - xi;
    float yf = y - yi;
    
    // Cubic interpolation weights
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);
    
    // Get corner values
    float a = hash2D(xi, yi);
    float b = hash2D(xi + 1, yi);
    float c = hash2D(xi, yi + 1);
    float d = hash2D(xi + 1, yi + 1);
    
    // Bilinear interpolation
    float result = (a * (1.0f - u) + b * u) * (1.0f - v) +
                   (c * (1.0f - u) + d * u) * v;
    return result;
}

// Multi-octave noise (FBM)
float fbm(float x, float y, int octaves, float persistance = 0.5f) {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        result += noise2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistance;
        frequency *= 2.0f;
    }
    
    // Normalize to [-1, 1] range
    return result / maxValue;
}

// --- Sky Class Methods ---

void Sky::GenerateCloudTexture() {
    const int size = 256; 
    std::vector<unsigned char> data(size * size * 4);
    
    // Use a higher scale factor for smoother clouds in the texture
    const float noise_scale = 32.0f; 
    const int octaves = 4;
    
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float fx = x / noise_scale;
            float fy = y / noise_scale;
            
            float noise = fbm(fx, fy, octaves);
            
            // Remap from [-1, 1] to [0, 1]
            noise = (noise + 1.0f) * 0.5f;
            
            // Apply contrast to create distinct cloud formations
            noise = std::pow(noise, 1.5f);
            
            unsigned char value = static_cast<unsigned char>(
                std::clamp(noise * 255.0f, 0.0f, 255.0f)
            );
            
            int idx = (y * size + x) * 4;
            data[idx + 0] = value; 
            data[idx + 1] = value; 
            data[idx + 2] = value; 
            data[idx + 3] = 255; 
        }
    }
    
    // Texture setup (Unchanged - it was correct)
    glGenTextures(1, &cloudTexture);
    glBindTexture(GL_TEXTURE_2D, cloudTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, 
                 GL_UNSIGNED_BYTE, data.data());
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Use nearest filter for mag to avoid blurriness on close-up or highly scaled noise
    // However, linear is usually preferred for clouds. Sticking to your original linear setup:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
Sky::Sky() : VAO(0), VBO(0), cloudTexture(0), time(0.0f) {
    // Initialization list handles member initialization.
}

// Destructor (Implementation for ??1Sky@@QAE@XZ)
Sky::~Sky() {
    // Only attempt to delete if objects were actually generated
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (cloudTexture != 0) glDeleteTextures(1, &cloudTexture);
}

void Sky::Init() {
    GenerateCloudTexture();
    
    // Fullscreen quad vertices (clip space)
    static float quadVertices[] = {
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Sky::Update(float dt) {
 
    time += dt;
}

void Sky::Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection, 
                 const glm::vec3& sunDir) {
    // Save current state
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    GLint depthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    
    // Configure for sky rendering
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    
    shader.Bind();
    
    // Set uniforms
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProj = glm::inverse(projection);
    shader.SetUniformMat4f("invView", invView);
    shader.SetUniformMat4f("invProjection", invProj);
    shader.SetUniform1f("time", time);
    
    // Sparse cloud settings - fewer clouds but denser
    shader.SetUniform1f("cirrus", 0.75f);   // Even higher threshold = fewer wispy clouds
    shader.SetUniform1f("cumulus", 0.90f);  // Even higher threshold = fewer puffy clouds
    
    // Draw fullscreen quad
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Restore state
    glDepthMask(depthMask);
    glDepthFunc(depthFunc);
}