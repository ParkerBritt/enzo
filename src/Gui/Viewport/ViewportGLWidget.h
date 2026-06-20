#if defined(__linux__) || defined(_WIN32)
#include <GL/gl.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Camera.h"
#include "Engine/Primitives/Mesh.h"
#include "Gui/Viewport/GLCamera.h"
#include "Gui/Viewport/GLCameraPrim.h"
#include "Gui/Viewport/GLGrid.h"
#include "Gui/Viewport/GLMesh.h"
#include "Gui/Viewport/GLPoints.h"
#include <GL/glext.h>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLWidget>
#include <iostream>
#include <memory>

class ViewportGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
{
  public:
    ViewportGLWidget(QWidget* parent) : QOpenGLWidget(parent) {}
    QSize sizeHint() const override { return QSize(-1, -1); }
    GLuint shaderProgram;
    GLCamera curCamera;
    std::unique_ptr<GLMesh> triangleMesh_;
    std::unique_ptr<GLGrid> gridMesh_;
    std::unique_ptr<GLPoints> points_;
    std::unique_ptr<GLCameraPrim> cameraPrims_;
    bool wireframeMode_ = true;

    // std::unique_ptr<GLMesh> meshFromGeo(enzo::geo::Geometry& geometry);

  protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

  public Q_SLOTS:
    void geometryChanged(std::shared_ptr<const enzo::NodePacket> packet);
    void clearGeometry();
    void setCamera(std::shared_ptr<const enzo::geo::Camera> camera);
    void toggleWireframe();
};
