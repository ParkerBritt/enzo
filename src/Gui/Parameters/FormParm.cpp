#include "Gui/Parameters/FormParm.h"
#include "Gui/Parameters/IntSliderParm.h"
#include "Engine/Types.h"
#include "Gui/Parameters/FloatSliderParm.h"
#include "Gui/Parameters/BoolSwitchParm.h"
#include "Gui/Parameters/StringParm.h"
#include <qboxlayout.h>
#include <QLabel>
#include <iostream>
#include <qlabel.h>
#include <string>
#include <icecream.hpp>


enzo::ui::FormParm::FormParm(std::weak_ptr<prm::Parameter> parameter)
{
    if(auto sharedParameter=parameter.lock())
    {
        std::string name = sharedParameter->getLabel();
        label_ = new QLabel(QString::fromStdString(name+":"));
        label_->setStyleSheet("QLabel{background: none}");
        label_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        mainLayout_ = new QHBoxLayout();
        mainLayout_->addWidget(label_);
        mainLayout_->setContentsMargins(0,0,0,0);

        switch(sharedParameter->getType())
        {
            case prm::Type::FLOAT:
            {
                mainLayout_->addWidget(new FloatSliderParm(parameter));
                break;
            }
            case prm::Type::INT:
            {
                mainLayout_->addWidget(new IntSliderParm(parameter));
                break;
            }
            case prm::Type::BOOL:
            {
                mainLayout_->addWidget(new BoolSwitchParm(parameter));
                mainLayout_->addStretch();
                break;
            }
            case prm::Type::XYZ:
            {
                const unsigned int vectorSize = sharedParameter->getVectorSize();
                QHBoxLayout* vectorLayout = new QHBoxLayout();
                for(unsigned int i = 0; i < vectorSize; i++)
                    vectorLayout->addWidget(new FloatSliderParm(parameter, i));
                mainLayout_->addLayout(vectorLayout);
                break;
            }
            case prm::Type::STRING:
            {
                mainLayout_->addWidget(new StringParm(parameter));
                break;
            }
            default:
                throw std::runtime_error("Form parm: paremeter type not accounted for " + std::to_string(static_cast<int>(sharedParameter->getType())));
        }




        setFixedHeight(24);
        setProperty("class", "Parameter");
        setStyleSheet(".Parameter { background-color: none;}");
        setLayout(mainLayout_);

    }

}

int enzo::ui::FormParm::getLeftPadding()
{

    return label_->minimumSizeHint().width();
}

void enzo::ui::FormParm::setLeftPadding(int padding)
{
    label_->setFixedWidth(padding);
}



