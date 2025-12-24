#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Shader.h"
#include "stb_image.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Tex {
    unsigned int id;
    std::string type;
    std::string path;
};

class ModelMesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Tex> textures;
    
    unsigned int VAO, VBO, EBO;
    
    ModelMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Tex> textures);
    void Draw(Shader& shader);
    
private:
    void setupMesh();
};

class Model {
public:
    std::vector<ModelMesh> meshes;
    std::string directory;
    std::vector<Tex> textures_loaded;
    
    Model(const std::string& path);
    Model(const std::string& path, const glm::vec3& rotationEulerDeg);

    void Draw(Shader& shader);
    glm::mat4 GetCorrectionTransform() const { return correctionTransform; }

private:
    glm::mat4 correctionTransform = glm::mat4(1.0f);
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    ModelMesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Tex> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
    unsigned int TextureFromFile(const char* path, const std::string& directory);
};