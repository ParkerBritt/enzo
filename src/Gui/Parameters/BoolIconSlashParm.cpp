#include "Gui/Parameters/BoolIconSlashParm.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include "Gui/IconRegistry.h"
#include "Gui/Style.h"
#include <QEasingCurve>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSize>

namespace enzo::ui {

namespace {

constexpr int ANIMATION_DURATION_MS = 160;
constexpr int TILT_DURATION_MS = 260;
constexpr float TILT_PEAK_DEGREES = -14.0f;
constexpr float SLASH_WIDTH_FRACTION = 0.13f;
constexpr float SLASH_MASK_FRACTION = 0.28f;
constexpr float SLASH_PADDING = 0.12f;

QLineF slashLine(const QRectF& iconRect, float progress)
{
    const QPointF start(
        iconRect.left() + iconRect.width() * SLASH_PADDING,
        iconRect.top() + iconRect.height() * SLASH_PADDING
    );
    const QPointF end(
        iconRect.right() - iconRect.width() * SLASH_PADDING,
        iconRect.bottom() - iconRect.height() * SLASH_PADDING
    );
    const QPointF current = start + (end - start) * progress;
    return QLineF(start, current);
}

} // namespace

SlashIconButton::SlashIconButton(QPixmap icon, int iconPx, QWidget* parent)
    : QAbstractButton(parent), icon_(std::move(icon)), iconPx_(iconPx)
{
    setCheckable(true);
}

void SlashIconButton::setSlashProgress(float progress)
{
    if (qFuzzyCompare(slashProgress_, progress)) return;
    slashProgress_ = progress;
    update();
}

void SlashIconButton::setTiltAngle(float degrees)
{
    if (qFuzzyCompare(tiltAngle_, degrees)) return;
    tiltAngle_ = degrees;
    update();
}

QSize SlashIconButton::sizeHint() const { return QSize(iconPx_, iconPx_); }

void SlashIconButton::paintEvent(QPaintEvent*)
{
    const QRectF widgetRect = rect();
    const QRectF iconRect(
        widgetRect.center().x() - iconPx_ / 2.0,
        widgetRect.center().y() - iconPx_ / 2.0,
        iconPx_,
        iconPx_
    );

    QPixmap composed(size());
    composed.fill(Qt::transparent);
    {
        QPainter layer(&composed);
        layer.setRenderHint(QPainter::Antialiasing);
        layer.setRenderHint(QPainter::SmoothPixmapTransform);
        layer.drawPixmap(iconRect, icon_, icon_.rect());

        if (slashProgress_ > 0.0f)
        {
            const QLineF line = slashLine(iconRect, slashProgress_);
            const float maskWidth = iconRect.width() * SLASH_MASK_FRACTION;

            QPen maskPen(Qt::black, maskWidth);
            maskPen.setCapStyle(Qt::RoundCap);
            layer.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            layer.setPen(maskPen);
            layer.drawLine(line);
        }
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!qFuzzyIsNull(tiltAngle_))
    {
        const QPointF pivot = iconRect.center();
        painter.translate(pivot);
        painter.rotate(tiltAngle_);
        painter.translate(-pivot);
    }

    painter.drawPixmap(0, 0, composed);

    if (slashProgress_ > 0.0f)
    {
        const QLineF line = slashLine(iconRect, slashProgress_);
        const float slashWidth = iconRect.width() * SLASH_WIDTH_FRACTION;

        QColor slashColor("#ff4d50");
        slashColor.setAlphaF(0.8);
        QPen slashPen(slashColor, slashWidth);
        slashPen.setCapStyle(Qt::RoundCap);
        painter.setPen(slashPen);
        painter.drawLine(line);
    }
}

BoolIconSlashParm::BoolIconSlashParm(
    std::weak_ptr<prm::NodeParameter> parameter,
    std::shared_ptr<prm::style::BoolIconSlash> style,
    QWidget* parent
)
    : Parameter(std::shared_ptr<prm::NodeParameter>(parameter)->getTemplate(), parent),
      parameter_(parameter), style_(std::move(style))
{
    auto parameterShared = parameter_.lock();

    const int sizePx = parameterHeight;
    const int iconPx = sizePx * style_->scale() - 8;

    QPixmap iconPixmap = IconRegistry::instance().pixmap(style_->icon(), QSize(iconPx, iconPx));

    button_ = new SlashIconButton(std::move(iconPixmap), iconPx);
    button_->setFixedSize(sizePx, sizePx);

    const bool toggledOn = parameterShared->evalInt() != 0;
    button_->setChecked(toggledOn);
    button_->setSlashProgress(toggledOn ? 0.0f : 1.0f);

    animation_ = new QVariantAnimation(this);
    animation_->setDuration(ANIMATION_DURATION_MS);
    animation_->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        button_->setSlashProgress(value.toFloat());
    });

    tiltAnimation_ = new QVariantAnimation(this);
    tiltAnimation_->setDuration(TILT_DURATION_MS);
    tiltAnimation_->setEasingCurve(QEasingCurve::InOutCubic);
    tiltAnimation_->setStartValue(0.0f);
    tiltAnimation_->setKeyValueAt(0.45, TILT_PEAK_DEGREES);
    tiltAnimation_->setEndValue(0.0f);
    connect(tiltAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        button_->setTiltAngle(value.toFloat());
    });

    contentLayout_->addWidget(button_);
    contentLayout_->addStretch();
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    valueChangedConnection_ =
        parameterShared->valueChanged.connect([this]() { syncFromParameter(); });
    connect(button_, &QAbstractButton::toggled, this, &BoolIconSlashParm::onToggle);
}

void BoolIconSlashParm::syncFromParameter()
{
    if (auto parameterShared = parameter_.lock())
    {
        const bool toggled = parameterShared->evalInt() != 0;
        button_->blockSignals(true);
        button_->setChecked(toggled);
        button_->blockSignals(false);
        animateTo(toggled);
    }
}

void BoolIconSlashParm::onToggle(bool checked)
{
    if (auto parameterShared = parameter_.lock())
    {
        auto before = parameterShared->getValues();
        parameterShared->setInt(checked);
        auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(
            parameterShared->getOpId(),
            parameterShared->getName(),
            before,
            parameterShared->getValues()
        );
        enzo::nt::nm().undoStack().push(std::move(cmd));
        animateTo(checked);
        if (!checked)
        {
            tiltAnimation_->stop();
            tiltAnimation_->start();
        }
    }
}

void BoolIconSlashParm::animateTo(bool toggledOn)
{
    const float target = toggledOn ? 0.0f : 1.0f;
    animation_->stop();
    animation_->setStartValue(button_->slashProgress());
    animation_->setEndValue(target);
    animation_->start();
}

} // namespace enzo::ui
