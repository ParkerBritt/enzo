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
: parameter_{parameter}
{
    if(auto sharedParameter=parameter_.lock())
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
                FloatSliderParm* slider = new FloatSliderParm(parameter);
                mainLayout_->addWidget(slider);
                connect(slider, &FloatSliderParm::valueChanged, this, [this](bt::floatT value){this->changeValue(value, 0);}); 
                break;
            }
            case prm::Type::INT:
            {
                IntSliderParm* slider = new IntSliderParm(parameter_);
                mainLayout_->addWidget(slider);
                connect(slider, &IntSliderParm::valueChanged, this, [this](bt::intT value){this->changeValue(value, 0);}); 
                break;
            }
            case prm::Type::BOOL:
            {
                BoolSwitchParm* switchParm = new BoolSwitchParm(parameter_);
                mainLayout_->addWidget(switchParm);
                mainLayout_->addStretch();
                connect(switchParm, &BoolSwitchParm::valueChanged, this, [this](bt::intT value){this->changeValue(value, 0);}); 
                break;
            }
            case prm::Type::XYZ:
            {
                const unsigned int vectorSize = sharedParameter->getVectorSize();
                QHBoxLayout* vectorLayout = new QHBoxLayout();
                for(int i=0; i<vectorSize; i++)
                {
                    FloatSliderParm* slider = new FloatSliderParm(parameter, i);
                    vectorLayout->addWidget(slider);
                    connect(slider, &FloatSliderParm::valueChanged, this, [this, i](bt::floatT value){this->changeValue(value, i);}); 

                }
                mainLayout_->addLayout(vectorLayout);
                break;
            }
            case prm::Type::STRING:
            {
                StringParm* stringParm = new StringParm(parameter);

                connect(stringParm, &StringParm::valueChanged, this, [this](bt::String value){this->changeValue(value, 0);}); 

                mainLayout_->addWidget(stringParm);

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


void enzo::ui::FormParm::changeValue(enzo::bt::intT value, unsigned int index)
{
    if(auto sharedParameter=parameter_.lock())
    {
        sharedParameter->setInt(value, index);
    }
    else
    {
        std::cout << "ERROR: parameter no longer exists\n";

    }
     
}

void enzo::ui::FormParm::changeValue(enzo::bt::floatT value, unsigned int index)
{
    if(auto sharedParameter=parameter_.lock())
    {
        sharedParameter->setFloat(value, index);
    }
    else
    {
        std::cout << "ERROR: parameter no longer exists\n";

    }
     
}

void enzo::ui::FormParm::changeValue(enzo::bt::String value, unsigned int index)
{
    if(auto sharedParameter=parameter_.lock())
    {
        sharedParameter->setString(value, index);
    }
    else
    {
        std::cout << "ERROR: parameter no longer exists\n";

    }
     
}


