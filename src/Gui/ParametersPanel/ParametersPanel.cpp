#include "Gui/ParametersPanel/ParametersPanel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Core/Types.h"
#include "Gui/Parameters/BoolParm.h"
#include "Gui/Parameters/FloatSliderParm.h"
#include "Gui/Parameters/GroupParm.h"
#include "Gui/Parameters/IntSliderParm.h"
#include "Gui/Parameters/Parameter.h"
#include "Gui/Parameters/StringParm.h"
#include "Gui/Parameters/XYZParm.h"
#include <qboxlayout.h>
#include <qnamespace.h>
#include <qwidget.h>
#include <stdexcept>

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

enzo::ui::Parameter* ParametersPanel::buildTemplateWidget(const enzo::prm::Template& templateEntry,
                                                          enzo::nt::GeometryOperator& displayOp,
                                                          std::vector<enzo::ui::Parameter*>& leafWidgets,
                                                          int& maxLeftPadding)
{
    using namespace enzo;

    if (templateEntry.getType() == prm::Type::GROUP)
    {
        ui::GroupParm* groupWidget = new ui::GroupParm(templateEntry);
        for (const prm::Template& child : templateEntry.getChildren())
        {
            ui::Parameter* childWidget = buildTemplateWidget(child, displayOp, leafWidgets, maxLeftPadding);
            if (childWidget) groupWidget->addChild(childWidget);
        }
        // Groups participate in left-padding alignment so their labels line up with leaf labels.
        leafWidgets.push_back(groupWidget);
        const int groupLeftPadding = groupWidget->getLeftPadding();
        if (groupLeftPadding > maxLeftPadding) maxLeftPadding = groupLeftPadding;
        return groupWidget;
    }

    auto parameter = displayOp.getParameter(templateEntry.getName());
    if (parameter.expired()) return nullptr;

    ui::Parameter* leafWidget = nullptr;
    switch (templateEntry.getType())
    {
        case prm::Type::FLOAT: leafWidget = new ui::FloatSliderParm(parameter); break;
        case prm::Type::INT:   leafWidget = new ui::IntSliderParm(parameter); break;
        case prm::Type::BOOL:  leafWidget = ui::BoolParm::create(parameter); break;
        case prm::Type::XYZ:   leafWidget = new ui::XYZParm(parameter); break;
        case prm::Type::STRING: leafWidget = new ui::StringParm(parameter); break;
        default:
            throw std::runtime_error("ParametersPanel: parameter type not accounted for "
                + std::to_string(static_cast<int>(templateEntry.getType())));
    }

    leafWidgets.push_back(leafWidget);
    const int leftPadding = leafWidget->getLeftPadding();
    if (leftPadding > maxLeftPadding) maxLeftPadding = leftPadding;
    return leafWidget;
}

void ParametersPanel::selectionChanged(enzo::nt::OpId opId)
{
    using namespace enzo;
    enzo::nt::NetworkManager& nm = enzo::nt::nm();

    clearParameters();

    enzo::nt::GeometryOperator& displayOp = nm.getGeoOperator(opId);
    const std::vector<prm::Template>& templates = displayOp.getTemplates();

    std::vector<enzo::ui::Parameter*> leafWidgets;
    int maxLeftPadding = 0;

    std::vector<enzo::ui::Parameter*> topWidgets;
    topWidgets.reserve(templates.size());
    for (const prm::Template& templateEntry : templates)
    {
        enzo::ui::Parameter* widget = buildTemplateWidget(templateEntry, displayOp, leafWidgets, maxLeftPadding);
        if (widget) topWidgets.push_back(widget);
    }

    const int leftPadding = maxLeftPadding + 5;
    for (enzo::ui::Parameter* leaf : leafWidgets) leaf->setLeftPadding(leftPadding);

    for (enzo::ui::Parameter* widget : topWidgets) parametersLayout_->addWidget(widget);
}
