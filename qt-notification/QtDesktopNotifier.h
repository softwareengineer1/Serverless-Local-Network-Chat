#ifndef QTDESKTOPNOTIFIER_H
#define QTDESKTOPNOTIFIER_H

#include "QtAbstractNotifier.h"

#include <QSystemTrayIcon>

class QtDesktopNotifier : public QtAbstractNotifier
{
public:
    QtDesktopNotifier();

public:
    bool show(const QVariant &notificationParameters);
    bool show(const QString caption,const QString title);
    bool show(const char *caption,const char *title);
    void clearBadgeNumberIOS() { }

private:
    QSystemTrayIcon* m_SystemTrayIcon;
};
#endif // QTDESKTOPNOTIFIER_H
