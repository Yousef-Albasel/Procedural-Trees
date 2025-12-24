#include "Tree.h"
#include <stack>
#include <cmath>
#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

Tree::Tree() 
    : branchAngle(25.0f),
      lengthScale(0.90f),
      radiusScale(0.88f),
      initialLength(4.0f),
      initialRadius(0.35f),
      radialSegments(8),
      leafSize(0.3f),
      leafDensity(0.7f),
      minLeafDepth(3),
      VAO(0), VBO(0), NBO(0), EBO(0), instanceVBO(0),
      leafVAO(0), leafVBO(0), leafUVBO(0), leafEBO(0), leafInstanceVBO(0),
      buffersInitialized(false),
      leafBuffersInitialized(false),
      branchCount(0),
      templateVertexCount(0),
      leafTexture(0),
      position(glm::vec3(0.0f))
{
    // Default L-System: Simple branching tree with leaves
    axiom = "F";
    rules['F'] = "F[+F][-F]F";
}

Tree::~Tree() {
    Clean();
}

void Tree::Init(const glm::vec3& pos) {
    position = pos;
    
    // Create the template cylinder mesh for branches
    CreateCylinderTemplate();
    
    // Create the template quad mesh for leaves
    CreateLeafQuadTemplate();
    
    // Initialize OpenGL buffers for branches
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &NBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceVBO);
    
    // Initialize OpenGL buffers for leaves
    glGenVertexArrays(1, &leafVAO);
    glGenBuffers(1, &leafVBO);
    glGenBuffers(1, &leafUVBO);
    glGenBuffers(1, &leafEBO);
    glGenBuffers(1, &leafInstanceVBO);
    
    SetupBuffers();
    SetupLeafBuffers();
    
    buffersInitialized = true;
    leafBuffersInitialized = true;
    
    std::cout << "Tree initialized at position: (" 
              << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    std::cout << "Template cylinder: " << templateVertices.size() << " vertices, "
              << templateIndices.size() << " indices" << std::endl;
    std::cout << "Leaf quad template created with " << leafQuadVertices.size() << " vertices" << std::endl;
}

void Tree::CreateCylinderTemplate() {
    templateVertices.clear();
    templateNormals.clear();
    templateIndices.clear();
    
    int segments = radialSegments;
    
    // Generate vertices around the cylinder
    for (int i = 0; i <= segments; i++) {
        float theta = (float)i / segments * 2.0f * glm::pi<float>();
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);
        
        // Bottom ring (y=0)
        glm::vec3 pos(cosTheta, 0.0f, sinTheta);
        templateVertices.push_back(pos);
        templateNormals.push_back(glm::normalize(glm::vec3(cosTheta, 0.0f, sinTheta)));
        
        // Top ring (y=1)
        pos = glm::vec3(cosTheta, 1.0f, sinTheta);
        templateVertices.push_back(pos);
        templateNormals.push_back(glm::normalize(glm::vec3(cosTheta, 0.0f, sinTheta)));
    }
    
    // Generate indices for the cylinder sides
    unsigned int baseIndex = 0;
    for (int i = 0; i < segments; i++) {
        unsigned int base = baseIndex + i * 2;
        
        templateIndices.push_back(base);
        templateIndices.push_back(base + 2);
        templateIndices.push_back(base + 1);
        
        templateIndices.push_back(base + 1);
        templateIndices.push_back(base + 2);
        templateIndices.push_back(base + 3);
    }
    
    // Add bottom cap
    unsigned int centerBottomIndex = templateVertices.size();
    templateVertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    templateNormals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    
    for (int i = 0; i < segments; i++) {
        unsigned int base = baseIndex + i * 2;
        templateIndices.push_back(centerBottomIndex);
        templateIndices.push_back(base);
        templateIndices.push_back(base + 2);
    }
    
    // Add top cap
    unsigned int centerTopIndex = templateVertices.size();
    templateVertices.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    templateNormals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    
    for (int i = 0; i < segments; i++) {
        unsigned int base = baseIndex + i * 2 + 1;
        templateIndices.push_back(centerTopIndex);
        templateIndices.push_back(base + 2);
        templateIndices.push_back(base);
    }
    
    templateVertexCount = templateIndices.size();
}


// Add this method to Tree.cpp after the Generate() function:
void Tree::GenerateLeavesAtEndpoints() {
    std::cout << "Generating leaves at endpoints..." << std::endl;
    
    for (const auto& branch : branchInstances) {
        // Extract radius from the transform matrix (scale component)
        float radius = glm::length(glm::vec3(branch.transform[0]));
        
        // More lenient threshold - adjust to control where leaves appear
        // Lower value = only thinnest branches, Higher value = more branches get leaves
        if (radius < initialRadius * 0.25f) {  // Increased from 0.15f
            
            // Extract the END position of the branch
            // The transform is: Translation * Rotation * Scale
            // Position is in column 3, but we need to add the length
            glm::vec3 startPos = glm::vec3(branch.transform[3]);
            
            // Direction is the Y-axis of the rotation (column 1)
            glm::vec3 direction = glm::normalize(glm::vec3(branch.transform[1]));
            
            // Length is the Y-component of the scale
            float length = glm::length(glm::vec3(branch.transform[1]));
            
            // Calculate the END position of this branch
            glm::vec3 endPos = startPos + direction * length;
            
            // Generate MORE leaves per cluster for denser foliage
            int leavesPerCluster = 5 + (rand() % 6);  // 5-10 leaves per cluster (was 3-6)
            
            for (int i = 0; i < leavesPerCluster; i++) {
                // Larger offset for more spread-out clusters
                float offsetDist = leafSize * 1.0f;  // Increased from 0.5f
                glm::vec3 randomOffset(
                    ((rand() % 100) / 50.0f - 1.0f) * offsetDist,
                    ((rand() % 100) / 50.0f - 1.0f) * offsetDist,
                    ((rand() % 100) / 50.0f - 1.0f) * offsetDist
                );
                
                glm::vec3 leafPos = endPos + randomOffset;  // Use endPos instead of position
                
                // Create leaf instance
                LeafInstance instance;
                instance.position = leafPos;
                
                // Spherical normal for lighting
                glm::vec3 treeCenter = this->position + glm::vec3(0.0f, initialLength * 2.0f, 0.0f);
                instance.normal = glm::normalize(leafPos - treeCenter);
                
                // Slightly larger leaves for better coverage
                float scaleVariation = 0.9f + (rand() % 50) / 100.0f;  // 0.9 to 1.4
                instance.scale = glm::vec2(leafSize * scaleVariation * 1.2f);  // 20% larger
                
                // Random rotation
                instance.rotation = (rand() % 360) * glm::pi<float>() / 180.0f;
                
                // Leaf color with variation
                float colorVariation = 0.85f + (rand() % 30) / 100.0f;  // More variation
                instance.color = glm::vec3(0.2f, 0.6f, 0.15f) * colorVariation;
                
                leafInstances.push_back(instance);
            }
        }
    }
    
    std::cout << "Generated " << leafInstances.size() << " leaves at branch endpoints" << std::endl;
}


void Tree::CreateLeafQuadTemplate() {
    leafQuadVertices.clear();
    leafQuadUVs.clear();
    leafQuadIndices.clear();
    
    // Create a simple quad (two triangles) centered at origin
    // The quad lies in the XY plane
    leafQuadVertices = {
        glm::vec3(-0.5f, -0.5f, 0.0f),  // Bottom-left
        glm::vec3( 0.5f, -0.5f, 0.0f),  // Bottom-right
        glm::vec3( 0.5f,  0.5f, 0.0f),  // Top-right
        glm::vec3(-0.5f,  0.5f, 0.0f)   // Top-left
    };
    
    // UV coordinates
    leafQuadUVs = {
        glm::vec2(0.0f, 0.0f),  // Bottom-left
        glm::vec2(1.0f, 0.0f),  // Bottom-right
        glm::vec2(1.0f, 1.0f),  // Top-right
        glm::vec2(0.0f, 1.0f)   // Top-left
    };
    
    // Indices for two triangles
    leafQuadIndices = {
        0, 1, 2,  // First triangle
        0, 2, 3   // Second triangle
    };
}

void Tree::SetupBuffers() {
    glBindVertexArray(VAO);
    
    // Vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, templateVertices.size() * sizeof(glm::vec3), 
                 templateVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normals
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER, templateNormals.size() * sizeof(glm::vec3), 
                 templateNormals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, templateIndices.size() * sizeof(unsigned int), 
                 templateIndices.data(), GL_STATIC_DRAW);
    
    // Instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    
    // Instance matrix (4 vec4s for mat4)
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(BranchInstance), 
                             (void*)(i * sizeof(glm::vec4)));
        glEnableVertexAttribArray(2 + i);
        glVertexAttribDivisor(2 + i, 1);
    }
    
    // Instance color
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(BranchInstance), 
                         (void*)(offsetof(BranchInstance, color)));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    
    // Instance thickness
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(BranchInstance), 
                         (void*)(offsetof(BranchInstance, thickness)));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);
    
    glBindVertexArray(0);
}

void Tree::SetupLeafBuffers() {
    glBindVertexArray(leafVAO);
    
    // Vertex positions (quad template)
    glBindBuffer(GL_ARRAY_BUFFER, leafVBO);
    glBufferData(GL_ARRAY_BUFFER, leafQuadVertices.size() * sizeof(glm::vec3), 
                 leafQuadVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    // UV coordinates
    glBindBuffer(GL_ARRAY_BUFFER, leafUVBO);
    glBufferData(GL_ARRAY_BUFFER, leafQuadUVs.size() * sizeof(glm::vec2), 
                 leafQuadUVs.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, leafEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, leafQuadIndices.size() * sizeof(unsigned int), 
                 leafQuadIndices.data(), GL_STATIC_DRAW);
    
    // Instance buffer for leaf data
    glBindBuffer(GL_ARRAY_BUFFER, leafInstanceVBO);
    
    // Instance position (vec3)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, position));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    // Instance normal (vec3) - spherical normal for lighting
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, normal));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    
    // Instance scale (vec2)
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, scale));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    // Instance rotation (float)
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, rotation));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    
    // Instance color (vec3)
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, color));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    
    glBindVertexArray(0);
}

void Tree::AddRule(char symbol, const std::string& replacement) {
    rules[symbol] = replacement;
}

void Tree::InterpretLSystemRecursive(char symbol, int depth, int maxDepth, 
                                     TurtleState& turtle, std::stack<TurtleState>& stack) {
    bool shouldExpand = (depth < maxDepth) && (rules.find(symbol) != rules.end());
    
    if (shouldExpand) {
        const std::string& replacement = rules[symbol];
        for (char c : replacement) {
            InterpretLSystemRecursive(c, depth + 1, maxDepth, turtle, stack);
        }
    } else {
        InterpretSymbol(symbol, turtle, stack);
    }
}

void Tree::InterpretSymbol(char c, TurtleState& turtle, std::stack<TurtleState>& stack) {
    switch (c) {
        case 'F': 
        case 'X': {
            const float minLength = 0.02f;
            const float minRadius = 0.005f;
            
            if (turtle.length < minLength || turtle.radius < minRadius) {
                break;
            }
            
            // Draw forward and create branch instance
            glm::vec3 endPos = turtle.position + turtle.direction * turtle.length;
            float endRadius = turtle.radius * radiusScale;
            
            if (endRadius < 0.01f) {
                endRadius = 0.01f;
            }
            
            GenerateBranchInstance(turtle.position, endPos, turtle.radius, endRadius, turtle.depth);
            
            turtle.position = endPos;
            turtle.radius = endRadius;
            turtle.length *= lengthScale;
            turtle.depth++;
            branchCount++;
            break;
        }
        
        case 'L': {
            // Generate leaf at current position
            // Only add leaves if depth is sufficient and random chance succeeds
            if (turtle.depth >= minLeafDepth) {
                float randomChance = (rand() % 100) / 100.0f;
                if (randomChance < leafDensity) {
                    GenerateLeafInstance(turtle.position, turtle.direction, turtle);
                }
            }
            break;
        }
        
        case '+': {
            float randomness = ((rand() % 100) / 100.0f - 0.5f) * 0.1f; 
            float angleRad = glm::radians(branchAngle) + randomness;
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.up);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.up);
            break;
        }
        
        case '-': {
            float angleRad = glm::radians(-branchAngle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.up);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.up);
            break;
        }
        
        case '&': {
            float angleRad = glm::radians(branchAngle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.right);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.right);
            break;
        }
        
        case '^': {
            float angleRad = glm::radians(-branchAngle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.right);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.right);
            break;
        }
        
        case '\\': {
            float angleRad = glm::radians(branchAngle);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.direction);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.direction);
            break;
        }
        
        case '/': {
            float angleRad = glm::radians(-branchAngle);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.direction);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.direction);
            break;
        }
        
        case '[': {
            stack.push(turtle);
            break;
        }
        
        case ']': {
            if (!stack.empty()) {
                turtle = stack.top();
                stack.pop();
            }
            break;
        }
    }
}

void Tree::Generate(int iterations) {
    std::cout << "Generating tree with " << iterations << " iterations..." << std::endl;
    
    const int MAX_SAFE_ITERATIONS = 10;
    if (iterations > MAX_SAFE_ITERATIONS) {
        std::cout << "WARNING: Iterations clamped to " << MAX_SAFE_ITERATIONS << std::endl;
        iterations = MAX_SAFE_ITERATIONS;
    }
    
    // Clear previous data
    branchInstances.clear();
    leafInstances.clear();
    stateStack.clear();
    branchCount = 0;
    
    std::cout << "Axiom: " << axiom << std::endl;
    std::cout << "Rules: " << std::endl;
    for (const auto& rule : rules) {
        std::cout << "  " << rule.first << " -> " << rule.second << std::endl;
    }
    
    // Initialize turtle state
    TurtleState turtle;
    turtle.position = position;
    turtle.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    turtle.right = glm::vec3(1.0f, 0.0f, 0.0f);
    turtle.up = glm::vec3(0.0f, 0.0f, 1.0f);
    turtle.length = initialLength;
    turtle.radius = initialRadius;
    turtle.depth = 0;
    
    std::stack<TurtleState> stack;
    
    std::cout << "Interpreting L-System recursively..." << std::endl;
    
    for (char c : axiom) {
        InterpretLSystemRecursive(c, 0, iterations, turtle, stack);
    }
    
    std::cout << "Branch instances created: " << branchInstances.size() << std::endl;
    GenerateLeavesAtEndpoints();

    std::cout << "Leaf instances created: " << leafInstances.size() << std::endl;
    
    // Update buffers
    if (buffersInitialized) {
        UpdateInstanceBuffer();
    }
    if (leafBuffersInitialized) {
        UpdateLeafInstanceBuffer();
    }
    
    std::cout << "Tree generated: " << branchCount << " branches, " 
              << leafInstances.size() << " leaves" << std::endl;
}

void Tree::GenerateBranchInstance(const glm::vec3& start, const glm::vec3& end,
                                  float startRadius, float endRadius, int depth) {
    BranchInstance instance;
    
    glm::vec3 direction = end - start;
    float length = glm::length(direction);
    
    if (length < 0.001f) return;
    
    direction = glm::normalize(direction);
    
    // Build rotation matrix
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 rotation;
    
    if (abs(glm::dot(direction, up)) > 0.999f) {
        rotation = glm::mat4(1.0f);
        if (direction.y < 0) {
            rotation = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        }
    } else {
        glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f)));
        if (glm::length(right) < 0.01f) {
            right = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f)));
        }
        glm::vec3 newUp = glm::cross(right, direction);
        
        rotation = glm::mat4(
            glm::vec4(right, 0.0f),
            glm::vec4(direction, 0.0f),
            glm::vec4(newUp, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );
    }
    
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), start);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(startRadius, length, startRadius));
    
    instance.transform = translation * rotation * scale;
    
    float depthFactor = 1.0f - (depth * 0.05f);
    depthFactor = glm::clamp(depthFactor, 0.5f, 1.0f);
    instance.color = glm::vec3(0.4f, 0.25f, 0.15f) * depthFactor;
    
    instance.thickness = endRadius / startRadius;
    
    branchInstances.push_back(instance);
}

void Tree::GenerateLeafInstance(const glm::vec3& leafPos, const glm::vec3& direction, 
                                const TurtleState& turtle) {
    LeafInstance instance;
    
    instance.position = leafPos;
    
    // Calculate spherical normal pointing outward from tree center
    // This creates the "volume lighting" effect
    glm::vec3 treeCenter = position + glm::vec3(0.0f, initialLength * 2.0f, 0.0f);
    instance.normal = glm::normalize(leafPos - treeCenter);
    
    // Random scale variation
    float scaleVariation = 0.8f + (rand() % 40) / 100.0f;  // 0.8 to 1.2
    instance.scale = glm::vec2(leafSize * scaleVariation);
    
    // Random rotation for variety
    instance.rotation = (rand() % 360) * glm::pi<float>() / 180.0f;
    
    // Leaf color with slight variation
    float colorVariation = 0.9f + (rand() % 20) / 100.0f;  // 0.9 to 1.1
    instance.color = glm::vec3(0.2f, 0.6f, 0.15f) * colorVariation;
    
    leafInstances.push_back(instance);
}

void Tree::UpdateInstanceBuffer() {
    if (branchInstances.empty()) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, branchInstances.size() * sizeof(BranchInstance),
                 branchInstances.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Tree::UpdateLeafInstanceBuffer() {
    if (leafInstances.empty()) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, leafInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, leafInstances.size() * sizeof(LeafInstance),
                 leafInstances.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Tree::LoadLeafTexture(const std::string& texturePath) {
    // Delete old texture if it exists
    if (leafTexture != 0) {
        glDeleteTextures(1, &leafTexture);
    }
    
    glGenTextures(1, &leafTexture);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Load image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        // Check if we need to convert black/white to alpha
        bool needsConversion = (nrChannels == 1 || nrChannels == 3);
        
        if (needsConversion) {
            std::cout << "Converting silhouette texture to RGBA with alpha..." << std::endl;
            
            // Create RGBA buffer
            int pixelCount = width * height;
            unsigned char* rgbaData = new unsigned char[pixelCount * 4];
            
            for (int i = 0; i < pixelCount; i++) {
                unsigned char value;
                
                if (nrChannels == 1) {
                    // Grayscale
                    value = data[i];
                } else {
                    // RGB - use average
                    value = (data[i * 3] + data[i * 3 + 1] + data[i * 3 + 2]) / 3;
                }
                
                // White leaf on black background: white = opaque, black = transparent
                rgbaData[i * 4 + 0] = 60;   // R - dark green
                rgbaData[i * 4 + 1] = 120;  // G - green
                rgbaData[i * 4 + 2] = 40;   // B - dark green
                rgbaData[i * 4 + 3] = value; // A - white becomes opaque, black becomes transparent
            }
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);
            delete[] rgbaData;
            
        } else {
            // Already has alpha channel
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Loaded leaf texture: " << texturePath << " (" << width << "x" << height << ", " 
                  << nrChannels << " channels)" << std::endl;
    } else {
        std::cerr << "Failed to load leaf texture: " << texturePath << std::endl;
        std::cerr << "Make sure the file exists at: " << texturePath << std::endl;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Tree::Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!buffersInitialized || branchInstances.empty()) return;
    
    shader.Bind();
    
    shader.setUniformMat4f("u_View", const_cast<glm::mat4&>(view));
    shader.setUniformMat4f("u_Projection", const_cast<glm::mat4&>(projection));
    
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 0.8f, -0.5f));
    shader.SetUniform3f("u_LightDir", lightDir.x, lightDir.y, lightDir.z);
    
    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, templateVertexCount, GL_UNSIGNED_INT, 0, 
                           branchInstances.size());
    glBindVertexArray(0);
    
    shader.Unbind();
}

void Tree::RenderLeaves(Shader& leafShader, const glm::mat4& view, const glm::mat4& projection) {
    if (!leafBuffersInitialized || leafInstances.empty()) return;
    
    leafShader.Bind();
    
    leafShader.setUniformMat4f("u_View", view);
    leafShader.setUniformMat4f("u_Projection", projection);
    
    // Light direction
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 0.8f, -0.5f));
    leafShader.SetUniform3f("u_LightDir", lightDir.x, lightDir.y, lightDir.z);
    
    // Bind leaf texture
    if (leafTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leafTexture);
        leafShader.SetUniform1i("u_LeafTexture", 0);
    }
    
    // Disable backface culling for double-sided leaves
    glDisable(GL_CULL_FACE);
    
    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindVertexArray(leafVAO);
    glDrawElementsInstanced(GL_TRIANGLES, leafQuadIndices.size(), GL_UNSIGNED_INT, 0, 
                           leafInstances.size());
    glBindVertexArray(0);
    
    // Restore state
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    
    leafShader.Unbind();
}

void Tree::Clean() {
    if (buffersInitialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &NBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &instanceVBO);
        buffersInitialized = false;
    }
    
    if (leafBuffersInitialized) {
        glDeleteVertexArrays(1, &leafVAO);
        glDeleteBuffers(1, &leafVBO);
        glDeleteBuffers(1, &leafUVBO);
        glDeleteBuffers(1, &leafEBO);
        glDeleteBuffers(1, &leafInstanceVBO);
        leafBuffersInitialized = false;
    }
    
    if (leafTexture != 0) {
        glDeleteTextures(1, &leafTexture);
        leafTexture = 0;
    }
    
    templateVertices.clear();
    templateNormals.clear();
    templateIndices.clear();
    leafQuadVertices.clear();
    leafQuadUVs.clear();
    leafQuadIndices.clear();
    branchInstances.clear();
    leafInstances.clear();
}