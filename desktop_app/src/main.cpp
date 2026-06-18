#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("prj33");
    QApplication::setApplicationName("desktop_app");

    MainWindow w;
    w.show();

    return app.exec();
}
