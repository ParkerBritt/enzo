#pragma once
#include "Graphics/FrameState.h"
#include "Graphics/ViewportRenderer.h"
#include <QObject>
#include <QSGSimpleTextureNode>

class QQuickWindow;

namespace enzo::ui {

/// @brief The scene graph node that renders and shows a viewport on the render thread.
class ViewportNode : public QObject, public QSGSimpleTextureNode
{
    Q_OBJECT
  public:
    explicit ViewportNode(QQuickWindow* window);

    /// @brief Takes the frame to draw next and the rectangle to show it in.
    void sync(const gfx::FrameState& frame, const QRectF& rect);

  private:
    /// @brief Renders the pending frame when it differs from the one on screen.
    void render();

    QQuickWindow* window_;
    gfx::ViewportRenderer renderer_;
    gfx::FrameState pendingFrame_;
    gfx::FrameState renderedFrame_;
    void* shownImage_ = nullptr;
};

} // namespace enzo::ui
