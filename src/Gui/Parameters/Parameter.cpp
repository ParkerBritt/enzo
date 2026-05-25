#include "Gui/Parameters/Parameter.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

enzo::ui::Parameter::Parameter(const prm::Template& parmTemplate, QWidget* parent)
: QWidget(parent)
{
    mainLayout_ = new QHBoxLayout();
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout_);

    std::string tooltipFull = "<span style=\"font-weight:600;\">" + parmTemplate.getLabel() + "</span>";
    const std::string tooltip = parmTemplate.getTooltip();
    if (!tooltip.empty())
    {
        tooltipFull += "\n" + tooltip;
    }
    setToolTip(QString::fromStdString(tooltipFull));

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
        contentWidget_->setStyleSheet(R"(
            .ParameterBg {
                background: #1a1a1a;
                border-radius: 8px;
                border: 1px solid #383838;
            }
        )");
    }
    else
    {
        contentWidget_->setStyleSheet(".ParameterBg { background: transparent; border: none; }");
    }
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

void enzo::ui::Parameter::addChild(Parameter*)
{
}
