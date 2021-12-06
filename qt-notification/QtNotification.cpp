#include "QtNotification.h"
#include "QtNotifierFactory.h"

//------------------------------------------------------------------------------

QtNotification::QtNotification(QObject *parent)
    : QObject(parent)
    , _Notifier(nullptr)
{
    _Notifier = QtNotifierFactory::GetPlatformDependencyNotifier();
}

//------------------------------------------------------------------------------

QtNotification::~QtNotification()
{
    if (_Notifier != nullptr)
    {
        delete _Notifier;
    }
}

//------------------------------------------------------------------------------

bool QtNotification::show(const QVariant &notificationParameters)
{
    return _Notifier->show(notificationParameters);
}

bool QtNotification::show(const QString title,const QString message)
{
    return _Notifier->show(title,message);
}

bool QtNotification::show(const char *title,const char *message)
{
    return _Notifier->show(title,message);
}

//------------------------------------------------------------------------------
