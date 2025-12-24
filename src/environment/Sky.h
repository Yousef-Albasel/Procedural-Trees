// Sky.h
#pragma once
#include <GL/glew.h>   // or your GL loader
#include <glm/glm.hpp>

class Shader; // forward

class Sky {
public:
    Sky();
    ~Sky();

    void Init();
    void Update(float dt);
    void Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& sunDir);

private:
    void GenerateCloudTexture();

    GLuint VAO;
    GLuint VBO;
    GLuint cloudTexture;
    float time;
};
