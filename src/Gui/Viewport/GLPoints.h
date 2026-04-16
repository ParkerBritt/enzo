#pragma once
#include "Engine/Types.h"
#if defined(__linux__) || defined(_WIN32)
#include <GL/gl.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include <QOpenGLFunctions_3_3_Core>
#include <glm/ext/vector_float3.hpp>
#include <qopenglversionfunctions.h>
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/NodePacket.h"
#include "Gui/Viewport/GLCamera.h"

struct Point
{
    glm::vec3 position;
    float scale;
};

class GLPoints
: protected QOpenGLFunctions_3_3_Core
{
public:
    GLPoints();
    GLuint vao;

    GLuint billboardVertexBuffer_;
    GLuint pointDataBuffer_;
    GLuint shaderProgram;



    std::vector<GLint> faceIndexData;
    std::vector<Point> points_;

    enzo::ga::Offset pointCount;

    void init();
    void initBuffers();
    void initShaderProgram();
    void setPoints(const enzo::NodePacket& packet, GLCamera& camera);
    void updatePointSize(GLCamera& camera);
    void useProgram();
    void bind();
    void unbind();
    void draw();
};
