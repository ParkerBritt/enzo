#include "HeaderBar.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Serializer/Serializer.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <QToolButton>
#include <iostream>

using Entry = enzo::ui::Menu::Entry;

HeaderBar::HeaderBar()
{
    mainLayout_ = new QHBoxLayout(this);
    mainLayout_->setContentsMargins(4, 2, 4, 2);
    mainLayout_->setSpacing(2);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    setStyleSheet(R"(
    QToolButton
    {
        border: none;
        border-radius: 6px;
        padding: 2px 8px;
        color: #B3B3B3;
        background: transparent;
    }
    QToolButton:hover
    {
        background: #282828;
    }
    )");

    // One menu instance is repopulated by whichever button opens it
    menu_ = new enzo::ui::Menu(this);

    addMenuButton("File", [this] { return fileEntries(); });
    addMenuButton("Edit", [] { return std::vector<Entry>{}; });
    addMenuButton("Window", [] { return std::vector<Entry>{}; });
    addMenuButton("Help", [] { return std::vector<Entry>{{.text = "Info"}}; });

    mainLayout_->addStretch();
}

void HeaderBar::addMenuButton(
    const QString& title,
    std::function<std::vector<Entry>()> build
)
{
    QToolButton* button = new QToolButton(this);
    button->setText(title);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);

    connect(button, &QToolButton::clicked, this, [this, button, build] {
        std::vector<Entry> entries = build();
        if (entries.empty()) return;
        menu_->setEntries(std::move(entries));
        menu_->popup(button->mapToGlobal(QPoint(0, button->height())));
    });

    mainLayout_->addWidget(button);
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
        entries.push_back(
            {.text = QFileInfo(filePath).fileName(),
             .action = [this, filePath] { openFile(filePath); }}
        );
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
