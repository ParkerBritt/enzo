#pragma once
#include "Engine/Types.h"
#include <GL/gl.h>
#include <QOpenGLFunctions_3_2_Core>
#include <glm/ext/vector_float3.hpp>
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/NodePacket.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

class GLMesh
: protected QOpenGLFunctions_3_2_Core
{
public:
    GLMesh();
    GLuint vao;
    GLuint vertexBuffer;
    GLuint faceIndexBuffer;
    GLuint lineIndexBuffer;

    // std::vector<GLfloat> vertexPosData;
    std::vector<Vertex> vertices;
    std::vector<GLint> faceIndexData;
    std::vector<GLint> lineIndexData;

    void init();
    void initBuffers();
    void setPosBuffer(const enzo::NodePacket& packet);
    void setIndexBuffer(const enzo::NodePacket& packet);
    void bind();
    void unbind();
    void draw();
};
