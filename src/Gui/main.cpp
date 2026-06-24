#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

namespace
{

/// @brief Reloads the window from disk whenever a QML file changes.
///
/// @note The watch follows the folder rather than its files because a save
/// often replaces the file and drops a plain file watch.
#ifdef ENZO_QML_SOURCE_DIR
void installHotReload(QQmlApplicationEngine& engine, const QUrl& entry)
{
    const QString sourceDir = QStringLiteral(ENZO_QML_SOURCE_DIR);

    auto* watcher = new QFileSystemWatcher(&engine);
    watcher->addPath(sourceDir);

    auto reload = [&engine, entry, sourceDir, watcher]() {
        const QList<QObject*> previousRoots = engine.rootObjects();
        engine.clearComponentCache();
        engine.load(entry);
        for (QObject* root : previousRoots)
            root->deleteLater();

        // Re-arm the watch in case the save replaced the folder contents.
        if (!watcher->directories().contains(sourceDir))
            watcher->addPath(sourceDir);
    };

    QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, &engine, reload);
}
#endif

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Enzo");
    app.setApplicationName("Enzo");

    QQuickStyle::setStyle("Basic");

    QQmlApplicationEngine engine;

#ifdef ENZO_QML_SOURCE_DIR
    // Dev builds load QML straight from the source tree and reload on edit.
    const QUrl entry = QUrl::fromLocalFile(QStringLiteral(ENZO_QML_SOURCE_DIR) + "/Main.qml");
    engine.addImportPath(QStringLiteral(ENZO_QML_SOURCE_DIR));
    installHotReload(engine, entry);
    engine.load(entry);
#else
    // Release loads the module compiled into the binary.
    engine.loadFromModule("Enzo", "Main");
#endif

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
