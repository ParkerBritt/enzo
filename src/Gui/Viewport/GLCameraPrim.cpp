#include "Gui/Viewport/GLCameraPrim.h"
#include "Engine/Operator/Camera.h"
#include "Engine/Types.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>

// Unit rectangle wireframe vertices (GL_LINES): 4 edges of a 1x0.75 rect centered at origin
static const glm::vec3 rectVerts[] = {
    // bottom edge
    {-0.5f, -0.375f, 0.0f}, { 0.5f, -0.375f, 0.0f},
    // right edge
    { 0.5f, -0.375f, 0.0f}, { 0.5f,  0.375f, 0.0f},
    // top edge
    { 0.5f,  0.375f, 0.0f}, {-0.5f,  0.375f, 0.0f},
    // left edge
    {-0.5f,  0.375f, 0.0f}, {-0.5f, -0.375f, 0.0f},
};
static constexpr size_t kRectVertCount = sizeof(rectVerts) / sizeof(rectVerts[0]);

GLCameraPrim::GLCameraPrim() {
    initializeOpenGLFunctions();
    init();
}

void GLCameraPrim::init() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    initBuffers();
    initShaderProgram();

    glBindVertexArray(0);
}

void GLCameraPrim::initBuffers() {
    // Static rectangle geometry
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVerts), rectVerts, GL_STATIC_DRAW);

    // location 0: vertex position (per-vertex)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Instance buffer for model matrices (mat4 = 4 vec4 attribute slots)
    glGenBuffers(1, &instanceVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    for (int i = 0; i < 4; ++i) {
        glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (void*)(i * sizeof(glm::vec4)));
        glEnableVertexAttribArray(1 + i);
        glVertexAttribDivisor(1 + i, 1);
    }
}

void GLCameraPrim::initShaderProgram() {
    const std::string vertSrc = R"(
        #version 330 core
        uniform mat4 uView;
        uniform mat4 uProj;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in mat4 aModel;
        void main()
        {
            gl_Position = uProj * uView * aModel * vec4(aPos, 1.0);
        }
    )";

    const std::string fragSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main()
        {
            FragColor = vec4(0.9, 0.7, 0.2, 1.0);
        }
    )";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* vsc = vertSrc.c_str();
    glShaderSource(vs, 1, &vsc, NULL);
    glCompileShader(vs);

    int success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cout << "ERROR::GLCAMERAPRIM::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fsc = fragSrc.c_str();
    glShaderSource(fs, 1, &fsc, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cout << "ERROR::GLCAMERAPRIM::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void GLCameraPrim::setCameras(const enzo::NodePacket& packet) {
    std::vector<glm::mat4> models;

    for (size_t i = 0; i < packet.size(); ++i) {
        auto prim = packet.getPrimitive(i);
        if (prim->getType() != enzo::geo::PrimType::CAMERA) continue;

        auto cam = std::static_pointer_cast<const enzo::geo::Camera>(prim);
        auto eigenMat = cam->getTransform();

        // Eigen is column-major, glm is column-major — copy directly
        glm::mat4 model;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                model[col][row] = static_cast<float>(eigenMat(row, col));

        models.push_back(model);
    }

    instanceCount_ = models.size();

    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);
    glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4),
                 models.data(), GL_DYNAMIC_DRAW);
}

void GLCameraPrim::bind() { glBindVertexArray(vao); }
void GLCameraPrim::unbind() { glBindVertexArray(0); }
void GLCameraPrim::useProgram() { glUseProgram(shaderProgram); }

void GLCameraPrim::draw() {
    if (instanceCount_ == 0) return;
    bind();
    useProgram();
    glDrawArraysInstanced(GL_LINES, 0, kRectVertCount, instanceCount_);
}
