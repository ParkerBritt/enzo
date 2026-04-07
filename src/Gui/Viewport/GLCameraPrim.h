#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <glm/ext/vector_float3.hpp>
#include "Engine/Operator/NodePacket.h"

class GLCameraPrim
: protected QOpenGLFunctions_3_3_Core
{
public:
    GLCameraPrim();
    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;

    void init();
    void initBuffers();
    void initShaderProgram();
    void useProgram();
    void bind();
    void unbind();
    void draw();

    void setCameras(enzo::NodePacket& packet);

private:
    size_t instanceCount_ = 0;
    GLuint instanceVbo_;
};
