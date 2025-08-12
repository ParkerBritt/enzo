#pragma once

#include "Engine/Operator/OpInfo.h"
#include "Engine/Types.h"
#include <QWidget>
#include <QHBoxLayout>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <QLabel>

class GeoSheetModeButton
: public QPushButton
{
public:
    GeoSheetModeButton(QWidget *parent = nullptr);
private:
    QHBoxLayout* mainLayout_;
protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool hovered_ = false;
};

class GeoSheetMenuBarModeSelection
: public QWidget
{
public:
    GeoSheetMenuBarModeSelection(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    GeoSheetModeButton* pointButton;
    GeoSheetModeButton* vertexButton;
    GeoSheetModeButton* primitiveButton;
    GeoSheetModeButton* globalButton;
private:
    QHBoxLayout* mainLayout_;
    QButtonGroup modeButtonGroup_;
};

class GeometrySpreadsheetMenuBar
: public QWidget
{
public:
    GeometrySpreadsheetMenuBar(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    GeoSheetMenuBarModeSelection* modeSelection;
    void setNode(enzo::nt::OpId opId);
private:
    QHBoxLayout* mainLayout_;
    QLabel* nodeLabel_;
};

