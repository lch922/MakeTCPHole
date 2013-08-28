#include <QApplication>
#include "clientform.h"

int main(int argc, char** argv)
{
    QApplication a(argc, argv);
    ClientForm f;
    f.show();
    return a.exec();
}
