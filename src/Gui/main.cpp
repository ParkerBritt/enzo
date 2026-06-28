#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Gui/Network/NetworkViewModel.h"
#include "Gui/Parameters/ParametersViewModel.h"
#include "Gui/Spreadsheet/SpreadsheetViewModel.h"
#include "Gui/Style/Theme.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QUrl>

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

/// @brief Builds a one node grid network and selects it.
///
/// Stands in for real scene loading. Selecting the node makes the engine emit
/// the selection signal the spreadsheet view-model listens for.
void buildSampleNetwork()
{
    enzo::op::OperatorTable::initPlugins();

    auto& network = enzo::nt::nm();
    auto create = [&](const char* type, enzo::Vector2 position) {
        return network
            .createOperator(enzo::op::OperatorTable::getOpInfo(type).value(), "", position);
    };

    const enzo::nt::OpId gridId = create("grid", {0.f, 0.f});
    const enzo::nt::OpId transformId = create("transform", {200.f, 120.f});
    create("cube", {-180.f, 140.f});
    create("circle", {40.f, -160.f});

    // Feed the grid's output into the transform so there is a wire to draw.
    network.connectNodes(gridId, 0, transformId, 0);

    network.cookOp(gridId);
    network.setSelectedNodes({gridId});
    network.setPrimaryNode(gridId);
    network.setDisplayOp(gridId);
}

} // namespace

namespace {

/// @brief Registers the bundled Public Sans and Inconsolata weights.
void loadFonts()
{
    QDirIterator it(
        QStringLiteral(ENZO_DEV_FONTS_DIR),
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
    buildSampleNetwork();

    enzo::ui::Theme theme;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Theme", &theme);
    engine.rootContext()->setContextProperty("spreadsheet", &spreadsheet);
    engine.rootContext()->setContextProperty("network", &network);
    engine.rootContext()->setContextProperty("viewport", &viewport);
    engine.rootContext()->setContextProperty("parameters", &parameters);

#ifdef ENZO_QML_SOURCE_DIR
    // Dev builds load QML straight from the source tree and reload on edit.
    const QUrl entry = QUrl::fromLocalFile(QStringLiteral(ENZO_QML_SOURCE_DIR) + "/App.qml");
    engine.addImportPath(QStringLiteral(ENZO_QML_SOURCE_DIR));
    installHotReload(engine, entry);
    engine.load(entry);
#else
    // Release loads the module compiled into the binary.
    engine.loadFromModule("Enzo", "App");
#endif

    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
