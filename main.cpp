#include "zcode2gcode.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ZCode2GCode w;
    w.show();

    return a.exec();
}
