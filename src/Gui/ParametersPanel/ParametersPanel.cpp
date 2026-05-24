#include "Gui/ParametersPanel/ParametersPanel.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Types.h"
#include "Gui/Parameters/FormParm.h"
#include "Engine/Network/NetworkManager.h"
#include <qboxlayout.h>
#include <qnamespace.h>
#include <qwidget.h>

ParametersPanel::ParametersPanel(QWidget *parent)
: Panel(parent)
{
    mainLayout_ = new QVBoxLayout();
    parametersLayout_ = new QVBoxLayout();
    parametersLayout_->setContentsMargins(15,15,15,15);
    parametersLayout_->setAlignment(Qt::AlignTop);
    bgWidget_ = new QWidget();
    bgWidget_->setLayout(parametersLayout_);

    bgWidget_->setObjectName("ParametersPanelBg");
    bgWidget_->setStyleSheet(R"(
        #ParametersPanelBg {
            background-color: #282828;
        }
    )");

    mainLayout_->addWidget(bgWidget_);

    setLayout(mainLayout_);
}

void ParametersPanel::clearParameters()
{
    QLayoutItem *child;
    while ((child = parametersLayout_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
}

QWidget* ParametersPanel::buildTemplateWidget(const enzo::prm::Template& templateEntry,
                                              enzo::nt::GeometryOperator& displayOp,
                                              std::vector<enzo::ui::FormParm*>& leafWidgets,
                                              int& maxLeftPadding)
{
    using namespace enzo;

    // Group recurses into a nested horizontal or vertical layout
    if (templateEntry.getType() == prm::Type::GROUP)
    {
        QWidget* groupWidget = new QWidget();
        groupWidget->setAttribute(Qt::WA_TranslucentBackground);
        QBoxLayout* groupLayout = templateEntry.getDirection() == prm::Direction::HORIZONTAL
            ? static_cast<QBoxLayout*>(new QHBoxLayout())
            : static_cast<QBoxLayout*>(new QVBoxLayout());
        groupLayout->setContentsMargins(0, 0, 0, 0);
        for (const prm::Template& child : templateEntry.getChildren())
        {
            QWidget* childWidget = buildTemplateWidget(child, displayOp, leafWidgets, maxLeftPadding);
            if (childWidget) groupLayout->addWidget(childWidget);
        }
        groupWidget->setLayout(groupLayout);
        return groupWidget;
    }

    // Leaf creates a FormParm wired to the live Parameter
    auto parameter = displayOp.getParameter(templateEntry.getName());
    if (parameter.expired()) return nullptr;
    enzo::ui::FormParm* parameterWidget = new enzo::ui::FormParm(parameter);
    leafWidgets.push_back(parameterWidget);
    const int leftPadding = parameterWidget->getLeftPadding();
    if (leftPadding > maxLeftPadding) maxLeftPadding = leftPadding;
    return parameterWidget;
}

void ParametersPanel::selectionChanged(enzo::nt::OpId opId)
{
    using namespace enzo;
    enzo::nt::NetworkManager& nm = enzo::nt::nm();

    clearParameters();

    enzo::nt::GeometryOperator& displayOp = nm.getGeoOperator(opId);
    const std::vector<prm::Template>& templates = displayOp.getTemplates();

    std::vector<enzo::ui::FormParm*> leafWidgets;
    int maxLeftPadding = 0;

    // Build all top level widgets
    std::vector<QWidget*> topWidgets;
    topWidgets.reserve(templates.size());
    for (const prm::Template& templateEntry : templates)
    {
        QWidget* widget = buildTemplateWidget(templateEntry, displayOp, leafWidgets, maxLeftPadding);
        if (widget) topWidgets.push_back(widget);
    }

    // Align leaf labels across the panel
    const int leftPadding = maxLeftPadding + 5;
    for (enzo::ui::FormParm* leaf : leafWidgets) leaf->setLeftPadding(leftPadding);

    for (QWidget* widget : topWidgets) parametersLayout_->addWidget(widget);
}
