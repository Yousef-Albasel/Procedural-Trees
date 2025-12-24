#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <map>
#include <stack>
#include "Shader.h"

struct BranchInstance {
    glm::mat4 transform;
    glm::vec3 color;
    float thickness;
};

struct LeafInstance {
    glm::vec3 position;
    glm::vec3 normal;      // Spherical normal for better lighting
    glm::vec2 scale;       // XY scale (usually uniform)
    float rotation;        // Rotation around normal
    glm::vec3 color;       // Color tint
};

struct TreeNode {
    glm::vec3 position;
    glm::vec3 direction;
    float radius;
    float length;
    int depth;
};

struct TurtleState {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;
    float length;
    float radius;
    int depth;
};

class Tree {
public:
    Tree();
    ~Tree();

    // Initialize the tree with L-System parameters
    void Init(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));
    
    // Generate tree using L-System rules
    void Generate(int iterations = 4);
    
    // Render the tree using GPU instancing
    void Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection);
    
    // Render leaves separately with their own shader
    void RenderLeaves(Shader& leafShader, const glm::mat4& view, const glm::mat4& projection);
    
    // Cleanup
    void Clean();
    
    // Setters for customization
    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetAngle(float angle) { branchAngle = angle; }
    void SetLengthScale(float scale) { lengthScale = scale; }
    void SetRadiusScale(float scale) { radiusScale = scale; }
    void SetInitialLength(float length) { initialLength = length; }
    void SetInitialRadius(float radius) { initialRadius = radius; }
    void SetAxiom(const std::string& axiom) { this->axiom = axiom; }
    void AddRule(char symbol, const std::string& replacement);
    
    // Leaf settings
    void SetLeafSize(float size) { leafSize = size; }
    void SetLeafDensity(float density) { leafDensity = density; }
    void SetMinLeafDepth(int depth) { minLeafDepth = depth; }
    
    // Get statistics
    int GetBranchCount() const { return branchCount; }
    int GetInstanceCount() const { return branchInstances.size(); }
    int GetLeafCount() const { return leafInstances.size(); }
    
    // Leaf texture management
    void LoadLeafTexture(const std::string& texturePath);
    GLuint GetLeafTexture() const { return leafTexture; }
    
private:
    // L-System generation
    std::string ApplyRules(const std::string& current, int iterations);
    void InterpretLSystem(const std::string& lsystem);
    
    // OPTIMIZED: Direct recursive interpretation (no string storage)
    void InterpretLSystemRecursive(char symbol, int depth, int maxDepth,
                                   TurtleState& turtle, std::stack<TurtleState>& stack);
    void InterpretSymbol(char c, TurtleState& turtle, std::stack<TurtleState>& stack);
    
    // GPU Instancing for branches
    void CreateCylinderTemplate();
    void GenerateBranchInstance(const glm::vec3& start, const glm::vec3& end,
                                float startRadius, float endRadius, int depth);
    void UpdateInstanceBuffer();
    // Leaf generation and rendering
    void CreateLeafQuadTemplate();
    void GenerateLeafInstance(const glm::vec3& position, const glm::vec3& direction, 
                              const TurtleState& turtle);
    void UpdateLeafInstanceBuffer();
    void SetupLeafBuffers();
    
    void Tree::GenerateLeavesAtEndpoints();
    // Setup OpenGL buffers
    void SetupBuffers();
    
    // L-System parameters
    std::string axiom;
    std::map<char, std::string> rules;
    std::string currentLSystem;
    
    friend class Renderer;
    
    // Tree generation parameters
    glm::vec3 position;
    float branchAngle;
    float lengthScale;
    float radiusScale;
    float initialLength;
    float initialRadius;
    int radialSegments;
    
    // Leaf parameters
    float leafSize;
    float leafDensity;      // 0-1, probability of leaf generation
    int minLeafDepth;       // Minimum depth before leaves appear
    GLuint leafTexture;
    
    // Template cylinder mesh (shared by all branch instances)
    std::vector<glm::vec3> templateVertices;
    std::vector<glm::vec3> templateNormals;
    std::vector<unsigned int> templateIndices;
    
    // Template quad mesh for leaves
    std::vector<glm::vec3> leafQuadVertices;
    std::vector<glm::vec2> leafQuadUVs;
    std::vector<unsigned int> leafQuadIndices;
    
    // Instance data
    std::vector<BranchInstance> branchInstances;
    std::vector<LeafInstance> leafInstances;
    
    // Rendering state stack for L-System interpretation
    std::vector<TurtleState> stateStack;
    
    // OpenGL objects for branches
    GLuint VAO, VBO, NBO, EBO;
    GLuint instanceVBO;
    bool buffersInitialized;
    
    // OpenGL objects for leaves
    GLuint leafVAO, leafVBO, leafUVBO, leafEBO;
    GLuint leafInstanceVBO;
    bool leafBuffersInitialized;
    
    // Statistics
    int branchCount;
    int templateVertexCount;
};