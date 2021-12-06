QT       += core gui network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
PKGCONFIG += openssl
LIBS += -lz

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    #When building for Android-armv7: If neccessary change path to match where you copied the compiled libsodium to if not the same place below
    #LIBS += -L $$PWD/android/libs -lQt5Svg
    ANDROID_EXTRA_LIBS = $$PWD/libs/armeabi-v7a/libQt5Svg_armeabi-v7a.so
}
#E:\Qt\5.15.2\android\lib\

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    chatwindow.cpp \
    peerprofileswindow.cpp \
    qt-notification/QtNotification.cpp \
    qt-notification/QtNotifierFactory.cpp

HEADERS += \
    chatwindow.h \
    httpsclient.h \
    localnetworkchat.h \
    peerprofileswindow.h \
    qcompressor.h \
    qt-notification/QtAbstractNotifier.h \
    qt-notification/QtNotification.h \
    qt-notification/QtNotifierFactory.h \
    suspendable.h

FORMS += \
    chatwindow.ui \
    peerprofileswindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

android {
    QT += androidextras
    HEADERS += qt-notification/QtAndroidNotifier.h
    SOURCES += qt-notification/QtAndroidNotifier.cpp
    LIBS += -lz

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/gradle.properties \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/drawable-hdpi/icon.png \
        android/res/drawable-ldpi/icon.png \
        android/res/drawable-mdpi/icon.png \
        android/res/values/libs.xml \
        android/src/com/notification/javalib/QtAndroidNotification.java

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    ANDROID_ABIS = armeabi-v7a
}

ios {
LIBS += -framework UIKit
LIBS += -framework Foundation
LIBS += -framework UserNotifications

HEADERS += qt-notification/QtIOSNotifier.h
OBJECTIVE_SOURCES += qt-notification/QtIOSNotifier.mm
}

!android:!ios {
QT += widgets
HEADERS += qt-notification/QtDesktopNotifier.h
SOURCES += qt-notification/QtDesktopNotifier.cpp
}

RESOURCES += \
    resource.qrc
