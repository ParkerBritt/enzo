#pragma once
#include "Engine/Types.h"
#if defined(__linux__) || defined(_WIN32)
#include <GL/gl.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include <QOpenGLFunctions_3_2_Core>
#include <glm/ext/vector_float3.hpp>
#include "Engine/Operator/Geometry.h"

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
    void setPosBuffer(enzo::geo::Geometry& geometry);
    void setIndexBuffer(enzo::geo::Geometry& geometry);
    void bind();
    void unbind();
    void draw();
};
