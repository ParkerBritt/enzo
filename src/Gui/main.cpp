#include <QApplication>
#include <QFontDatabase>
#include <QPushButton>
#include <QSurfaceFormat>
#include "Engine/Operator/OperatorTable.h"

#include "Interface.h"

int main(int argc, char **argv)
{
    // set up rendering
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    // init plugins
    enzo::op::OperatorTable::initPlugins();

    QApplication app (argc, argv);
    app.setOrganizationName("Enzo");
    app.setApplicationName("Enzo");

    // load fonts
    QFontDatabase::addApplicationFont(":/fonts/Rubik/Rubik-VariableFont_wght.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik/Rubik-Italic-VariableFont_wght.ttf");
    QFont appFont("Rubik");
    appFont.setPointSize(9);
    app.setFont(appFont);

    EnzoUI interface;
    interface.show();

    return app.exec();
}
