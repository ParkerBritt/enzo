#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "Engine/Types.h"
#include "Gui/Panels/Panel.h"

class ParametersPanel
: public Panel
{
public:
    ParametersPanel(QWidget *parent = nullptr);
public Q_SLOTS:
    void selectionChanged(enzo::nt::OpId opId);
    void clearParameters();
private:
    QVBoxLayout* mainLayout_;
    QVBoxLayout* parametersLayout_;
    QWidget* bgWidget_;

};
