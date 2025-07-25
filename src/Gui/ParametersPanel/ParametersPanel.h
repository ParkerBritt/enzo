#pragma once

#include <QWidget>
#include <QVBoxLayout>

class ParametersPanel
: public QWidget
{
public:
    ParametersPanel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
public Q_SLOTS:
    void selectionChanged();
private:
    QVBoxLayout* mainLayout_;
    QVBoxLayout* parametersLayout_;
    QWidget* bgWidget_;

};
