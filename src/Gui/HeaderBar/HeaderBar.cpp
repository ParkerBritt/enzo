#include "HeaderBar.h"
#include "Engine/Serializer/Serializer.h"
#include "Engine/Network/NetworkManager.h"
#include <qaction.h>
#include <iostream>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>

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
    QAction* fileSaveAsAction = new QAction("Save As...");
    QAction* fileImportEnzoAction = new QAction("Enzo");
    QAction* fileNewAction = new QAction("New");

    // Add Actions
    fileMenu->addAction(fileNewAction);
    fileMenu->addAction(fileOpenAction);
    fileMenu->addMenu("Open Recent");
    fileMenu->addMenu(fileImportMenu);
    fileMenu->addAction(fileSaveAction);
    fileMenu->addAction(fileSaveAsAction);
    fileMenu->addAction("Quit");

    fileImportMenu->addAction(fileImportEnzoAction);

    // Connect actions
    connect(fileNewAction, &QAction::triggered, this, &HeaderBar::onFileNewClicked);
    connect(fileOpenAction, &QAction::triggered, this, &HeaderBar::onFileOpenClicked);
    connect(fileSaveAsAction, &QAction::triggered, this, &HeaderBar::onFileSaveAsClicked);

    // Edit sub menu
    QMenu* editMenu = header->addMenu("Edit");

    // Window sub menu
    QMenu* windowMenu = header->addMenu("Window");

    // Help sub menu
    QMenu* helpMenu = header->addMenu("Help");
    helpMenu->addAction("Info");

    mainLayout_->addWidget(header);
}

void HeaderBar::onFileNewClicked()
{
    enzo::nt::nm().clear();
}

void HeaderBar::onFileOpenClicked()
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QString fileName = QFileDialog::getOpenFileName(this,
    tr("Open File"), homeDir, tr("Enzo files (*.enzo);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    if (!QFile::exists(fileName))
    {
        std::cerr << "File does not exist: " << fileName.toStdString() << "\n";
        return;
    }

    enzo::nt::Serializer serializer;
    serializer.load( enzo::nt::nm(), fileName.toStdString());
}

void HeaderBar::onFileSaveAsClicked()
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString defaultSavePath = QDir(homeDir).filePath("Untitled.enzo");

    QString fileName = QFileDialog::getSaveFileName(this,
    tr("Save File"), defaultSavePath, tr("Enzo files (*.enzo);;All Files (*)"));

    enzo::nt::Serializer serializer;
    serializer.save( enzo::nt::nm(), fileName.toStdString());
    std::cout << "SAVING\n";

}
