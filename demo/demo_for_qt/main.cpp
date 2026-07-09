#include "widget.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // print SDK Version
    qDebug() << "ViewLink SDK Version: " << GetSDKVersion();

    // initialize SDK
    VLK_Init();


    Widget w;
    w.show();

    int ret = a.exec();

    // diconnect all
    VLK_Disconnect();

    // uninitialize SDK
    VLK_UnInit();

    return ret;
}
