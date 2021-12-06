#ifndef QTANDROIDNOTIFIER_H
#define QTANDROIDNOTIFIER_H

#include "QtAbstractNotifier.h"

class QtAndroidNotifier : public QtAbstractNotifier
{
public:
    QtAndroidNotifier() {}

public:
    bool show(const QVariant &notificationParameters);
    bool show(const QString caption,const QString title);
    bool show(const char *caption,const char *title);
    void clearBadgeNumberIOS() { }

private:
    int id=0;
};

#endif // QTANDROIDNOTIFIER_H
