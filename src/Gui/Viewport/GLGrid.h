#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <glm/ext/vector_float3.hpp>

class GLGrid : protected QOpenGLFunctions_3_3_Core
{
  public:
    GLGrid();
    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;
    std::vector<glm::vec3> vertices;

    void init();
    void initBuffers();
    void initShaderProgram();
    void useProgram();
    void bind();
    void unbind();
    void draw();
};
