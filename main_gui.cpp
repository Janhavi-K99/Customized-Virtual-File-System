#include "CVFS.h"
#include "cvfs_wrapper.h"
#include "gui/mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setApplicationName("CVFS Explorer");

    CVFS_Init();

    MainWindow window;
    window.show();

    int result = app.exec();

    CVFS_Shutdown();
    return result;
}
