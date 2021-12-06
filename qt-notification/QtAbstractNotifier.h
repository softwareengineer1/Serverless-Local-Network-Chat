#ifndef QTABSTRACTNOTIFIER_H
#define QTABSTRACTNOTIFIER_H

#include <QObject>
#include <QRandomGenerator>

/**
 * @class QtAbstractNotifier The interface for generic notification
 * @brief The class which contains properties for notifications such as
 *        hide, show notifications
 *
 * Each method returns a boolean indicating whether the notifications are
 * supported or not
 */

class QtAbstractNotifier : public QObject
{
    Q_OBJECT

public:
    virtual bool show(const QVariant &notificationParameters) = 0;
    virtual bool show(const QString caption,const QString title) = 0;
    virtual bool show(const char *caption,const char *title) = 0;
    virtual void clearBadgeNumberIOS()=0;
};

#endif // QTABSTRACTNOTIFIER_H
