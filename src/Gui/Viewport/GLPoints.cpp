
#include "Gui/Viewport/GLPoints.h"
#include <GL/gl.h>
#include <glm/fwd.hpp>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <glm/geometric.hpp>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include "Engine/Types.h"
#include "Gui/Viewport/GLCamera.h"
#include "icecream.hpp"


GLPoints::GLPoints()
{
    initializeOpenGLFunctions();
    init();
}

void GLPoints::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    initBuffers();
    initShaderProgram();

    // unbind vertex array
    glBindVertexArray(0);
}

void GLPoints::initBuffers()
{
    const GLfloat billboardVertexPositions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
    };

    points_ = {};
    pointCount = points_.size();

    glGenBuffers(1, &billboardVertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, billboardVertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(billboardVertexPositions), billboardVertexPositions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &pointDataBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, pointDataBuffer_);
    glBufferData(GL_ARRAY_BUFFER, points_.size() * sizeof(Point), points_.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glVertexAttribDivisor(1, 1);
}


void GLPoints::setPoints(enzo::geo::Geometry& geometry, GLCamera& camera)
{
    const enzo::ga::Offset numPoints =  geometry.getNumSoloPoints();
    const glm::vec3 camPosGlm = camera.getPos();
    const enzo::bt::Vector3 camPos(camPosGlm.x, camPosGlm.y, camPosGlm.z);

    points_ = std::vector<Point>(numPoints);

    std::vector<enzo::ga::Offset> soloPoints = {geometry.soloPointsBegin(), geometry.soloPointsEnd()};

    tbb::parallel_for(tbb::blocked_range<enzo::ga::Offset>(0, numPoints), [&](tbb::blocked_range<enzo::ga::Offset> range)
    {
        for(enzo::ga::Offset i=range.begin(); i<range.end(); ++i)
        {
            const enzo::ga::Offset ptOffset = soloPoints[i];
            const enzo::bt::Vector3 pos = geometry.getPointPos(ptOffset);
            points_[i] = {glm::vec3(pos.x(), pos.y(), pos.z()), static_cast<float>((pos-camPos).norm())*0.005f};
        }
    });
    pointCount = points_.size();

    glBindBuffer(GL_ARRAY_BUFFER, pointDataBuffer_);
    glBufferData(GL_ARRAY_BUFFER, points_.size() * sizeof(Point), points_.data(), GL_STATIC_DRAW);
}

void GLPoints::updatePointSize(GLCamera& camera)
{
    const glm::vec3 camPosGlm = camera.getPos();

    for(Point& point : points_)
    {
        point = {point.position, glm::distance(point.position,camPosGlm)*0.005f};
    }

    glBindBuffer(GL_ARRAY_BUFFER, pointDataBuffer_);
    glBufferData(GL_ARRAY_BUFFER, points_.size() * sizeof(Point), points_.data(), GL_STATIC_DRAW);
}



void GLPoints::initShaderProgram()
{
    // vertex shader
    const std::string vertexShaderSource = R"(
    #version 330 core
    uniform mat4 uView;
    uniform mat4 uProj;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec4 aOffsetScale;

    out vec2 vLocal;

    void main()
    {
        vec3 right = vec3(uView[0][0], uView[1][0], uView[2][0]);
        vec3  up   = vec3(uView[0][1], uView[1][1], uView[2][1]);

        vLocal = aPos.xy;
        vec3 worldPos = aOffsetScale.xyz +
                        (right * aPos.x + up * aPos.y) * aOffsetScale.w;

        gl_Position = uProj * uView * vec4(worldPos, 1.0);
    }
    )";
    // shader type
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // convert source
    const GLchar* vertexShaderSourceC = vertexShaderSource.c_str();
    // create shader object
    glShaderSource(vertexShader, 1, &vertexShaderSourceC, NULL);
    // compile shader object
    glCompileShader(vertexShader);

    
    // log shader error
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    else
    {
        std::cout << "success\n";

    }
    
    
    // fragment shader
    const std::string fragmentShaderSource = R"(
    #version 330 core
    in  vec2 vLocal;
    out vec4 FragColor;

    void main()
    {
        if (length(vLocal) > 0.5) discard;
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
    )";

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fragmentShaderSourceC = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourceC, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    else
    {
        std::cout << "success\n";

    }

    // create shader program
    shaderProgram = glCreateProgram();
    // attach shaders
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    // link program
    glLinkProgram(shaderProgram);
    
    // delete shaders now that the program is complete
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}



void GLPoints::bind()
{
    glBindVertexArray(vao);
}

void GLPoints::useProgram()
{
    glUseProgram(shaderProgram);
}


void GLPoints::unbind()
{
    glBindVertexArray(0);
}

void GLPoints::draw()
{
    bind();

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, pointCount);
}
