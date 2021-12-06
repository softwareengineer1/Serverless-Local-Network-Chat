#include "QtAndroidNotifier.h"

#include <QVariant>
#include <QtAndroid>

//------------------------------------------------------------------------------

bool QtAndroidNotifier::show(const QVariant &notificationParameters)
{
    QVariantMap parameters=notificationParameters.toMap();
    QString caption=parameters.value("caption", "").toString();
    QString title=parameters.value("title", "").toString();
    id=parameters.value("id", QRandomGenerator::global()->bounded(100000,100000000)).toInt();
    return show(caption,title);
}
//------------------------------------------------------------------------------

bool QtAndroidNotifier::show(const QString title,const QString message) {
    Q_UNUSED(title);
    Q_UNUSED(message);
//    QAndroidJniObject jni_caption=QAndroidJniObject::fromString(title);
//    QAndroidJniObject jni_title=QAndroidJniObject::fromString(message);

//    QAndroidJniObject::callStaticMethod<void>("com/notification/javalib/QtAndroidNotification",
//                                              "show",
//                                              "(Ljava/lang/String;Ljava/lang/String;I)V",
//                                              jni_title.object<jstring>(),
//                                              jni_caption.object<jstring>(),
//                                              static_cast<jint>(id));
//    return true;
}

bool QtAndroidNotifier::show(const char *title,const char *message) {
    return show(QString(title),QString(message));
}
