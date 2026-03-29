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
    QMenu* fileImportMenu = new QMenu("Import");

    // Create actions
    QAction* fileOpenAction = new QAction("Open");
    QAction* fileSaveAction = new QAction("Save");
    QAction* fileImportEnzoAction = new QAction("Enzo");

    // Add Actions
    fileMenu->addAction("New");
    fileMenu->addAction(fileOpenAction);
    fileMenu->addMenu("Open Recent");
    fileMenu->addMenu(fileImportMenu);
    fileMenu->addAction(fileSaveAction);
    fileMenu->addAction("Save As...");
    fileMenu->addAction("Quit");

    fileImportMenu->addAction(fileImportEnzoAction);

    // Connect actions
    connect(fileOpenAction, &QAction::triggered, this, &HeaderBar::onOpenClicked);
    connect(fileSaveAction, &QAction::triggered, this, &HeaderBar::onSaveClicked);

    // Edit sub menu
    QMenu* editMenu = header->addMenu("Edit");

    // Window sub menu
    QMenu* windowMenu = header->addMenu("Window");

    // Help sub menu
    QMenu* helpMenu = header->addMenu("Help");
    helpMenu->addAction("Info");

    mainLayout_->addWidget(header);
}

void HeaderBar::onOpenClicked()
{
    enzo::nt::Serializer serializer;
    serializer.load(
enzo::nt::nm()
            );
    std::cout << "loading\n";


}

void HeaderBar::onSaveClicked()
{
    enzo::nt::Serializer serializer;
    serializer.save(
enzo::nt::nm()
            );
    std::cout << "SAVING\n";

}
