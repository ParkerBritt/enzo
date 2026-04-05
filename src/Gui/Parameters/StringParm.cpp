#include "Gui/Parameters/StringParm.h"
#include "Engine/Types.h"
#include <QPainter>
#include <QPaintEvent>
#include <QLabel>
#include <iostream>
#include <qboxlayout.h>
#include <qnamespace.h>
#include <algorithm>
#include <QLineEdit>
#include <string>
#include <icecream.hpp>


enzo::ui::StringParm::StringParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent)
: QLineEdit(parent)
{
    // tells qt to style the widget even though it's a Q_OBJECT
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(24);

    parameter_ = parameter;
    if (auto parameterShared = parameter_.lock()) {
        setText(QString::fromStdString(parameterShared->evalString()));
    } else {
        throw std::bad_weak_ptr();
    }

    setProperty("type", "StringParm");
    setStyleSheet(R"(
                  QWidget[type="StringParm"]
                  {
                      border-radius: 6px;
                      border: 1px solid #383838;
                      padding: 0px 5px 0px 5px;
                  }
                  )");

    connect(this, &QLineEdit::textEdited, this, &enzo::ui::StringParm::setValueQString);
}

void enzo::ui::StringParm::setValueQString(QString value) {
    if (auto parameterShared = parameter_.lock()) {
        parameterShared->setString(value.toStdString());
    }
}
