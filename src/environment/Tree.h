#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <tuple>
#include "Shader.h"

struct LeafInstance {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 scale;
    float rotation;
    glm::vec3 color;
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

// Structure to represent a branch segment in the tree
struct BranchSegment {
    glm::vec3 startPos;
    glm::vec3 endPos;
    float startRadius;
    float endRadius;
    int depth;
    int parentIndex;  // -1 for root, otherwise index of parent segment
    std::vector<int> childIndices;
};

class Tree {
public:
    Tree();
    ~Tree();

    void Init(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));
    void Generate(int iterations = 4);
    void Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection);
    void RenderLeaves(Shader& leafShader, const glm::mat4& view, const glm::mat4& projection);
    void Clean();
    
    // Setters
    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetAngle(float angle) { branchAngle = angle; }
    void SetLengthScale(float scale) { lengthScale = scale; }
    void SetRadiusScale(float scale) { radiusScale = scale; }
    void SetInitialLength(float length) { initialLength = length; }
    void SetInitialRadius(float radius) { initialRadius = radius; }
    void SetAxiom(const std::string& axiom) { this->axiom = axiom; }
    void AddRule(char symbol, const std::string& replacement);
    void SetLeafSize(float size) { leafSize = size; }
    void SetLeafDensity(float density) { leafDensity = density; }
    void SetMinLeafDepth(int depth) { minLeafDepth = depth; }
    void SetRadialSegments(int segments) { radialSegments = segments; }
    
    // Getters
    int GetBranchCount() const { return branchSegments.size(); }
    int GetLeafCount() const { return leafInstances.size(); }
    
    // Texture
    void LoadLeafTexture(const std::string& texturePath);
    GLuint GetLeafTexture() const { return leafTexture; }
    
private:
    // L-System interpretation
    void InterpretLSystemRecursive(char symbol, int depth, int maxDepth,
                                   TurtleState& turtle, std::stack<TurtleState>& stack,
                                   int currentSegmentIndex);
    void InterpretSymbol(char c, TurtleState& turtle, std::stack<TurtleState>& stack,
                        int& currentSegmentIndex);
    
    // Continuous mesh generation
    void GenerateContinuousMesh();
    void CreateVertexRing(const glm::vec3& center, const glm::vec3& direction,
                         float radius, std::vector<glm::vec3>& outVertices,
                         std::vector<glm::vec3>& outNormals);
    void ConnectRings(int startRingIndex, int endRingIndex,
                     std::vector<unsigned int>& indices);
    glm::vec3 CalculateBranchColor(int depth, float radiusRatio);
    
    // Leaf generation
    void GenerateLeavesAtEndpoints();
    void CreateLeafQuadTemplate();
    void UpdateLeafInstanceBuffer();
    void SetupLeafBuffers();
    
    // OpenGL setup
    void SetupBranchBuffers();
    
    // L-System parameters
    std::string axiom;
    std::map<char, std::string> rules;
    
    // Tree parameters
    glm::vec3 position;
    float branchAngle;
    float lengthScale;
    float radiusScale;
    float initialLength;
    float initialRadius;
    int radialSegments;
    friend class Renderer;
    // Leaf parameters
    float leafSize;
    float leafDensity;
    int minLeafDepth;
    GLuint leafTexture;
    
    // Branch structure
    std::vector<BranchSegment> branchSegments;
    
    // Continuous mesh data
    std::vector<glm::vec3> branchVertices;
    std::vector<glm::vec3> branchNormals;
    std::vector<glm::vec3> branchColors;
    std::vector<unsigned int> branchIndices;
    
    // Leaf data
    std::vector<glm::vec3> leafQuadVertices;
    std::vector<glm::vec2> leafQuadUVs;
    std::vector<unsigned int> leafQuadIndices;
    std::vector<LeafInstance> leafInstances;
    
    // OpenGL objects for branches
    GLuint branchVAO, branchVBO, branchNBO, branchCBO, branchEBO;
    bool branchBuffersInitialized;
    
    // OpenGL objects for leaves
    GLuint leafVAO, leafVBO, leafUVBO, leafEBO, leafInstanceVBO;
    bool leafBuffersInitialized;
    
    // Stack indices for branching (used during generation)
    std::stack<int> segmentIndexStack;
};