#ifndef QTIOSNOTIFIER_H
#define QTIOSNOTIFIER_H

#include "QtAbstractNotifier.h"

class QtIosNotifier : public QtAbstractNotifier
{
public:
    QtIosNotifier();

public:
    void clearBadgeNumber();
    bool show(const QVariant &notificationParameters);
    bool show(const QString caption,const QString title);
    bool show(const char *caption,const char *title);
    void clearBadgeNumberIOS() {
        clearBadgeNumber();
    }

private:
    void                *m_Delegate;
    quint32 badgeNumber=0;
};

#endif // QTIOSNOTIFIER_H
