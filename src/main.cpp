#include <QApplication>
#include <QIcon>
#include "MainWindow.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("TransactionPhotoPro");
    app.setApplicationDisplayName("معاملات Photo Pro");
    app.setOrganizationName("TransactionPhotoPro");
    app.setLayoutDirection(Qt::RightToLeft);
    app.setWindowIcon(QIcon(":/icon.png"));
    MainWindow w;
    w.show();
    return app.exec();
}
