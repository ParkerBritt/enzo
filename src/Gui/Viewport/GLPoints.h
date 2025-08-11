#pragma once
#include "Engine/Types.h"
#include <GL/gl.h>
#include <QOpenGLFunctions_3_3_Core>
#include <glm/ext/vector_float3.hpp>
#include <qopenglversionfunctions.h>
#include "Engine/Operator/Geometry.h"
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
    void setPoints(enzo::geo::Geometry& geometry, GLCamera& camera);
    void updatePointSize(GLCamera& camera);
    void useProgram();
    void bind();
    void unbind();
    void draw();
};
