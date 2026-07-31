#pragma once

#include "Engine/Parameter/Ramp.h"
#include <QColor>
#include <QQuickItem>
#include <QVariantList>

namespace enzo::ui {

/// @brief Draws a ramp parameter's curve, sampled through prm::Ramp so the
/// plot matches what a cook reads.
/// @note Points are maps of position, value, and interp token, in any order.
class RampCurveItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
    Q_PROPERTY(QColor curveColor MEMBER curveColor_ NOTIFY styleChanged)
    Q_PROPERTY(QColor fillColor MEMBER fillColor_ NOTIFY styleChanged)
    Q_PROPERTY(qreal curveWidth MEMBER curveWidth_ NOTIFY styleChanged)

  public:
    explicit RampCurveItem(QQuickItem* parent = nullptr);

    /// @brief Samples the curve at a position in the zero to one domain.
    Q_INVOKABLE qreal valueAt(qreal position) const;

    QVariantList points() const { return points_; }
    void setPoints(const QVariantList& points);

  Q_SIGNALS:
    void pointsChanged();
    void styleChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  private:
    QVariantList points_;
    prm::Ramp ramp_;
    QColor curveColor_;
    QColor fillColor_;
    qreal curveWidth_ = 2;
};

} // namespace enzo::ui
