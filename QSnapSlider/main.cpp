#include "snapslidertest.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SnapSliderTest w;
    w.show();
    return a.exec();
}
