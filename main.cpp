#include "chatwindow.h"
#include <QApplication>

int main(int argc,char *argv[]) {
    QApplication a(argc,argv);
    qRegisterMetaType<Profile>("Profile");
    //qRegisterMetaType<QtNotification>("QtNotification",0);
    QApplication::setStyle(QStyleFactory::create("Windows"));
    QApplication::setPalette(QApplication::style()->standardPalette());
    QCoreApplication::setOrganizationName("ProSoftworks");
    QCoreApplication::setOrganizationDomain("areallylongurlthatsnotreallyreal.com");
    QCoreApplication::setApplicationName("Local Network Chat");
    chatwindow w;
    w.show();
    return a.exec();
}
