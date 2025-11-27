
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 在Qt5/Qt6中，默认使用UTF-8编码，不需要额外设置
    // 但可以确保Qt使用系统默认的编码设置
    // QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    clipboard::MainWindow window;
    window.show();

    return app.exec();
}
