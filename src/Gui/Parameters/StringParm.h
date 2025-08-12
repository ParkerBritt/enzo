#pragma once
#include "Engine/Types.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include "Engine/Parameter/Parameter.h"

namespace enzo::ui
{

class StringParm
: public QLineEdit
{
    Q_OBJECT
public:
    StringParm(std::weak_ptr<prm::Parameter>, QWidget *parent = nullptr);
    void setValue(bt::String value);
    void setValueQString(QString value);
    std::weak_ptr<prm::Parameter> parameter_;

Q_SIGNALS:
    void valueChanged(bt::String value); 

private:
    bt::String value_;


protected:

};

}

