#pragma once

#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Camera.h"
#include "Shader.h"
#include "Sky.h"
#include "Tree.h"

struct TreePreset {
    std::string name;
    std::string axiom;
    int iterations;
    float branchAngle;
    float lengthScale;
    float radiusScale;
    float leafSize;
    float leafDensity;
    float divergenceAngle1;  
    float divergenceAngle2;  
    int minLeafDepth;
    std::vector<std::pair<char, std::string>> rules; // symbol, replacement
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    void Init();
    void Render();
    void Clean();
    
    void processKeyboardInput(GLFWwindow* window, float deltaTime);
    void ProcessMouseInput(GLFWwindow* window, const float& dt);
    void RenderDebugUI(float deltaTime);
    void SavePresetToFile();
    void LoadPresetsFromFile();
    void ApplyPreset(const TreePreset& preset);
    
private:
    // Core components
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Sky> sky;
    std::unique_ptr<Tree> tree;
    
    // Shaders
    Shader* skyShader = nullptr;
    Shader* treeShader = nullptr;
    Shader* leafShader = nullptr;
    
    // Camera controls
    bool showDebugWindow = true;
    bool enableMouseLook = false;
    bool firstMouse = true;
    double mouseX = 0.0, mouseY = 0.0;
    float lastX = 640.0f, lastY = 360.0f;
    float mouseSensitivity = 0.15f;
    float movementSpeed = 50.0f;
    
    // FPS counter
    int frameCount = 0;
    float fpsTimer = 0.0f;
    float currentFPS = 0.0f;
    
    // Tree L-System parameters
    int treeIterations = 4;
    float treeBranchAngle = 25.0f;
    float treeLengthScale = 0.90f;
    float treeRadiusScale = 0.88f;
    bool treeNeedsRegeneration = false;
    
    // Leaf parameters
    bool renderLeaves = true;
    float leafSize = 0.3f;
    float leafDensity = 0.7f;
    int minLeafDepth = 3;
    float treeDivergenceAngle1 = 137.5f;  // Golden angle
    float treeDivergenceAngle2 = 90.0f;   // Secondary divergence
    // L-System UI
    static const int MAX_RULES = 8;
    char axiomInputBuffer[256] = "F";
    char ruleSymbols[MAX_RULES];
    char ruleReplacements[MAX_RULES][256];
    bool ruleEnabled[MAX_RULES];
    int selectedPreset = 0;
    std::vector<TreePreset> presets;
    char presetNameBuffer[256] = "";
    
    void ApplyCurrentRules();
    void ApplyTreePreset(int presetIndex);

};