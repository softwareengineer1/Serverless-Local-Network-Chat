#ifndef QTNOTIFICATION_H
#define QTNOTIFICATION_H

#include <QObject>
#include "QtAbstractNotifier.h"

/**
 * @brief The generic notification wrapper common for all platform
 */
class QtNotification : public QObject
{
    Q_OBJECT
    public:
        explicit QtNotification(QObject *parent = nullptr);
        ~QtNotification();

    /// @see QtAbstractNotifier
    Q_INVOKABLE bool show(const QVariant &notificationParameters);
    Q_INVOKABLE bool show(const QString title,const QString message);
    Q_INVOKABLE bool show(const char *caption,const char *title);
    Q_INVOKABLE void clearBadgeNumberIOS() { }

    private:
        /// \brief The notifier object which maps to the notification methods for
        /// the respective platforms
        QtAbstractNotifier *_Notifier;
};

#endif // QTNOTIFICATION_H
