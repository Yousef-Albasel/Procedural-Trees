#include "Renderer.h"
#include "imgui/imgui.h"
#include <filesystem>
#include <exception>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>

Renderer::Renderer() {
    camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 2.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    sky = std::make_unique<Sky>();
    tree = std::make_unique<Tree>();
    
    for (int i = 0; i < MAX_RULES; i++) {
        ruleSymbols[i] = 'A' + i;
        strcpy(ruleReplacements[i], "");
        ruleEnabled[i] = false;
    }
    
    // Set default rule
    strcpy(axiomInputBuffer, "X");
    ruleSymbols[0] = 'X';
    strcpy(ruleReplacements[0], "FTF[+XL][-XL][&XL][^XXL]FXL");
    ruleEnabled[0] = true;
    
    // Load presets from file
    LoadPresetsFromFile();
}

Renderer::~Renderer() {
    Clean();
}

void Renderer::Init() {
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    
    // Initialize sky
    sky->Init();
    skyShader = new Shader("../src/res/shaders/sky.shader");
    
    // Initialize tree
    tree->Init(glm::vec3(0.0f, 0.0f, 0.0f));
    tree->SetAngle(treeBranchAngle);
    tree->SetLengthScale(treeLengthScale);
    tree->SetRadiusScale(treeRadiusScale);
    
    // Set randomness parameters (NEW)
    tree->SetAngleRandomness(0.15f);      // 15% angle variation
    tree->SetLengthRandomness(0.1f);      // 10% length variation
    tree->SetTropism(glm::vec3(0.0f, -0.2f, 0.0f));  // Slight downward bias (gravity)
    tree->SetBranchProbability(1.0f);     // 100% branch probability
    
    // Set leaf parameters
    tree->SetLeafSize(leafSize);
    tree->SetLeafDensity(leafDensity);
    tree->SetMinLeafDepth(minLeafDepth);
    
    // Load leaf texture (black background, white leaf silhouette)
    tree->LoadLeafTexture("../src/res/leaves.jpg");
    
    ApplyCurrentRules();
    tree->Generate(treeIterations);
    
    treeShader = new Shader("../src/res/shaders/tree.shader");
    leafShader = new Shader("../src/res/shaders/leaf.shader");
    
    std::cout << "Renderer initialized successfully" << std::endl;
}

void Renderer::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    static float lastTime = 0.0f;
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    
    const float maxDeltaTime = 0.1f;
    if (deltaTime > maxDeltaTime) {
        deltaTime = maxDeltaTime;
    }
    
    sky->Update(deltaTime);
    
    // Check if tree needs regeneration
    if (treeNeedsRegeneration) {
        tree->SetAngle(treeBranchAngle);
        tree->SetLengthScale(treeLengthScale);
        tree->SetRadiusScale(treeRadiusScale);
        tree->SetLeafSize(leafSize);
        tree->SetLeafDensity(leafDensity);
        tree->SetMinLeafDepth(minLeafDepth);
        ApplyCurrentRules();
        tree->Generate(treeIterations);
        treeNeedsRegeneration = false;
    }
    
    // Setup matrices
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
    glm::mat4 view = camera->getViewMatrix();
    glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, 0.8f, -0.5f));
    glm::vec3 sunColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 cameraPosition = camera->getCameraPos();

    // Render sky
    sky->Render(*skyShader, view, projection, sunDirection);    
    
    // Render tree branches
    if (treeShader) {
        tree->Render(*treeShader, view, projection);
    }
    
    // Render leaves
    if (leafShader && renderLeaves) {
        tree->RenderLeaves(*leafShader, view, projection);
    }
}

void Renderer::Clean() {
    if (tree) {
        tree->Clean();
    }
    
    if (skyShader) {
        delete skyShader;
        skyShader = nullptr;
    }
    
    if (treeShader) {
        delete treeShader;
        treeShader = nullptr;
    }
    
    if (leafShader) {
        delete leafShader;
        leafShader = nullptr;
    }
}

void Renderer::ApplyCurrentRules() {
    tree->rules.clear();
    tree->SetAxiom(std::string(axiomInputBuffer));
    
    for (int i = 0; i < MAX_RULES; i++) {
        if (ruleEnabled[i] && strlen(ruleReplacements[i]) > 0) {
            tree->AddRule(ruleSymbols[i], std::string(ruleReplacements[i]));
            std::cout << "Applied rule: " << ruleSymbols[i] << " -> " << ruleReplacements[i] << std::endl;
        }
    }
}

void Renderer::SavePresetToFile() {
    std::ofstream file("../src/res/presets.txt", std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open presets.txt for writing" << std::endl;
        return;
    }
    
    file << "[PRESET]" << std::endl;
    file << "Name=" << presetNameBuffer << std::endl;
    file << "Axiom=" << axiomInputBuffer << std::endl;
    file << "Iterations=" << treeIterations << std::endl;
    file << "BranchAngle=" << treeBranchAngle << std::endl;
    file << "LengthScale=" << treeLengthScale << std::endl;
    file << "RadiusScale=" << treeRadiusScale << std::endl;
    file << "LeafSize=" << leafSize << std::endl;
    file << "LeafDensity=" << leafDensity << std::endl;
    file << "MinLeafDepth=" << minLeafDepth << std::endl;
    
    for (int i = 0; i < MAX_RULES; i++) {
        if (ruleEnabled[i] && strlen(ruleReplacements[i]) > 0) {
            file << "Rule=" << ruleSymbols[i] << ":" << ruleReplacements[i] << std::endl;
        }
    }
    
    file << "[END]" << std::endl << std::endl;
    file.close();
    
    std::cout << "Preset saved: " << presetNameBuffer << std::endl;
    
    // Reload presets to update the list
    LoadPresetsFromFile();
}

void Renderer::LoadPresetsFromFile() {
    presets.clear();
    
    std::ifstream file("../src/res/presets.txt");
    if (!file.is_open()) {
        std::cout << "No presets.txt file found, starting with empty preset list" << std::endl;
        return;
    }
    
    TreePreset currentPreset;
    bool readingPreset = false;
    std::string line;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        if (line == "[PRESET]") {
            readingPreset = true;
            currentPreset = TreePreset(); // Reset
            currentPreset.rules.clear();
            continue;
        }
        
        if (line == "[END]") {
            if (readingPreset) {
                presets.push_back(currentPreset);
                readingPreset = false;
            }
            continue;
        }
        
        if (readingPreset) {
            size_t delimPos = line.find('=');
            if (delimPos != std::string::npos) {
                std::string key = line.substr(0, delimPos);
                std::string value = line.substr(delimPos + 1);
                
                if (key == "Name") {
                    currentPreset.name = value;
                } else if (key == "Axiom") {
                    currentPreset.axiom = value;
                } else if (key == "Iterations") {
                    currentPreset.iterations = std::stoi(value);
                } else if (key == "BranchAngle") {
                    currentPreset.branchAngle = std::stof(value);
                } else if (key == "LengthScale") {
                    currentPreset.lengthScale = std::stof(value);
                } else if (key == "RadiusScale") {
                    currentPreset.radiusScale = std::stof(value);
                } else if (key == "LeafSize") {
                    currentPreset.leafSize = std::stof(value);
                } else if (key == "LeafDensity") {
                    currentPreset.leafDensity = std::stof(value);
                } else if (key == "MinLeafDepth") {
                    currentPreset.minLeafDepth = std::stoi(value);
                } else if (key == "Rule") {
                    size_t colonPos = value.find(':');
                    if (colonPos != std::string::npos && colonPos > 0) {
                        char symbol = value[0];
                        std::string replacement = value.substr(colonPos + 1);
                        currentPreset.rules.push_back({symbol, replacement});
                    }
                }
            }
        }
    }
    
    file.close();
    std::cout << "Loaded " << presets.size() << " presets from file" << std::endl;
}

void Renderer::ApplyPreset(const TreePreset& preset) {
    strcpy(axiomInputBuffer, preset.axiom.c_str());
    treeIterations = preset.iterations;
    treeBranchAngle = preset.branchAngle;
    treeLengthScale = preset.lengthScale;
    treeRadiusScale = preset.radiusScale;
    leafSize = preset.leafSize;
    leafDensity = preset.leafDensity;
    minLeafDepth = preset.minLeafDepth;
    
    // Clear all rules
    for (int i = 0; i < MAX_RULES; i++) {
        ruleEnabled[i] = false;
        strcpy(ruleReplacements[i], "");
    }
    
    // Apply preset rules
    for (size_t i = 0; i < preset.rules.size() && i < MAX_RULES; i++) {
        ruleSymbols[i] = preset.rules[i].first;
        strcpy(ruleReplacements[i], preset.rules[i].second.c_str());
        ruleEnabled[i] = true;
    }
    
    treeNeedsRegeneration = true;
}

void Renderer::processKeyboardInput(GLFWwindow* window, float deltaTime) {
    float adjustedDeltaTime = deltaTime * (movementSpeed / 50.0f);

    static bool f1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        showDebugWindow = !showDebugWindow;
        f1Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
    }

    if (showDebugWindow) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera->updateKeyboardInput(adjustedDeltaTime, 0);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera->updateKeyboardInput(adjustedDeltaTime, 1);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera->updateKeyboardInput(adjustedDeltaTime, 3);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera->updateKeyboardInput(adjustedDeltaTime, 2);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera->updateKeyboardInput(adjustedDeltaTime, 4);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera->updateKeyboardInput(adjustedDeltaTime, 5);
        }

        static bool f3Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed) {
            enableMouseLook = !enableMouseLook;
            firstMouse = true;
            f3Pressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
            f3Pressed = false;
        }

        if (enableMouseLook) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glm::vec3 forward = glm::normalize(glm::vec3(camera->getCameraFront().x, 0.0f, camera->getCameraFront().z));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        glm::vec3 moveDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            moveDir += forward;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            moveDir -= forward;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            moveDir -= right;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            moveDir += right;
        }
    }
}

void Renderer::ProcessMouseInput(GLFWwindow* window, const float& dt) {
    glfwGetCursorPos(window, &mouseX, &mouseY);

    float xpos = static_cast<float>(mouseX);
    float ypos = static_cast<float>(mouseY);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (showDebugWindow) {
        if (enableMouseLook) {
            camera->updateMouseInput(dt, xoffset * mouseSensitivity, yoffset * mouseSensitivity);
        }
    }
    else {
        camera->updateMouseInput(dt, xoffset * mouseSensitivity, yoffset * mouseSensitivity);
    }
}

void Renderer::RenderDebugUI(float deltaTime) {
    if (!showDebugWindow) return;

    frameCount++;
    fpsTimer += deltaTime;
    if (fpsTimer >= 1.0f) {
        currentFPS = frameCount / fpsTimer;
        frameCount = 0;
        fpsTimer = 0.0f;
    }

    ImGui::Begin("Debug Info", &showDebugWindow);
    
    ImGui::Text("Press F1 to toggle this window");
    ImGui::Separator();

    ImGui::Text("FPS: %.1f", currentFPS);
    ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec3 pos = camera->getCameraPos();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        
        glm::vec3 front = camera->getCameraFront();
        ImGui::Text("Front: (%.2f, %.2f, %.2f)", front.x, front.y, front.z);
        
        ImGui::Separator();
        
        if (ImGui::Checkbox("Enable Mouse Look", &enableMouseLook)) {
            firstMouse = true;
        }
        
        ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 1.0f);
        ImGui::SliderFloat("Movement Speed", &movementSpeed, 10.0f, 200.0f);
    }

  if (ImGui::CollapsingHeader("Tree L-System", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("L-System Parameters:");
    
    bool changed = false;
    changed |= ImGui::SliderInt("Iterations", &treeIterations, 1, 8);
    changed |= ImGui::SliderFloat("Branch Angle", &treeBranchAngle, 10.0f, 45.0f, "%.1f deg");
    changed |= ImGui::SliderFloat("Length Scale", &treeLengthScale, 0.5f, 0.95f, "%.2f");
    changed |= ImGui::SliderFloat("Radius Scale", &treeRadiusScale, 0.5f, 0.95f, "%.2f");
    
    ImGui::Separator();
    ImGui::Text("Randomness Parameters:");
        // Get current values from tree
    float angleRand = tree->GetAngleRandomness();
    float lengthRand = tree->GetLengthRandomness();
    glm::vec3 tropism = tree->GetTropism();
    float branchProb = tree->GetBranchProbability();
    
    if (ImGui::SliderFloat("Angle Randomness", &angleRand, 0.0f, 0.5f, "%.2f")) {
        tree->SetAngleRandomness(angleRand);
        changed = true;
    }
    if (ImGui::SliderFloat("Length Randomness", &lengthRand, 0.0f, 0.3f, "%.2f")) {
        tree->SetLengthRandomness(lengthRand);
        changed = true;
    }
    
    ImGui::Separator();
    ImGui::Text("Tropism (Directional Bias):");
    if (ImGui::SliderFloat3("Tropism Vector", &tropism.x, -1.0f, 1.0f, "%.2f")) {
        tree->SetTropism(tropism);
        changed = true;
    }
    ImGui::TextDisabled("(e.g., (0, -0.2, 0) for gravity effect)");
    
    if (ImGui::SliderFloat("Branch Probability", &branchProb, 0.5f, 1.0f, "%.2f")) {
        tree->SetBranchProbability(branchProb);
        changed = true;
    }
    ImGui::TextDisabled("(lower = sparser tree)");
    
    ImGui::Separator();
    ImGui::Text("Leaf Parameters:");
    
    if (ImGui::Checkbox("Render Leaves", &renderLeaves)) {
        // Just toggle rendering, no regeneration needed
    }
    
    changed |= ImGui::SliderFloat("Leaf Size", &leafSize, 0.1f, 1.0f, "%.2f");
    changed |= ImGui::SliderFloat("Leaf Density", &leafDensity, 0.0f, 1.0f, "%.2f");
    changed |= ImGui::SliderInt("Min Leaf Depth", &minLeafDepth, 0, 6);
    
    if (changed) {
        treeNeedsRegeneration = true;
    }
    
    ImGui::Separator();
    ImGui::Text("Axiom & Rules:");
    ImGui::TextDisabled("You can now use parameterized F segments:");
    ImGui::BulletText("F - default length and radius");
    ImGui::BulletText("F(2) - double length, default radius");
    ImGui::BulletText("F(2,0.5) - double length, half radius");
    ImGui::BulletText("F(0.5,1.5) - half length, 1.5x radius");
    
    ImGui::Separator();
    
    ImGui::Text("Axiom:");
    ImGui::SameLine();
        if (ImGui::InputText("##Axiom", axiomInputBuffer, sizeof(axiomInputBuffer))) {
        }
        
        ImGui::Separator();
        ImGui::Text("Production Rules:");
        
        for (int i = 0; i < MAX_RULES; i++) {
            ImGui::PushID(i);
            
            ImGui::Checkbox("##Enable", &ruleEnabled[i]);
            ImGui::SameLine();
            
            char symbolStr[2] = {ruleSymbols[i], '\0'};
            ImGui::SetNextItemWidth(30);
            if (ImGui::InputText("##Symbol", symbolStr, 2)) {
                if (symbolStr[0] != '\0') {
                    ruleSymbols[i] = symbolStr[0];
                }
            }
            
            ImGui::SameLine();
            ImGui::Text("->");
            ImGui::SameLine();
            
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("##Replacement", ruleReplacements[i], sizeof(ruleReplacements[i]));
            
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Apply Rules & Regenerate", ImVec2(-1, 0))) {
            treeNeedsRegeneration = true;
        }
        
        ImGui::Separator();
        ImGui::Text("Save Current Configuration:");
        ImGui::InputText("Preset Name", presetNameBuffer, sizeof(presetNameBuffer));
        if (ImGui::Button("Save Preset", ImVec2(-1, 0))) {
            if (strlen(presetNameBuffer) > 0) {
                SavePresetToFile();
                strcpy(presetNameBuffer, ""); // Clear the name buffer
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Load Preset:");
        
        if (ImGui::Button("Refresh Preset List", ImVec2(-1, 0))) {
            LoadPresetsFromFile();
        }
        
        for (size_t i = 0; i < presets.size(); i++) {
            if (ImGui::Button(presets[i].name.c_str(), ImVec2(-1, 0))) {
                ApplyPreset(presets[i]);
            }
        }
        
        if (presets.empty()) {
            ImGui::TextDisabled("No presets loaded. Save a preset to get started!");
        }
        
        ImGui::Separator();
        ImGui::Text("Symbol Reference:");
        ImGui::BulletText("F, X - Forward (F draws, X doesn't)");
        ImGui::BulletText("L - Leaf (spawns a leaf at current position)");
        ImGui::BulletText("+ - Yaw right, - - Yaw left");
        ImGui::BulletText("& - Pitch down, ^ - Pitch up");
        ImGui::BulletText("\\ - Roll left, / - Roll right");
        ImGui::BulletText("[ - Save state, ] - Restore state");
        
        ImGui::Separator();
        ImGui::Text("Statistics:");
        ImGui::Text("Branches: %d", tree->GetBranchCount());
        ImGui::Text("Leaves: %d", tree->GetLeafCount());
    }

    if (ImGui::CollapsingHeader("Controls")) {
        ImGui::Text("Camera:");
        ImGui::BulletText("WASD - Move");
        ImGui::BulletText("Space - Move Up");
        ImGui::BulletText("Shift - Move Down");
        ImGui::BulletText("Mouse - Look Around");
        ImGui::Separator();
        ImGui::Text("Interface:");
        ImGui::BulletText("F1 - Toggle Debug Window");
        ImGui::BulletText("F3 - Toggle Mouse Look");
    }

    if (ImGui::CollapsingHeader("System")) {
        ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
        ImGui::Text("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    }

    ImGui::End();
}