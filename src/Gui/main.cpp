#include "Engine/Core/InstallPaths.h"
#include "Engine/Network/NodeLoader.h"
#include "Gui/Controllers/SceneController.h"
#include "Gui/Network/NetworkViewModel.h"
#include "Gui/Parameters/ParametersViewModel.h"
#include "Gui/Spreadsheet/SpreadsheetViewModel.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <argparse/argparse.hpp>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QUrl>

#include <iostream>

namespace {

#ifdef ENZO_QML_SOURCE_DIR
/// @brief Returns the QML source root and every folder beneath it.
///
/// Feature folders each hold their own QML, so the whole tree is watched.
QStringList qmlSourceDirs()
{
    const QString root = QStringLiteral(ENZO_QML_SOURCE_DIR);
    QStringList dirs{root};
    QDirIterator it(root, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
        dirs << it.next();
    return dirs;
}

/// @brief Reloads the window from disk whenever a QML file changes.
///
/// @note The watch follows folders rather than files because a save often
/// replaces the file and drops a plain file watch.
void installHotReload(QQmlApplicationEngine& engine, const QUrl& entry)
{
    auto* watcher = new QFileSystemWatcher(&engine);
    watcher->addPaths(qmlSourceDirs());

    auto reload = [&engine, entry, watcher]() {
        const QList<QObject*> previousRoots = engine.rootObjects();
        engine.clearComponentCache();
        engine.load(entry);
        for (QObject* root : previousRoots)
            root->deleteLater();

        // Re-arm any folders a save may have replaced.
        const QStringList watched = watcher->directories();
        for (const QString& dir : qmlSourceDirs())
            if (!watched.contains(dir)) watcher->addPath(dir);
    };

    QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, &engine, reload);
}
#endif

} // namespace

namespace {

/// @brief Returns the scene file path given on the command line, empty when none was.
///
/// @note Exits the process on a bad argument, and on --help or --version,
/// before any of the interface is built.
QString parseCommandLine(int argc, char** argv)
{
    argparse::ArgumentParser parser("enzo", ENZO_VERSION);
    parser.add_description("Procedural 3D modelling.");
    parser.add_argument("scene")
        .help("an .enzo scene to open on startup")
        .nargs(argparse::nargs_pattern::optional)
        .default_value(std::string{});

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n\n" << parser;
        std::exit(1);
    }

    return QString::fromStdString(parser.get<std::string>("scene"));
}

} // namespace

namespace {

/// @brief Registers the bundled Public Sans and Inconsolata weights.
void loadFonts()
{
    QDirIterator it(
        QString::fromStdString((enzo::getStaticDir() / "fonts").string()),
        {"*.ttf"},
        QDir::Files,
        QDirIterator::Subdirectories
    );
    while (it.hasNext())
        QFontDatabase::addApplicationFont(it.next());
}

} // namespace

int main(int argc, char** argv)
{
    const QString scenePath = parseCommandLine(argc, argv);

    // The viewport composites a legacy OpenGL renderer, so the scene graph runs
    // on the OpenGL backend.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // Icons recolour their SVG markup through a local file XMLHttpRequest.
    qputenv("QML_XHR_ALLOW_FILE_READ", "1");

    // Send Qt and QML debug logging to the terminal.
    qputenv("QT_LOGGING_RULES", "default.debug=true;qml.debug=true;js.debug=true");

    // The viewport renderer needs a 3.3 core context for instanced points.
    // Multisampling smooths the scene graph geometry, including the link curves.
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("Enzo");
    app.setApplicationName("Enzo");

    loadFonts();
    QQuickStyle::setStyle("Basic");

    // The view-models bridge the engine to QML.
    enzo::ui::SpreadsheetViewModel spreadsheet;
    enzo::ui::NetworkViewModel network;
    enzo::ui::ViewportViewModel viewport;
    enzo::ui::ParametersViewModel parameters;
    enzo::ui::SceneController scene;

    enzo::nt::NodeLoader::loadNodes();

    if (!scenePath.isEmpty())
    {
        if (!QFileInfo::exists(scenePath))
        {
            std::cerr << "no such file: " << scenePath.toStdString() << "\n";
            return 1;
        }

        try
        {
            scene.openPath(scenePath);
        }
        catch (const std::exception& error)
        {
            std::cerr << "could not open " << scenePath.toStdString() << ": " << error.what()
                      << "\n";
            return 1;
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("spreadsheet", &spreadsheet);
    engine.rootContext()->setContextProperty("network", &network);
    engine.rootContext()->setContextProperty("viewport", &viewport);
    engine.rootContext()->setContextProperty("parameters", &parameters);
    engine.rootContext()->setContextProperty("scene", &scene);

    // A run from a build directory loads QML straight from the source tree and
    // reloads on edit. An installed run loads the module compiled into the binary.
    const QString qmlSourceDir = QStringLiteral(ENZO_QML_SOURCE_DIR);
    if (QFileInfo::exists(qmlSourceDir + "/App.qml"))
    {
        const QUrl entry = QUrl::fromLocalFile(qmlSourceDir + "/App.qml");
        engine.addImportPath(qmlSourceDir);
        installHotReload(engine, entry);
        engine.load(entry);
    }
    else
    {
        engine.loadFromModule("Enzo", "App");
    }

    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
