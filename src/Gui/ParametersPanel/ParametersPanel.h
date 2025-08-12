#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "Engine/Types.h"

class ParametersPanel
: public QWidget
{
public:
    ParametersPanel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
public Q_SLOTS:
    void selectionChanged(enzo::nt::OpId opId);
private:
    QVBoxLayout* mainLayout_;
    QVBoxLayout* parametersLayout_;
    QWidget* bgWidget_;

};
