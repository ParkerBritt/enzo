#include "Gui/ParametersPanel/ParametersPanel.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/Template.h"
#include "Gui/Parameters/BoolParm.h"
#include "Gui/Parameters/DropdownParm.h"
#include "Gui/Parameters/FloatSliderParm.h"
#include "Gui/Parameters/GroupParm.h"
#include "Gui/Parameters/IntSliderParm.h"
#include "Gui/Parameters/Parameter.h"
#include "Gui/Parameters/RampParm.h"
#include "Gui/Parameters/SpacerParm.h"
#include "Gui/Parameters/StringParm.h"
#include "Gui/Parameters/XYZParm.h"
#include "Gui/Style.h"
#include <QLabel>
#include <QScrollArea>
#include <qboxlayout.h>
#include <qnamespace.h>
#include <qwidget.h>
#include <stdexcept>

ParametersPanel::ParametersPanel(QWidget* parent) : Panel(parent)
{
    mainLayout_ = new QVBoxLayout();
    parametersLayout_ = new QVBoxLayout();
    parametersLayout_->setContentsMargins(25, 15, 15, 15);
    parametersLayout_->setSpacing(enzo::ui::parameterGap);
    parametersLayout_->setAlignment(Qt::AlignTop);
    bgWidget_ = new QWidget();
    bgWidget_->setLayout(parametersLayout_);

    bgWidget_->setObjectName("ParametersPanelBg");
    bgWidget_->setStyleSheet(R"(
        #ParametersPanelBg {
            background-color: #282828;
        }
    )");

    // Add scrollArea for lots of parameters.
    scrollArea_ = new QScrollArea();
    scrollArea_->setWidget(bgWidget_);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setStyleSheet(R"(
        QScrollBar:vertical {
            background: #1B1B1B;
            width: 15px;
        }
        QScrollBar::handle:vertical {
            background: #282828;
            min-height: 50px;
            border-radius: 5px;
            border: 1px solid #2D2D2D;
            margin: 2px;
        }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical,
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
    )");

    // Placeholder shown while no node is selected.
    noSelectionLabel_ = new QLabel("No Node Selected");
    noSelectionLabel_->setAlignment(Qt::AlignCenter);
    noSelectionLabel_->setStyleSheet(R"(
        background-color: #282828;
        color: rgba(255, 255, 255, 50%);
    )");

    mainLayout_->addWidget(scrollArea_);
    mainLayout_->addWidget(noSelectionLabel_);
    scrollArea_->hide();

    setLayout(mainLayout_);
}

void ParametersPanel::clearParameters()
{
    QLayoutItem* child;
    while ((child = parametersLayout_->takeAt(0)) != nullptr)
    {
        delete child->widget();
        delete child;
    }

    scrollArea_->hide();
    noSelectionLabel_->show();
}

enzo::ui::Parameter* ParametersPanel::buildTemplateWidget(
    const enzo::prm::Template& templateEntry,
    enzo::nt::GeometryOperator& displayOp,
    std::vector<enzo::ui::Parameter*>& leafWidgets,
    int& maxLeftPadding
)
{
    using namespace enzo;

    if (templateEntry.getType() == prm::Type::GROUP)
    {
        ui::GroupParm* groupWidget = new ui::GroupParm(templateEntry);
        for (const prm::Template& child : templateEntry.getChildren())
        {
            ui::Parameter* childWidget =
                buildTemplateWidget(child, displayOp, leafWidgets, maxLeftPadding);
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
    case prm::Type::FLOAT:
        leafWidget = new ui::FloatSliderParm(parameter);
        break;
    case prm::Type::INT:
        leafWidget = new ui::IntSliderParm(parameter);
        break;
    case prm::Type::BOOL:
        leafWidget = ui::BoolParm::create(parameter);
        break;
    case prm::Type::XYZ:
        leafWidget = new ui::XYZParm(parameter);
        break;
    case prm::Type::STRING:
        leafWidget = new ui::StringParm(parameter);
        break;
    case prm::Type::DROPDOWN:
        leafWidget = new ui::DropdownParm(parameter);
        break;
    case prm::Type::RAMP:
        leafWidget = new ui::RampParm(parameter);
        break;
    case prm::Type::SPACER:
        leafWidget = new ui::SpacerParm(templateEntry);
        break;
    default:
        throw std::runtime_error(
            "ParametersPanel: parameter type not accounted for " +
            std::to_string(static_cast<int>(templateEntry.getType()))
        );
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

    // Drop the previous node's subscription before its widgets are deleted.
    parameterChangedConnection_.disconnect();
    leafWidgets_.clear();
    clearParameters();

    enzo::nt::GeometryOperator& displayOp = nm.getGeoOperator(opId);
    const std::vector<prm::Template>& templates = displayOp.getTemplates();

    int maxLeftPadding = 0;

    std::vector<enzo::ui::Parameter*> topWidgets;
    topWidgets.reserve(templates.size());
    for (const prm::Template& templateEntry : templates)
    {
        enzo::ui::Parameter* widget =
            buildTemplateWidget(templateEntry, displayOp, leafWidgets_, maxLeftPadding);
        if (widget) topWidgets.push_back(widget);
    }

    const int leftPadding = maxLeftPadding + 5;
    for (enzo::ui::Parameter* leaf : leafWidgets_)
        leaf->setLeftPadding(leftPadding);

    for (enzo::ui::Parameter* widget : topWidgets)
        parametersLayout_->addWidget(widget);

    // Paint the initial enabled state, then follow parameter changes for this node.
    refreshEnabledStates(displayOp);
    parameterChangedConnection_ =
        displayOp.parameterChanged.connect([this, opId](const std::string&) {
            refreshEnabledStates(enzo::nt::nm().getGeoOperator(opId));
        });

    noSelectionLabel_->hide();
    scrollArea_->show();
}

void ParametersPanel::refreshEnabledStates(enzo::nt::GeometryOperator& op)
{
    for (enzo::ui::Parameter* leaf : leafWidgets_)
    {
        leaf->setVisible(!op.isParameterHidden(leaf->getName()));
        leaf->setEnabled(op.isParameterEnabled(leaf->getName()));
    }
}
