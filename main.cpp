#include "cyberdom.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CyberDom w;
    w.show();
    return a.exec();
}
