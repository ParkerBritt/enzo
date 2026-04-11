#include "Gui/Viewport/ViewportGLWidget.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include "Gui/Viewport/GLMesh.h"
#include "Gui/Viewport/GLPoints.h"
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <qtimer.h>
#include "Engine/Operator/Mesh.h"

void ViewportGLWidget::initializeGL()
{
    using namespace enzo;
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    enzo::geo::Mesh geo = enzo::geo::Mesh();
    triangleMesh_ = std::make_unique<GLMesh>();
    gridMesh_ = std::make_unique<GLGrid>();
    points_ = std::make_unique<GLPoints>();
    cameraPrims_ = std::make_unique<GLCameraPrim>();

    QSurfaceFormat fmt = context()->format();
    std::cout << "format: " << (fmt.renderableType() == QSurfaceFormat::OpenGLES ? "GLES" : "Desktop") << "\n";
    std::cout << "format: " << (fmt.renderableType() == QSurfaceFormat::OpenGL ? "true" : "false") << "\n";

    // init loop
    QTimer* loopTimer = new QTimer(this);
    connect(loopTimer, &QTimer::timeout, this, QOverload<>::of(&QOpenGLWidget::update));
    loopTimer->start(16);

    // init camera
    curCamera = GLCamera(-10, 5, -10);


    // vertex shader
    const std::string vertexShaderSource = R"(
        #version 330 core
        uniform mat4 uView;
        uniform mat4 uProj;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        out vec3 Normal;

        void main()
        {
            Normal = aNormal;
            gl_Position = uProj * uView * vec4(aPos, 1.0);
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
    in vec3 Normal;

    out vec4 FragColor;

    float remap(float value, float inMin, float inMax, float outMin, float outMax)
    {
        return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
    }

    vec3 remap(vec3 value, vec3 inMin, vec3 inMax, vec3 outMin, vec3 outMax)
    {
        return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
    }

    void main()
    {
        vec3 lightDir = normalize(vec3(1.0,1.0,1.0));
        float brightness = remap(dot(Normal, lightDir), -1, 1, 0.5, 1);

        vec4 Color = vec4(1.0f,1.0f,1.0f,1.0f);

        // set color to normal
        // Color = vec4(remap(Normal, vec3(-1), vec3(1), vec3(0), vec3(1)), 1.0f);


        FragColor = Color*vec4(vec3(brightness), 1.0f);
    }
    )";
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fragmentShaderSourceC = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourceC, NULL);
    glCompileShader(fragmentShader);

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



    constexpr float clearValue = 0.19;
    glClearColor(clearValue, clearValue, clearValue, 1.0f);
}



void ViewportGLWidget::resizeGL(int w, int h)
{
}

void ViewportGLWidget::paintGL()
{


    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


    glm::mat4 projMatrix = glm::perspective(
        glm::radians(45.0f),                  // FOV
        float(width()) / height(),          // aspect ratio
        0.1f,                                 // near plane
        1000.0f                                // far plane
    );


    glUseProgram(shaderProgram);
    GLint projMLoc = glGetUniformLocation(shaderProgram, "uProj");
    glUniformMatrix4fv(projMLoc, 1, GL_FALSE, glm::value_ptr(projMatrix));

    GLint viewMLoc = glGetUniformLocation(shaderProgram, "uView");
    curCamera.setUniform(viewMLoc);

    triangleMesh_->draw();

    points_->useProgram();
    points_->bind();
    points_->updatePointSize(curCamera);
    glUniformMatrix4fv(glGetUniformLocation(points_->shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(projMatrix));
    curCamera.setUniform(glGetUniformLocation(points_->shaderProgram, "uView"));

    glUniform3fv(glGetUniformLocation(points_->shaderProgram, "uCameraRight"), 1, glm::value_ptr(curCamera.getRight()));
    glUniform3fv(glGetUniformLocation(points_->shaderProgram, "uCameraUp"), 1, glm::value_ptr(curCamera.getUp()));
    points_->draw();

    cameraPrims_->useProgram();
    glUniformMatrix4fv(glGetUniformLocation(cameraPrims_->shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(projMatrix));
    curCamera.setUniform(glGetUniformLocation(cameraPrims_->shaderProgram, "uView"));
    cameraPrims_->draw();

    gridMesh_->useProgram();
    glUniformMatrix4fv(glGetUniformLocation(gridMesh_->shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(projMatrix));
    curCamera.setUniform(glGetUniformLocation(gridMesh_->shaderProgram, "uView"));
    gridMesh_->draw();


}

// std::unique_ptr<GLMesh> ViewportGLWidget::meshFromGeo(enzo::geo::Geometry& geometry)
// {
//     using namespace enzo;

//     auto mesh = std::make_unique<GLMesh>();
//         
//     std::shared_ptr<ga::Attribute> PAttr = geometry.getAttribByName(ga::AttrOwner::POINT, "P");
//     ga::AttributeHandleVector3 PAttrHandle = ga::AttributeHandleVector3(PAttr);


//     mesh->setPosBuffer(geometry);
//     mesh->setIndexBuffer(geometry);



//     return mesh; 
// }

void ViewportGLWidget::clearGeometry()
{
    geometryChanged(std::make_shared<const enzo::NodePacket>());
}

void ViewportGLWidget::geometryChanged(std::shared_ptr<const enzo::NodePacket> packet)
{
    triangleMesh_->setPosBuffer(*packet);
    triangleMesh_->setIndexBuffer(*packet);

    points_->setPoints(*packet, curCamera);
    cameraPrims_->setCameras(*packet);
}

void ViewportGLWidget::setCamera(std::shared_ptr<const enzo::geo::Camera> camera)
{
    if (!camera) return;

    auto m = camera->getTransform();

    glm::vec3 pos{
        static_cast<float>(m(0, 3)),
        static_cast<float>(m(1, 3)),
        static_cast<float>(m(2, 3))
    };
    glm::vec3 forward{
        -static_cast<float>(m(0, 2)),
        -static_cast<float>(m(1, 2)),
        -static_cast<float>(m(2, 2))
    };
    glm::vec3 up{
        static_cast<float>(m(0, 1)),
        static_cast<float>(m(1, 1)),
        static_cast<float>(m(2, 1))
    };

    curCamera.setView(pos, pos + forward, up);
    update();
}
