#pragma once
#include "Engine/Types.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>

namespace enzo::ui
{

class StringParm
: public QLineEdit
{
    Q_OBJECT
public:
    StringParm(bt::String value, QWidget *parent = nullptr);
    void setValue(bt::String value);
    void setValueQString(QString value);

Q_SIGNALS:
    void valueChanged(bt::String value); 

private:
    bt::String value_;


protected:

};

}

