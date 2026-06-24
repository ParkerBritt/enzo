#include "LegacyGui/Parameters/Parameter.h"
#include "LegacyGui/Style.h"
#include <QEvent>
#include <QFontMetrics>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <string>

namespace {

std::string buildTooltip(const enzo::prm::Template& parmTemplate)
{
    const std::string description = parmTemplate.getTooltip();
    const std::string label = parmTemplate.getLabel();
    const std::string name = "name: " + parmTemplate.getName();

    // Measure the label and name at their own point sizes so neither title line wraps
    QFont labelFont;
    labelFont.setPointSize(10);
    QFont nameFont;
    nameFont.setPointSize(9);
    int minWidth = std::max(
                       QFontMetrics(labelFont).horizontalAdvance(QString::fromStdString(label)),
                       QFontMetrics(nameFont).horizontalAdvance(QString::fromStdString(name))
                   ) *
                   1.3;

    int maxWidth = 400;
    int width = std::clamp<int>(description.size() * 6, std::min(minWidth, maxWidth), maxWidth);

    // Had to use this weird table workaround to set the width
    std::string tooltip = "<html><table><tr><td width=\"" + std::to_string(width) + "\">";
    tooltip += "<span style=\"font-size:10pt; font-weight:600;\">" + label + "</span>";
    tooltip += "<br><span style=\"font-size:9pt; font-weight:500;\">" + name + "</span>";
    if (!description.empty())
    {
        tooltip += "<br><span style=\"font-size:8pt;\">" + description + "</span>";
    }
    tooltip += "</div></html>";
    return tooltip;
}

} // namespace

enzo::ui::Parameter::Parameter(const prm::Template& parmTemplate, QWidget* parent)
    : QWidget(parent), parmTemplate_(parmTemplate)
{
    mainLayout_ = new QHBoxLayout();
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout_);

    setToolTip(QString::fromStdString(buildTooltip(parmTemplate)));

    if (!parmTemplate.isLabelHidden())
    {
        const std::string name = parmTemplate.getLabel();
        label_ = new QLabel(QString::fromStdString(name + ":"));
        label_->setStyleSheet("QLabel{background: none}");
        label_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        mainLayout_->addWidget(label_);
    }

    contentWidget_ = new QWidget();
    contentLayout_ = parmTemplate.getDirection() == prm::Direction::HORIZONTAL
                         ? static_cast<QBoxLayout*>(new QHBoxLayout())
                         : static_cast<QBoxLayout*>(new QVBoxLayout());
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    contentWidget_->setLayout(contentLayout_);
    mainLayout_->addWidget(contentWidget_);

    contentWidget_->setProperty("class", "ParameterBg");
    if (parmTemplate.isBackgroundEnabled())
    {
        contentWidget_->setStyleSheet(
            QString(R"(
            .ParameterBg {
                background: %2;
                border-radius: %1px;
                border: 1px solid %3;
            }
        )")
                .arg(enzo::style::parameter::borderRadius)
                .arg(enzo::style::color::surfaceDeep.name(), enzo::style::color::border.name())
        );
    }
    else
    {
        contentWidget_->setStyleSheet(".ParameterBg { background: transparent; border: none; }");
    }
}

void enzo::ui::Parameter::disableBackground()
{
    contentWidget_->setStyleSheet(".ParameterBg { background: transparent; border: none; }");
}

void enzo::ui::Parameter::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() != QEvent::EnabledChange) return;

    // One opacity effect over the whole row dims the label and editor together,
    // so even widgets that paint themselves read as disabled with no extra code.
    if (isEnabled())
    {
        setGraphicsEffect(nullptr);
        return;
    }

    auto* dim = new QGraphicsOpacityEffect(this);
    dim->setOpacity(enzo::style::parameter::disabledOpacity);
    setGraphicsEffect(dim);
}

int enzo::ui::Parameter::getLeftPadding()
{
    if (!label_) return 0;
    return label_->minimumSizeHint().width();
}

void enzo::ui::Parameter::setLeftPadding(int padding)
{
    if (!label_) return;
    label_->setFixedWidth(padding);
}

void enzo::ui::Parameter::addChild(Parameter*) {}
