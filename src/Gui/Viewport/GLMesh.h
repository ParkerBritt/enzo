#pragma once
#include "Engine/Types.h"
#include <GL/gl.h>
#include <QOpenGLFunctions_3_2_Core>
#include <glm/ext/vector_float3.hpp>
#include "Engine/Operator/Primitive.h"

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
    void setPosBuffer(enzo::geo::Primitive& geometry);
    void setIndexBuffer(enzo::geo::Primitive& geometry);
    void bind();
    void unbind();
    void draw();
};
