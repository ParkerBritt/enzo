#include "Gui/Viewport/ViewportItem.h"
#include "Gui/Viewport/GLCameraPrim.h"
#include "Gui/Viewport/GLGrid.h"
#include "Gui/Viewport/GLMesh.h"
#include "Gui/Viewport/GLPoints.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions_3_2_Core>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace enzo::ui
{

namespace
{

/// Shades the display mesh with a single headlight, or flat grey in wireframe.
const char* const kMeshVertexShader = R"(
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

const char* const kMeshFragmentShader = R"(
    #version 330 core
    in vec3 Normal;
    uniform bool uWireframe;
    out vec4 FragColor;
    float remap(float value, float inMin, float inMax, float outMin, float outMax)
    {
        return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
    }
    void main()
    {
        if (uWireframe)
        {
            FragColor = vec4(0.4, 0.4, 0.4, 1.0);
            return;
        }
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float brightness = remap(dot(Normal, lightDir), -1.0, 1.0, 0.5, 1.0);
        FragColor = vec4(vec3(brightness), 1.0);
    }
)";

/// @brief Renders the scene into the item's framebuffer on the render thread.
///
/// Owns every GL resource and a copy of the camera, both refreshed from the item
/// during `synchronize`. The grid draws first so geometry overdraws it.
class ViewportRenderer : public QQuickFramebufferObject::Renderer,
                         protected QOpenGLFunctions_3_2_Core
{
  public:
    void synchronize(QQuickFramebufferObject* item) override
    {
        auto* viewport = static_cast<ViewportItem*>(item);
        camera_ = viewport->getCamera();
        size_ = viewport->size().toSize();

        if (auto packet = viewport->takePendingGeometry())
        {
            ensureInitialised();
            mesh_->setPosBuffer(*packet);
            mesh_->setIndexBuffer(*packet);
            points_->setPoints(*packet, camera_);
            cameraPrims_->setCameras(*packet);
        }
    }

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void render() override
    {
        ensureInitialised();

        glViewport(0, 0, size_.width(), size_.height());
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_MULTISAMPLE);

        constexpr float clearValue = 0.19f;
        glClearColor(clearValue, clearValue, clearValue, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = size_.height() > 0
            ? float(size_.width()) / float(size_.height())
            : 1.f;
        const glm::mat4 proj = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);

        drawGrid(proj);
        drawMesh(proj);
        drawPoints(proj);
        drawCameraPrims(proj);
    }

  private:
    // Builds the GL resources once a context is current on the render thread.
    void ensureInitialised()
    {
        if (initialised_) return;
        initializeOpenGLFunctions();

        mesh_ = std::make_unique<GLMesh>();
        grid_ = std::make_unique<GLGrid>();
        points_ = std::make_unique<GLPoints>();
        cameraPrims_ = std::make_unique<GLCameraPrim>();
        meshProgram_ = buildMeshProgram();
        initialised_ = true;
    }

    GLuint buildMeshProgram()
    {
        auto compile = [this](GLenum type, const char* source) {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            return shader;
        };
        GLuint vertex = compile(GL_VERTEX_SHADER, kMeshVertexShader);
        GLuint fragment = compile(GL_FRAGMENT_SHADER, kMeshFragmentShader);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return program;
    }

    void drawGrid(const glm::mat4& proj)
    {
        grid_->useProgram();
        glUniformMatrix4fv(
            glGetUniformLocation(grid_->shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(proj)
        );
        camera_.setUniform(glGetUniformLocation(grid_->shaderProgram, "uView"));
        grid_->draw();
    }

    void drawMesh(const glm::mat4& proj)
    {
        glUseProgram(meshProgram_);
        glUniformMatrix4fv(
            glGetUniformLocation(meshProgram_, "uProj"), 1, GL_FALSE, glm::value_ptr(proj)
        );
        camera_.setUniform(glGetUniformLocation(meshProgram_, "uView"));

        const GLint wireLoc = glGetUniformLocation(meshProgram_, "uWireframe");
        glUniform1i(wireLoc, 0);
        mesh_->draw();
        glUniform1i(wireLoc, 1);
        mesh_->drawWireframe();
    }

    void drawPoints(const glm::mat4& proj)
    {
        points_->useProgram();
        points_->bind();
        points_->updatePointSize(camera_);
        glUniformMatrix4fv(
            glGetUniformLocation(points_->shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(proj)
        );
        camera_.setUniform(glGetUniformLocation(points_->shaderProgram, "uView"));
        glUniform3fv(
            glGetUniformLocation(points_->shaderProgram, "uCameraRight"),
            1,
            glm::value_ptr(camera_.getRight())
        );
        glUniform3fv(
            glGetUniformLocation(points_->shaderProgram, "uCameraUp"),
            1,
            glm::value_ptr(camera_.getUp())
        );
        points_->draw();
    }

    void drawCameraPrims(const glm::mat4& proj)
    {
        cameraPrims_->useProgram();
        glUniformMatrix4fv(
            glGetUniformLocation(cameraPrims_->shaderProgram, "uProj"),
            1,
            GL_FALSE,
            glm::value_ptr(proj)
        );
        camera_.setUniform(glGetUniformLocation(cameraPrims_->shaderProgram, "uView"));
        cameraPrims_->draw();
    }

    bool initialised_ = false;
    QSize size_;
    GLCamera camera_;
    GLuint meshProgram_ = 0;
    std::unique_ptr<GLMesh> mesh_;
    std::unique_ptr<GLGrid> grid_;
    std::unique_ptr<GLPoints> points_;
    std::unique_ptr<GLCameraPrim> cameraPrims_;
};

} // namespace

ViewportItem::ViewportItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {}

QQuickFramebufferObject::Renderer* ViewportItem::createRenderer() const
{
    return new ViewportRenderer;
}

std::shared_ptr<const enzo::NodePacket> ViewportItem::takePendingGeometry()
{
    return std::move(pendingGeometry_);
}

void ViewportItem::setGeometry(std::shared_ptr<const enzo::NodePacket> packet)
{
    pendingGeometry_ = std::move(packet);
    update();
}

} // namespace enzo::ui
