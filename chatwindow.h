#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "peerprofileswindow.h"
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class chatwindow; }
QT_END_NAMESPACE

class NotificationBroadcaster : public Thread<SuspendableWorker> {
    Q_OBJECT
public:
    QtNotification notify;
    QString title,caption;
    explicit NotificationBroadcaster(QObject *parent=nullptr) : Thread<SuspendableWorker>(new SuspendableWorker,parent) { }
    ~NotificationBroadcaster() { }
    void broadcastNotification(QString caption,QString title) {
        this->caption=caption;
        this->title=title;
        QTimer::singleShot(1,this,&NotificationBroadcaster::doNotification);
    }

    void doNotification() {
        notify.show(title,caption);
    }

    void clearBadgeNumberIOS() {
        notify.clearBadgeNumberIOS();
    }
};

class chatwindow : public QMainWindow
{
    Q_OBJECT
signals:
    void sendTextMessage(QString text);
    void printConnectedNodes();
    void printKnownNodes();
    void showProfiles();
    void clearBadgeNumberIOS();
public:
    NetworkNode *netNode=nullptr;
    peerprofileswindow *peerprofiles=nullptr;
    NotificationBroadcaster *notifier=nullptr;
    chatwindow(QWidget *parent=nullptr);
    ~chatwindow();
public slots:
    bool eventFilter(QObject *object,QEvent *event);
    void displayMessage(QString from,QString msg,QString id="");
    void displayMyOwnMessage(QString from,QString msg);
    void loadSavedMessages();
    void updateUnsentPendingOfflineMessages();
    void showMyPeerId(QString peerId);
    void setIpVisible(int yesno);
private slots:
    void on_btnClear_clicked();
    void on_btnSend_clicked();
    void on_btnProfile_clicked();
    void on_quitButton_clicked();

private:
    Ui::chatwindow *ui;
    QSettings x;
};
#endif // CHATWINDOW_H
