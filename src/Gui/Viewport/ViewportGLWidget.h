#include <GL/gl.h>
#include <GL/glext.h>
#include <QOpenGLWidget>
#include <iostream>
#include <QOpenGLFunctions_3_2_Core>
#include "Gui/Viewport/GLCamera.h"
#include "Gui/Viewport/GLMesh.h"
#include "Gui/Viewport/GLGrid.h"
#include "Gui/Viewport/GLPoints.h"
#include "Engine/Operator/Primitive.h"
#include "Engine/Operator/NodePacket.h"

class ViewportGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
{
public:
    ViewportGLWidget(QWidget *parent) : QOpenGLWidget(parent) { }
    QSize sizeHint() const override { return QSize(-1, -1); }
    GLuint shaderProgram;
    GLCamera curCamera;
    std::unique_ptr<GLMesh> triangleMesh_ ;
    std::unique_ptr<GLGrid> gridMesh_ ;
    std::unique_ptr<GLPoints> points_ ;

    // std::unique_ptr<GLMesh> meshFromGeo(enzo::geo::Geometry& geometry);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

public Q_SLOTS:
    void geometryChanged(enzo::NodePacket& packet);
    void clearGeometry();
};
