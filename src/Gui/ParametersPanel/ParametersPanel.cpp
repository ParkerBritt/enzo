#include "Gui/ParametersPanel/ParametersPanel.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Types.h"
#include "Gui/Parameters/FloatSliderParm.h"
#include "Gui/Parameters/FormParm.h"
#include "Engine/Network/NetworkManager.h"
#include <memory>
#include <qboxlayout.h>
#include <QSpinBox>
#include <qnamespace.h>
#include <qwidget.h>
#include <QLineEdit>
#include <stdexcept>

ParametersPanel::ParametersPanel(QWidget *parent)
: Panel(parent)
{
    mainLayout_ = new QVBoxLayout();
    parametersLayout_ = new QVBoxLayout();
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

void ParametersPanel::selectionChanged(enzo::nt::OpId opId)
{
    using namespace enzo;
    enzo::nt::NetworkManager& nm = enzo::nt::nm();
    const enzo::nt::OpId displayOpId = opId;

    clearParameters();

    enzo::nt::GeometryOperator& displayOp = nm.getGeoOperator(displayOpId);
    auto parameters = displayOp.getParameters();

    std::vector<enzo::ui::FormParm*> parameterWidgets;
    parameterWidgets.reserve(parameters.size());

    int maxLeftPadding = 0;

    for(auto parameter : parameters)
    {
        auto parameterShared = parameter.lock();
        if(!parameterShared) throw std::runtime_error("Failed to lock parameter");

        enzo::ui::FormParm* parameterWidget = new enzo::ui::FormParm(parameter);
        int leftPadding = parameterWidget->getLeftPadding();
        if(leftPadding > maxLeftPadding) maxLeftPadding = leftPadding;

        parameterWidgets.push_back(parameterWidget);
    }

    const int leftPadding = maxLeftPadding + 5;

    for(auto parameterWidget : parameterWidgets)
    {
        parameterWidget->setLeftPadding(leftPadding);
        parametersLayout_->addWidget(parameterWidget);
    }
}

