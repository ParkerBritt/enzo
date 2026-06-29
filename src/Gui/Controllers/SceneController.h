#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace enzo::ui {

/// @brief File side of the menu bar: new, open and save of the scene.
///
/// Wraps the engine serializer and remembers the current file so a plain Save
/// writes back without a dialog. Open and Save As take a url from the QML file
/// dialog. The recent list is kept in the application settings.
///
/// e.g.
///   scene.hasCurrentFile ? scene.save() : saveDialog.open()
class SceneController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileChanged)
    Q_PROPERTY(bool hasCurrentFile READ hasCurrentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)

  public:
    explicit SceneController(QObject* parent = nullptr);

    QString currentFileName() const;
    bool hasCurrentFile() const;
    QStringList recentFiles() const;

    /// @brief Clears the network back to an empty scene.
    Q_INVOKABLE void newScene();

    /// @brief Loads a scene from a file dialog url.
    Q_INVOKABLE void open(const QUrl& fileUrl);

    /// @brief Loads a scene from a plain file path, used by the recent list.
    Q_INVOKABLE void openPath(const QString& filePath);

    /// @brief Writes the scene back to the current file.
    /// @note Does nothing without a current file. The caller opens Save As instead.
    Q_INVOKABLE void save();

    /// @brief Writes the scene to a file dialog url and adopts it as current.
    Q_INVOKABLE void saveAs(const QUrl& fileUrl);

  Q_SIGNALS:
    void currentFileChanged();
    void recentFilesChanged();

  private:
    void writeTo(const QString& filePath);
    void readFrom(const QString& filePath);
    void rememberRecent(const QString& filePath);

    QString currentFilePath_;
};

} // namespace enzo::ui
