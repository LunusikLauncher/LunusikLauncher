#include <QApplication>
#include <QIcon>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("no.id");
    QCoreApplication::setApplicationName("LunusikLauncher");
    QIcon icon(":/icons/LLL.ico");
    QSplashScreen* splash = new QSplashScreen(icon.pixmap(120, 120)); // Статичное фото для начала
    splash->show();
    splash->showMessage("Launching...", Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    app.processEvents();
    MainWindow w(splash);

    w.setWindowIcon(icon); 

    return app.exec();
}