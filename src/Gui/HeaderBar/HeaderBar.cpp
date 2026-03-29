#include "HeaderBar.h"
#include "Engine/Serializer/Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include <qaction.h>
#include <iostream>

HeaderBar::HeaderBar()
{
    mainLayout_ = new QHBoxLayout(this);
    setLayout(mainLayout_);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    

    QMenuBar* header = new QMenuBar();
    header->setStyleSheet(R"(
    QMenuBar::item
    {
        border-radius: 6px;
        padding: 2px 6px;
    }
    QMenuBar::item:selected
    {
        background: #282828;
    }

    QMenu::item
    {
        border-radius: 6px;
        padding: 2px 6px;
        height: 24px;
        width: 150px;
    }
    QMenu::item:selected
    {
        background: #282828;
    }
    )");

    // File menu
    QMenu* fileMenu = header->addMenu("File");
    fileMenu->addAction("New");
    fileMenu->addAction("Open");
    fileMenu->addMenu("Open Recent");
    QAction* saveAction = fileMenu->addAction("Save");
    fileMenu->addAction("Save As...");
    fileMenu->addAction("Quit");

    connect(saveAction, &QAction::triggered, this, &HeaderBar::onSaveClicked);

    // Edit menu
    QMenu* editMenu = header->addMenu("Edit");

    // Window menu
    QMenu* windowMenu = header->addMenu("Window");

    // Help menu
    QMenu* helpMenu = header->addMenu("Help");
    helpMenu->addAction("Info");

    mainLayout_->addWidget(header);
}

void HeaderBar::onSaveClicked()
{
    enzo::nt::Serializer serializer(
enzo::nt::nm()
            );
    std::cout << "SAVING\n";

}
