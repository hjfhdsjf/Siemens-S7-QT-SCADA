#include "s7_tester.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    S7_Tester w;
    w.show();
    return a.exec();
}
