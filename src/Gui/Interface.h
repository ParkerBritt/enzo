#pragma once
#include <QWidget>
#include <QBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <Gui/UtilWidgets/Splitter.h>
#include "Gui/ParametersPanel/ParametersPanel.h"

class Viewport;
class NetworkPanel;
class GeometrySpreadsheetPanel;

class EnzoUI
: public QWidget
{
    public:
        EnzoUI();

    private:
        void connectSignals();

        QVBoxLayout* mainLayout_;
        QVBoxLayout* viewportSplitLayout_;
        Splitter* viewportSplitter_;
        Splitter* networkSplitter_;
        Splitter* spreadsheetSplitter_;

        Viewport* viewport_;
        NetworkPanel* network_;
        ParametersPanel* parametersPanel_;
        GeometrySpreadsheetPanel* geometrySpreadsheetPanel_;
};
