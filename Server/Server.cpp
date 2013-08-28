#include <QApplication>
#include "serverform.h"
int main(int argc, char** argv)
{
    QApplication a(argc, argv);
    ServerForm f;
    f.show();
    return a.exec();
}
