#pragma once
#include "Gui/ParametersPanel/ParametersPanel.h"
#include <Gui/UtilWidgets/Splitter.h>
#include <QBoxLayout>
#include <QMenuBar>
#include <QSplitter>
#include <QWidget>

class Viewport;
class NetworkPanel;
class GeometrySpreadsheetPanel;

class EnzoUI : public QWidget {
  public:
    EnzoUI();

  private:
    void connectSignals();
    int const DEFAULT_HANDLE_WIDTH = 2;

    QVBoxLayout *mainLayout_;
    QVBoxLayout *viewportSplitLayout_;
    Splitter *viewportSplitter_;
    Splitter *networkSplitter_;
    Splitter *spreadsheetSplitter_;

    Viewport *viewport_;
    NetworkPanel *network_;
    ParametersPanel *parametersPanel_;
    GeometrySpreadsheetPanel *geometrySpreadsheetPanel_;
};
