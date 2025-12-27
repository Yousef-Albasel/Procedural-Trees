#include "Tree.h"
#include <stack>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cctype>
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
      initialRadius(0.65f),
      radialSegments(8),
      angleRandomness(0.15f),
      lengthRandomness(0.1f),
      radiusRandomness(0.05f),
      tropism(0.0f, -0.2f, 0.0f),
      branchProbability(1.0f),
      leafSize(0.3f),
      leafDensity(0.7f),
      minLeafDepth(3),
      branchVAO(0), branchVBO(0), branchNBO(0), branchCBO(0), branchEBO(0),
      leafVAO(0), leafVBO(0), leafUVBO(0), leafEBO(0), leafInstanceVBO(0),
      branchBuffersInitialized(false),
      leafBuffersInitialized(false),
      leafTexture(0),
      position(glm::vec3(0.0f))
{
    axiom = "F";
    rules['F'] = "F[+F][-F]F";
    srand(time(nullptr));
}

Tree::~Tree() {
    Clean();
}

void Tree::Init(const glm::vec3& pos) {
    position = pos;
    
    CreateLeafQuadTemplate();
    
    // Initialize OpenGL buffers for branches
    glGenVertexArrays(1, &branchVAO);
    glGenBuffers(1, &branchVBO);
    glGenBuffers(1, &branchNBO);
    glGenBuffers(1, &branchCBO);
    glGenBuffers(1, &branchEBO);
    
    // Initialize OpenGL buffers for leaves
    glGenVertexArrays(1, &leafVAO);
    glGenBuffers(1, &leafVBO);
    glGenBuffers(1, &leafUVBO);
    glGenBuffers(1, &leafEBO);
    glGenBuffers(1, &leafInstanceVBO);
    
    SetupLeafBuffers();
    
    branchBuffersInitialized = true;
    leafBuffersInitialized = true;
    
    std::cout << "Tree initialized at position: (" 
              << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

float Tree::RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

float Tree::ApplyRandomness(float value, float randomness) {
    float variation = RandomFloat(-randomness, randomness);
    return value * (1.0f + variation);
}

SegmentParams Tree::ParseSegmentParams(const std::string& str, size_t& pos) {
    SegmentParams params;
    
    // Check if there's a parameter list
    if (pos + 1 < str.length() && str[pos + 1] == '(') {
        size_t startPos = pos + 2;
        size_t endPos = str.find(')', startPos);
        
        if (endPos != std::string::npos) {
            std::string paramStr = str.substr(startPos, endPos - startPos);
            
            // Parse comma-separated values
            size_t commaPos = paramStr.find(',');
            
            if (commaPos != std::string::npos) {
                // Two parameters: F(length, radius)
                try {
                    params.length = std::stof(paramStr.substr(0, commaPos));
                    params.radius = std::stof(paramStr.substr(commaPos + 1));
                } catch (...) {
                    std::cerr << "Failed to parse segment parameters: " << paramStr << std::endl;
                }
            } else {
                // One parameter: F(length)
                try {
                    params.length = std::stof(paramStr);
                } catch (...) {
                    std::cerr << "Failed to parse segment parameter: " << paramStr << std::endl;
                }
            }
            
            // Move position past the closing parenthesis
            pos = endPos;
        }
    }
    
    return params;
}

void Tree::CreateLeafQuadTemplate() {
    leafQuadVertices.clear();
    leafQuadUVs.clear();
    leafQuadIndices.clear();
    
    leafQuadVertices = {
        glm::vec3(-0.5f, -0.5f, 0.0f),
        glm::vec3( 0.5f, -0.5f, 0.0f),
        glm::vec3( 0.5f,  0.5f, 0.0f),
        glm::vec3(-0.5f,  0.5f, 0.0f)
    };
    
    leafQuadUVs = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(0.0f, 1.0f)
    };
    
    leafQuadIndices = { 0, 1, 2, 0, 2, 3 };
}

void Tree::SetupLeafBuffers() {
    glBindVertexArray(leafVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, leafVBO);
    glBufferData(GL_ARRAY_BUFFER, leafQuadVertices.size() * sizeof(glm::vec3), 
                 leafQuadVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, leafUVBO);
    glBufferData(GL_ARRAY_BUFFER, leafQuadUVs.size() * sizeof(glm::vec2), 
                 leafQuadUVs.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, leafEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, leafQuadIndices.size() * sizeof(unsigned int), 
                 leafQuadIndices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, leafInstanceVBO);
    
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, position));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, normal));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, scale));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, rotation));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(LeafInstance), 
                         (void*)offsetof(LeafInstance, color));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    
    glBindVertexArray(0);
}

void Tree::SetupBranchBuffers() {
    glBindVertexArray(branchVAO);
    
    // Vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, branchVBO);
    glBufferData(GL_ARRAY_BUFFER, branchVertices.size() * sizeof(glm::vec3), 
                 branchVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normals
    glBindBuffer(GL_ARRAY_BUFFER, branchNBO);
    glBufferData(GL_ARRAY_BUFFER, branchNormals.size() * sizeof(glm::vec3), 
                 branchNormals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Colors
    glBindBuffer(GL_ARRAY_BUFFER, branchCBO);
    glBufferData(GL_ARRAY_BUFFER, branchColors.size() * sizeof(glm::vec3), 
                 branchColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(2);
    
    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, branchEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, branchIndices.size() * sizeof(unsigned int), 
                 branchIndices.data(), GL_STATIC_DRAW);
    
    glBindVertexArray(0);
}

void Tree::AddRule(char symbol, const std::string& replacement) {
    rules[symbol] = replacement;
}

void Tree::InterpretLSystemRecursive(char symbol, int depth, int maxDepth, 
                                     TurtleState& turtle, std::stack<TurtleState>& stack,
                                     int currentSegmentIndex) {
    bool shouldExpand = (depth < maxDepth) && (rules.find(symbol) != rules.end());
    
    if (shouldExpand) {
        const std::string& replacement = rules[symbol];
        
        // Iterate through replacement string, handling parameterized segments
        for (size_t i = 0; i < replacement.length(); i++) {
            char c = replacement[i];
            
            // Check for parameterized F
            if (c == 'F' && i + 1 < replacement.length() && replacement[i + 1] == '(') {
                // Parse parameters and pass to recursive call
                std::string tempStr = replacement.substr(i);
                size_t tempPos = 0;
                SegmentParams params = ParseSegmentParams(tempStr, tempPos);
                
                // Store params temporarily in turtle for the recursive call
                float oldLength = turtle.length;
                float oldRadius = turtle.radius;
                turtle.length *= params.length;
                turtle.radius *= params.radius;
                
                InterpretLSystemRecursive(c, depth + 1, maxDepth, turtle, stack, currentSegmentIndex);
                
                turtle.length = oldLength;
                turtle.radius = oldRadius;
                
                // Skip past the parameters
                i += tempPos;
            } else {
                InterpretLSystemRecursive(c, depth + 1, maxDepth, turtle, stack, currentSegmentIndex);
            }
        }
    } else {
        // Check if this is a parameterized F at terminal level
        InterpretSymbol(symbol, turtle, stack, currentSegmentIndex);
    }
}

void Tree::InterpretSymbol(char c, TurtleState& turtle, std::stack<TurtleState>& stack,
                           int& currentSegmentIndex) {
    switch (c) {
        case 'F': 
        case 'X': {
            // Check for branch probability
            if (c == 'F' && RandomFloat(0.0f, 1.0f) > branchProbability) {
                break;
            }
            
            const float minLength = 0.02f;
            const float minRadius = 0.005f;
            
            if (turtle.length < minLength || turtle.radius < minRadius) {
                break;
            }
            
            // Apply randomness to length and radius
            float actualLength = ApplyRandomness(turtle.length, lengthRandomness);
            float actualRadius = ApplyRandomness(turtle.radius, radiusRandomness);
            
            // Apply tropism (gravitational/phototropic bias)
            glm::vec3 tropismDirection = turtle.direction + tropism;
            if (glm::length(tropismDirection) > 0.001f) {
                turtle.direction = glm::normalize(tropismDirection);
            }
            
            glm::vec3 endPos = turtle.position + turtle.direction * actualLength;
            float endRadius = actualRadius * radiusScale;
            
            if (endRadius < 0.01f) {
                endRadius = 0.01f;
            }
            
            // Create branch segment
            BranchSegment segment;
            segment.startPos = turtle.position;
            segment.endPos = endPos;
            segment.startRadius = actualRadius;
            segment.endRadius = endRadius;
            segment.depth = turtle.depth;
            segment.parentIndex = currentSegmentIndex;
            
            // Add to parent's children
            if (currentSegmentIndex >= 0 && currentSegmentIndex < branchSegments.size()) {
                branchSegments[currentSegmentIndex].childIndices.push_back(branchSegments.size());
            }
            
            currentSegmentIndex = branchSegments.size();
            branchSegments.push_back(segment);
            
            turtle.position = endPos;
            turtle.radius = endRadius;
            turtle.length *= lengthScale;
            turtle.depth++;
            break;
        }
        
        case '+': {
            // Apply angle randomness
            float angle = ApplyRandomness(branchAngle, angleRandomness);
            float angleRad = glm::radians(angle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.up);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.up);
            break;
        }
        
        case '-': {
            float angle = ApplyRandomness(branchAngle, angleRandomness);
            float angleRad = glm::radians(-angle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.up);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.up);
            break;
        }
        
        case '&': {
            float angle = ApplyRandomness(branchAngle, angleRandomness);
            float angleRad = glm::radians(angle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.right);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.right);
            break;
        }
        
        case '^': {
            float angle = ApplyRandomness(branchAngle, angleRandomness);
            float angleRad = glm::radians(-angle);
            turtle.direction = glm::rotate(turtle.direction, angleRad, turtle.right);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.right);
            break;
        }
        
        case '\\': {
            float angle = ApplyRandomness(branchAngle, angleRandomness);
            float angleRad = glm::radians(angle);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.direction);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.direction);
            break;
        }
        
        case '/': {
            float angle = ApplyRandomness(branchAngle, angleRandomness);
            float angleRad = glm::radians(-angle);
            turtle.right = glm::rotate(turtle.right, angleRad, turtle.direction);
            turtle.up = glm::rotate(turtle.up, angleRad, turtle.direction);
            break;
        }
        
        case '[': {
            stack.push(turtle);
            segmentIndexStack.push(currentSegmentIndex);
            turtle.radius *= ApplyRandomness(0.7f, radiusRandomness);
            break;
        }
        
        case ']': {
            if (!stack.empty()) {
                turtle = stack.top();
                stack.pop();
            }
            if (!segmentIndexStack.empty()) {
                currentSegmentIndex = segmentIndexStack.top();
                segmentIndexStack.pop();
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
    branchSegments.clear();
    leafInstances.clear();
    branchVertices.clear();
    branchNormals.clear();
    branchColors.clear();
    branchIndices.clear();
    
    // Clear stacks
    while (!segmentIndexStack.empty()) segmentIndexStack.pop();
    
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
    int currentSegmentIndex = -1;
    
    std::cout << "Interpreting L-System..." << std::endl;
    
    // Interpret axiom with parameterized segments
    for (size_t i = 0; i < axiom.length(); i++) {
        char c = axiom[i];
        
        // Check for parameterized F in axiom
        if (c == 'F' && i + 1 < axiom.length() && axiom[i + 1] == '(') {
            SegmentParams params = ParseSegmentParams(axiom, i);
            
            float oldLength = turtle.length;
            float oldRadius = turtle.radius;
            turtle.length *= params.length;
            turtle.radius *= params.radius;
            
            InterpretLSystemRecursive(c, 0, iterations, turtle, stack, currentSegmentIndex);
            
            turtle.length = oldLength;
            turtle.radius = oldRadius;
        } else {
            InterpretLSystemRecursive(c, 0, iterations, turtle, stack, currentSegmentIndex);
        }
    }
    
    std::cout << "Branch segments created: " << branchSegments.size() << std::endl;
    
    // Generate continuous mesh from segments
    GenerateContinuousMesh();
    
    std::cout << "Continuous mesh: " << branchVertices.size() << " vertices, "
              << branchIndices.size() / 3 << " triangles" << std::endl;
    
    // Generate leaves
    GenerateLeavesAtEndpoints();
    std::cout << "Leaf instances created: " << leafInstances.size() << std::endl;
    
    // Update buffers
    if (branchBuffersInitialized) {
        SetupBranchBuffers();
    }
    if (leafBuffersInitialized) {
        UpdateLeafInstanceBuffer();
    }
    
    std::cout << "Tree generation complete!" << std::endl;
}

void Tree::CreateVertexRing(const glm::vec3& center, const glm::vec3& direction,
                           float radius, std::vector<glm::vec3>& outVertices,
                           std::vector<glm::vec3>& outNormals) {
    // Build orthogonal basis
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right;
    
    if (abs(glm::dot(direction, up)) > 0.999f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f)));
        if (glm::length(right) < 0.01f) {
            right = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f)));
        }
    }
    
    glm::vec3 newUp = glm::cross(right, direction);
    
    // Generate ring of vertices
    for (int i = 0; i <= radialSegments; i++) {
        float theta = (float)i / radialSegments * 2.0f * glm::pi<float>();
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);
        
        glm::vec3 offset = right * cosTheta + newUp * sinTheta;
        glm::vec3 normal = glm::normalize(offset);
        glm::vec3 vertex = center + offset * radius;
        
        outVertices.push_back(vertex);
        outNormals.push_back(normal);
    }
}

void Tree::ConnectRings(int startRingIndex, int endRingIndex,
                       std::vector<unsigned int>& indices) {
    int vertsPerRing = radialSegments + 1;
    
    for (int i = 0; i < radialSegments; i++) {
        unsigned int bottomLeft = startRingIndex + i;
        unsigned int bottomRight = startRingIndex + i + 1;
        unsigned int topLeft = endRingIndex + i;
        unsigned int topRight = endRingIndex + i + 1;
        
        // First triangle
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
        indices.push_back(topLeft);
        
        // Second triangle
        indices.push_back(topLeft);
        indices.push_back(bottomRight);
        indices.push_back(topRight);
    }
}

glm::vec3 Tree::CalculateBranchColor(int depth, float radiusRatio) {
    float depthFactor = 1.0f - (depth * 0.05f);
    depthFactor = glm::clamp(depthFactor, 0.5f, 1.0f);
    
    glm::vec3 baseColor = glm::vec3(0.4f, 0.25f, 0.15f);
    return baseColor * depthFactor;
}

void Tree::GenerateContinuousMesh() {
    if (branchSegments.empty()) return;
    
    std::cout << "Building continuous mesh from " << branchSegments.size() << " segments..." << std::endl;
    
    // Store ring start indices for each segment
    std::vector<int> segmentStartRings(branchSegments.size(), -1);
    std::vector<int> segmentEndRings(branchSegments.size(), -1);
    
    // Map to track shared junction vertices (position -> vertex ring index)
    std::map<std::tuple<float, float, float>, int> junctionRings;
    
    auto positionKey = [](const glm::vec3& pos) {
        float precision = 1000.0f; // Round to 3 decimal places
        return std::make_tuple(
            std::round(pos.x * precision) / precision,
            std::round(pos.y * precision) / precision,
            std::round(pos.z * precision) / precision
        );
    };
    
    // Generate rings for each segment, reusing junction vertices
    for (size_t i = 0; i < branchSegments.size(); i++) {
        const BranchSegment& seg = branchSegments[i];
        
        glm::vec3 direction = glm::normalize(seg.endPos - seg.startPos);
        
        // Check if start position already has a ring (junction vertex sharing)
        auto startKey = positionKey(seg.startPos);
        auto startIt = junctionRings.find(startKey);
        
        if (startIt != junctionRings.end() && seg.parentIndex >= 0) {
            // Reuse parent's end ring as our start ring
            segmentStartRings[i] = startIt->second;
        } else {
            // Create new start ring
            segmentStartRings[i] = branchVertices.size() / (radialSegments + 1);
            CreateVertexRing(seg.startPos, direction, seg.startRadius, branchVertices, branchNormals);
            
            glm::vec3 startColor = CalculateBranchColor(seg.depth, seg.startRadius / initialRadius);
            for (int j = 0; j <= radialSegments; j++) {
                branchColors.push_back(startColor);
            }
            
            junctionRings[startKey] = segmentStartRings[i];
        }
        
        // Always create end ring (might be reused by children)
        segmentEndRings[i] = branchVertices.size() / (radialSegments + 1);
        CreateVertexRing(seg.endPos, direction, seg.endRadius, branchVertices, branchNormals);
        
        glm::vec3 endColor = CalculateBranchColor(seg.depth + 1, seg.endRadius / initialRadius);
        for (int j = 0; j <= radialSegments; j++) {
            branchColors.push_back(endColor);
        }
        
        // Register end ring for potential reuse
        auto endKey = positionKey(seg.endPos);
        junctionRings[endKey] = segmentEndRings[i];
    }
    
    // Connect rings within each segment
    for (size_t i = 0; i < branchSegments.size(); i++) {
        int startRing = segmentStartRings[i] * (radialSegments + 1);
        int endRing = segmentEndRings[i] * (radialSegments + 1);
        
        ConnectRings(startRing, endRing, branchIndices);
    }
    
    std::cout << "Mesh generation complete: " << branchVertices.size() << " vertices" << std::endl;
}

void Tree::GenerateLeavesAtEndpoints() {
    std::cout << "Generating leaves at endpoints..." << std::endl;
    
    for (const auto& segment : branchSegments) {
        // Only add leaves to thin branches (endpoints)
        if (segment.endRadius < initialRadius * 0.25f) {
            
            int leavesPerCluster = 5 + (rand() % 6);
            
            for (int i = 0; i < leavesPerCluster; i++) {
                float offsetDist = leafSize * 1.0f;
                glm::vec3 randomOffset(
                    RandomFloat(-offsetDist, offsetDist),
                    RandomFloat(-offsetDist, offsetDist),
                    RandomFloat(-offsetDist, offsetDist)
                );
                
                glm::vec3 leafPos = segment.endPos + randomOffset;
                
                LeafInstance instance;
                instance.position = leafPos;
                
                glm::vec3 treeCenter = this->position + glm::vec3(0.0f, initialLength * 2.0f, 0.0f);
                instance.normal = glm::normalize(leafPos - treeCenter);
                
                float scaleVariation = RandomFloat(0.9f, 1.4f);
                instance.scale = glm::vec2(leafSize * scaleVariation * 1.2f);
                
                instance.rotation = RandomFloat(0.0f, 360.0f) * glm::pi<float>() / 180.0f;
                
                float colorVariation = RandomFloat(0.85f, 1.15f);
                instance.color = glm::vec3(0.2f, 0.6f, 0.15f) * colorVariation;
                
                leafInstances.push_back(instance);
            }
        }
    }
    
    std::cout << "Generated " << leafInstances.size() << " leaves" << std::endl;
}

void Tree::UpdateLeafInstanceBuffer() {
    if (leafInstances.empty()) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, leafInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, leafInstances.size() * sizeof(LeafInstance),
                 leafInstances.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Tree::LoadLeafTexture(const std::string& texturePath) {
    if (leafTexture != 0) {
        glDeleteTextures(1, &leafTexture);
    }
    
    glGenTextures(1, &leafTexture);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        bool needsConversion = (nrChannels == 1 || nrChannels == 3);
        
        if (needsConversion) {
            int pixelCount = width * height;
            unsigned char* rgbaData = new unsigned char[pixelCount * 4];
            
            for (int i = 0; i < pixelCount; i++) {
                unsigned char value;
                
                if (nrChannels == 1) {
                    value = data[i];
                } else {
                    value = (data[i * 3] + data[i * 3 + 1] + data[i * 3 + 2]) / 3;
                }
                
                rgbaData[i * 4 + 0] = 60;
                rgbaData[i * 4 + 1] = 120;
                rgbaData[i * 4 + 2] = 40;
                rgbaData[i * 4 + 3] = value;
            }
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);
            delete[] rgbaData;
            
        } else {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Loaded leaf texture: " << texturePath << std::endl;
    } else {
        std::cerr << "Failed to load leaf texture: " << texturePath << std::endl;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Tree::Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!branchBuffersInitialized || branchVertices.empty()) return;
    
    shader.Bind();
    
    shader.setUniformMat4f("u_View", const_cast<glm::mat4&>(view));
    shader.setUniformMat4f("u_Projection", const_cast<glm::mat4&>(projection));
    
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 0.8f, -0.5f));
    shader.SetUniform3f("u_LightDir", lightDir.x, lightDir.y, lightDir.z);
    
    // Enable polygon offset to reduce Z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    
    glBindVertexArray(branchVAO);
    glDrawElements(GL_TRIANGLES, branchIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    shader.Unbind();
}


void Tree::RenderLeaves(Shader& leafShader, const glm::mat4& view, const glm::mat4& projection) {
    if (!leafBuffersInitialized || leafInstances.empty()) return;
    
    leafShader.Bind();
    
    leafShader.setUniformMat4f("u_View", view);
    leafShader.setUniformMat4f("u_Projection", projection);
    
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 0.8f, -0.5f));
    leafShader.SetUniform3f("u_LightDir", lightDir.x, lightDir.y, lightDir.z);
    
    if (leafTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leafTexture);
        leafShader.SetUniform1i("u_LeafTexture", 0);
    }
    
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindVertexArray(leafVAO);
    glDrawElementsInstanced(GL_TRIANGLES, leafQuadIndices.size(), GL_UNSIGNED_INT, 0, 
                           leafInstances.size());
    glBindVertexArray(0);
    
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    
    leafShader.Unbind();
}

void Tree::Clean() {
    if (branchBuffersInitialized) {
        glDeleteVertexArrays(1, &branchVAO);
        glDeleteBuffers(1, &branchVBO);
        glDeleteBuffers(1, &branchNBO);
        glDeleteBuffers(1, &branchCBO);
        glDeleteBuffers(1, &branchEBO);
        branchBuffersInitialized = false;
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
    
    branchSegments.clear();
    branchVertices.clear();
    branchNormals.clear();
    branchColors.clear();
    branchIndices.clear();
    leafInstances.clear();
    leafQuadVertices.clear();
}