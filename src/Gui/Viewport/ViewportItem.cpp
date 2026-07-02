#include "Gui/Viewport/ViewportItem.h"
#include "Gui/Viewport/GLCameraPrim.h"
#include "Gui/Viewport/GLGrid.h"
#include "Gui/Viewport/GLMesh.h"
#include "Gui/Viewport/GLPoints.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions_3_2_Core>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace enzo::ui {

namespace {

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
    uniform vec3 uGeometry;
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
        FragColor = vec4(uGeometry * brightness, 1.0);
    }
)";

/// Fills the viewport with a radial gradient running from a bright centre
/// through a base colour out to a dark edge, all supplied by the theme. Drawn
/// as one screen triangle.
const char* const kBackgroundVertexShader = R"(
    #version 330 core
    out vec2 vUv;
    void main()
    {
        vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
        vUv = corner;
        gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
    }
)";

const char* const kBackgroundFragmentShader = R"(
    #version 330 core
    in vec2 vUv;
    uniform vec3 uCenter;
    uniform vec3 uBase;
    uniform vec3 uEdge;
    out vec4 FragColor;
    void main()
    {
        // Match radial-gradient(120% 100% at 50% 0%) in box relative space.
        // The framebuffer composites flipped, so the top stop sits at uv.y 0.
        vec2 uv = vUv;
        float dx = (uv.x - 0.5) / 1.2;
        float dy = uv.y;
        float d = sqrt(dx * dx + dy * dy);

        vec3 colour = d < 0.55
            ? mix(uCenter, uBase, d / 0.55)
            : mix(uBase, uEdge, clamp((d - 0.55) / 0.45, 0.0, 1.0));
        FragColor = vec4(colour, 1.0);
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
        backgroundColor_ = viewport->backgroundColor();
        gradientCenter_ = viewport->gradientCenter();
        gradientEdge_ = viewport->gradientEdge();
        geometryColor_ = viewport->geometryColor();

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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect =
            size_.height() > 0 ? float(size_.width()) / float(size_.height()) : 1.f;
        glm::mat4 proj = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);

        // The scene graph composites the framebuffer flipped, so the scene is
        // rendered flipped to land upright.
        proj[1][1] *= -1.f;

        drawBackground();
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
        meshProgram_ = buildProgram(kMeshVertexShader, kMeshFragmentShader);
        backgroundProgram_ = buildProgram(kBackgroundVertexShader, kBackgroundFragmentShader);
        glGenVertexArrays(1, &backgroundVao_);
        initialised_ = true;
    }

    GLuint buildProgram(const char* vertexSource, const char* fragmentSource)
    {
        auto compile = [this](GLenum type, const char* source) {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            return shader;
        };
        GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
        GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return program;
    }

    // Paints the radial background behind everything, leaving depth untouched.
    void drawBackground()
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(backgroundProgram_);

        auto setColour = [this](const char* name, const QColor& colour) {
            glUniform3f(
                glGetUniformLocation(backgroundProgram_, name),
                colour.redF(),
                colour.greenF(),
                colour.blueF()
            );
        };
        setColour("uCenter", gradientCenter_);
        setColour("uBase", backgroundColor_);
        setColour("uEdge", gradientEdge_);

        glBindVertexArray(backgroundVao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

    void drawGrid(const glm::mat4& proj)
    {
        grid_->useProgram();
        glUniformMatrix4fv(
            glGetUniformLocation(grid_->shaderProgram, "uProj"),
            1,
            GL_FALSE,
            glm::value_ptr(proj)
        );
        camera_.setUniform(glGetUniformLocation(grid_->shaderProgram, "uView"));
        grid_->draw();
    }

    void drawMesh(const glm::mat4& proj)
    {
        glUseProgram(meshProgram_);
        glUniformMatrix4fv(
            glGetUniformLocation(meshProgram_, "uProj"),
            1,
            GL_FALSE,
            glm::value_ptr(proj)
        );
        camera_.setUniform(glGetUniformLocation(meshProgram_, "uView"));
        glUniform3f(
            glGetUniformLocation(meshProgram_, "uGeometry"),
            geometryColor_.redF(),
            geometryColor_.greenF(),
            geometryColor_.blueF()
        );

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
            glGetUniformLocation(points_->shaderProgram, "uProj"),
            1,
            GL_FALSE,
            glm::value_ptr(proj)
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
    QColor backgroundColor_;
    QColor gradientCenter_;
    QColor gradientEdge_;
    QColor geometryColor_;
    GLuint meshProgram_ = 0;
    GLuint backgroundProgram_ = 0;
    GLuint backgroundVao_ = 0;
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

void ViewportItem::setViewModel(ViewportViewModel* viewModel)
{
    if (viewModel_ == viewModel) return;
    viewModel_ = viewModel;

    if (viewModel_)
    {
        connect(viewModel_, &ViewportViewModel::geometryChanged, this, [this]() {
            setGeometry(viewModel_->currentGeometry());
        });
        setGeometry(viewModel_->currentGeometry());
    }
    Q_EMIT viewModelChanged();
}

void ViewportItem::setBackgroundColor(const QColor& colour)
{
    if (backgroundColor_ == colour) return;
    backgroundColor_ = colour;
    update();
    Q_EMIT backgroundColorChanged();
}

void ViewportItem::setGeometryColor(const QColor& colour)
{
    if (geometryColor_ == colour) return;
    geometryColor_ = colour;
    update();
    Q_EMIT geometryColorChanged();
}

void ViewportItem::setGradientCenter(const QColor& colour)
{
    if (gradientCenter_ == colour) return;
    gradientCenter_ = colour;
    update();
    Q_EMIT gradientCenterChanged();
}

void ViewportItem::setGradientEdge(const QColor& colour)
{
    if (gradientEdge_ == colour) return;
    gradientEdge_ = colour;
    update();
    Q_EMIT gradientEdgeChanged();
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

void ViewportItem::orbit(qreal dx, qreal dy)
{
    constexpr float speed = 0.01f;
    camera_.rotateAroundCenter(float(-dx) * speed, glm::vec3(0.f, 1.f, 0.f));
    camera_.rotateAroundCenter(float(-dy) * speed, camera_.getRight());
    update();
}

void ViewportItem::pan(qreal dx, qreal dy)
{
    // Pan speed tracks distance so the scene tracks the cursor at any zoom.
    const float speed = 0.0015f * glm::max(glm::length(camera_.getPos()), 1.f);
    const glm::vec3 offset =
        camera_.getRight() * float(-dx) * speed + camera_.getUp() * float(-dy) * speed;
    camera_.movePos(offset.x, offset.y, offset.z);
    camera_.changeCenter(offset.x, offset.y, offset.z);
    update();
}

void ViewportItem::zoom(qreal amount)
{
    camera_.changeRadius(float(amount) * 0.1f * glm::max(glm::length(camera_.getPos()), 1.f));
    update();
}

} // namespace enzo::ui
