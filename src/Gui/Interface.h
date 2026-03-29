#pragma once
#include <QWidget>
#include <QBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <Gui/UtilWidgets/Splitter.h>

class EnzoUI
: public QWidget
{
    public:
        EnzoUI(); 
        
    private:
        QVBoxLayout* mainLayout_;
        QVBoxLayout* viewportSplitLayout_;
        Splitter* viewportSplitter_;
        Splitter* networkSplitter_;
        Splitter* spreadsheetSplitter_;


};
