#include "Gui/ParametersPanel/ParametersPanel.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Gui/Parameters/AbstractSliderParm.h"
#include "Gui/Parameters/AbstractFormParm.h"
#include "Gui/Parameters/FloatParm.h"
#include "Engine/Network/NetworkManager.h"
#include <memory>
#include <qboxlayout.h>
#include <QSpinBox>
#include <qnamespace.h>
#include <qwidget.h>
#include <QLineEdit>

ParametersPanel::ParametersPanel(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    mainLayout_ = new QVBoxLayout();
    parametersLayout_ = new QVBoxLayout();
    parametersLayout_->setAlignment(Qt::AlignTop);
    bgWidget_ = new QWidget();
    bgWidget_->setLayout(parametersLayout_);

    bgWidget_->setObjectName("ParametersPanelBg");
    bgWidget_->setStyleSheet(R"(
           QWidget#ParametersPanelBg {
                background-color: #282828;
               border-radius: 10px;
           }
    )"
      );

    mainLayout_->addWidget(bgWidget_);

    // parametersLayout_->addWidget(new enzo::ui::AbstractFormParm());
    // parametersLayout_->addWidget(new enzo::ui::AbstractFormParm());
    // parametersLayout_->addWidget(new enzo::ui::AbstractFormParm());
    // parametersLayout_->addWidget(new enzo::ui::AbstractFormParm());


    setLayout(mainLayout_);
}

void ParametersPanel::selectionChanged()
{
    enzo::nt::NetworkManager& nm = enzo::nt::nm();
    std::optional<enzo::nt::OpId> displayOpId = nm.getDisplayOp();

    if(!displayOpId.has_value()) return;

    // clear layout safely
    QLayoutItem *child;
    while ((child = parametersLayout_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    enzo::nt::GeometryOperator& displayOp = nm.getGeoOperator(displayOpId.value());
    auto parameters = displayOp.getParameters();
    for(auto parameter : parameters)
    {
        parametersLayout_->addWidget(new enzo::ui::AbstractFormParm(parameter));
    }
}

