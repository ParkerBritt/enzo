#include "Gui/Controllers/SceneController.h"

#include "Engine/Network/NetworkManager.h"
#include "Engine/Serializer/Serializer.h"

#include <QFileInfo>
#include <QSettings>

namespace {

// How many files the recent list keeps.
constexpr int maxRecentFiles = 10;

} // namespace

enzo::ui::SceneController::SceneController(QObject* parent) : QObject(parent) {}

QString enzo::ui::SceneController::currentFileName() const
{
    if (currentFilePath_.isEmpty()) return {};
    return QFileInfo(currentFilePath_).fileName();
}

bool enzo::ui::SceneController::hasCurrentFile() const { return !currentFilePath_.isEmpty(); }

QStringList enzo::ui::SceneController::recentFiles() const
{
    return QSettings().value("recentFiles").toStringList();
}

void enzo::ui::SceneController::newScene()
{
    enzo::nt::nm().clear();
    currentFilePath_.clear();
    Q_EMIT currentFileChanged();
}

void enzo::ui::SceneController::open(const QUrl& fileUrl) { openPath(fileUrl.toLocalFile()); }

void enzo::ui::SceneController::openPath(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    readFrom(filePath);
}

void enzo::ui::SceneController::save()
{
    if (currentFilePath_.isEmpty()) return;
    writeTo(currentFilePath_);
}

void enzo::ui::SceneController::saveAs(const QUrl& fileUrl)
{
    const QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) return;
    writeTo(filePath);
}

void enzo::ui::SceneController::writeTo(const QString& filePath)
{
    enzo::nt::Serializer().save(enzo::nt::nm(), filePath.toStdString());
    currentFilePath_ = filePath;
    Q_EMIT currentFileChanged();
    rememberRecent(filePath);
}

void enzo::ui::SceneController::readFrom(const QString& filePath)
{
    enzo::nt::Serializer().load(enzo::nt::nm(), filePath.toStdString());
    currentFilePath_ = filePath;
    Q_EMIT currentFileChanged();
    rememberRecent(filePath);
}

void enzo::ui::SceneController::rememberRecent(const QString& filePath)
{
    QSettings settings;
    QStringList recent = settings.value("recentFiles").toStringList();
    recent.removeAll(filePath);
    recent.prepend(filePath);
    settings.setValue("recentFiles", recent.mid(0, maxRecentFiles));
    Q_EMIT recentFilesChanged();
}
