#include "HeaderBar.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Serializer/Serializer.h"
#include "Gui/UtilWidgets/MenuBar.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <iostream>

using Entry = enzo::ui::Menu::Entry;

HeaderBar::HeaderBar()
{
    mainLayout_ = new QHBoxLayout(this);
    mainLayout_->setContentsMargins(4, 2, 4, 2);
    mainLayout_->setSpacing(2);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto* menuBar = new enzo::ui::MenuBar(this);
    menuBar->addMenu("File", [this] { return fileEntries(); });
    menuBar->addMenu("Edit", [] { return std::vector<Entry>{}; });
    menuBar->addMenu("Window", [] { return std::vector<Entry>{}; });
    menuBar->addMenu("Help", [] { return std::vector<Entry>{{.text = "Info"}}; });

    mainLayout_->addWidget(menuBar);
    mainLayout_->addStretch();
}

std::vector<Entry> HeaderBar::fileEntries()
{
    return {
        {.text = "New", .action = [this] { onFileNewClicked(); }},
        {.text = "Open", .action = [this] { onFileOpenClicked(); }},
        {.text = "Open Recent", .children = recentFileEntries()},
        {.text = "Import", .children = {{.text = "Enzo"}}},
        {.text = "Save", .action = [this] { onFileSaveClicked(); }},
        {.text = "Save As...", .action = [this] { onFileSaveAsClicked(); }},
        {.text = "Quit", .action = [] { QCoreApplication::quit(); }},
    };
}

std::vector<Entry> HeaderBar::recentFileEntries()
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentFiles").toStringList();

    if (recentFiles.isEmpty())
    {
        return {{.text = "No Recent Files"}};
    }

    std::vector<Entry> entries;
    for (const QString& filePath : recentFiles)
    {
        entries.push_back({.text = QFileInfo(filePath).fileName(), .action = [this, filePath] {
                               openFile(filePath);
                           }});
    }
    return entries;
}

void HeaderBar::onFileNewClicked() { enzo::nt::nm().clear(); }

void HeaderBar::onFileOpenClicked()
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open File"),
        homeDir,
        tr("Enzo files (*.enzo);;All Files (*)")
    );

    if (fileName.isEmpty()) return;

    openFile(fileName);
}

void HeaderBar::onFileSaveAsClicked()
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString defaultSavePath = QDir(homeDir).filePath("Untitled.enzo");

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save File"),
        defaultSavePath,
        tr("Enzo files (*.enzo);;All Files (*)")
    );

    saveFile(filePath);
}

void HeaderBar::onFileSaveClicked()
{
    if (currentFilePath_.isEmpty())
    {
        onFileSaveAsClicked();
        return;
    }

    saveFile(currentFilePath_);
}

void HeaderBar::saveFile(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        return;
    }

    enzo::nt::Serializer serializer;
    serializer.save(enzo::nt::nm(), filePath.toStdString());
    currentFilePath_ = filePath;
    addRecentFile(filePath);
}

void HeaderBar::openFile(const QString& filePath)
{
    if (!QFile::exists(filePath))
    {
        std::cerr << "File does not exist: " << filePath.toStdString() << "\n";
        return;
    }

    enzo::nt::Serializer serializer;
    serializer.load(enzo::nt::nm(), filePath.toStdString());
    currentFilePath_ = filePath;
    addRecentFile(filePath);
}

void HeaderBar::addRecentFile(const QString& filePath)
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    recentFiles.removeAll(filePath);
    recentFiles.prepend(filePath);
    recentFiles = recentFiles.mid(0, 10);
    settings.setValue("recentFiles", recentFiles);
}
