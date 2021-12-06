#ifndef PEERPROFILESWINDOW_H
#define PEERPROFILESWINDOW_H

#include "localnetworkchat.h"
#include <QFileDialog>

class ProfileImageDownloader : public Thread<SuspendableWorker> {
    Q_OBJECT
signals:
    void setProfileImageFromDownload(QPixmap image);
public:
    QByteArray profileImageBytes;
    QPixmap profileImage;
    QString url;
    httpsresponse response;
    explicit ProfileImageDownloader(QObject *parent=nullptr) : Thread<SuspendableWorker>(new SuspendableWorker,parent) {
        connect(this,&ProfileImageDownloader::setProfileImageFromDownload,&ProfileImageDownloader::completeSetProfileImageFromDownload);
    }
    ~ProfileImageDownloader() { }
    void startProfileImageDownload(QString imageURL) {
        url=imageURL;
        QTimer::singleShot(1,this,&ProfileImageDownloader::doProfileImageDownload);
    }

    void doProfileImageDownload() {
        httpsclient client;
        client.setUserAgent("");
        response=client.send(httpsrequest::get(url));
        if(response.success) {
            profileImageBytes=response.content;
            qDebug() << "Got response! Size:" << profileImageBytes.size();
            profileImage.loadFromData(profileImageBytes);
            emit setProfileImageFromDownload(profileImage);
        }
        else {
            qDebug() << "Failed downloading profile image at:" << url;
        }
    }

    void completeSetProfileImageFromDownload(QPixmap image) {
        profileImage=image;
        qDebug() << "Internally completed profile image download!";
    }
};

namespace Ui {
class peerprofileswindow;
}

class peerprofileswindow : public QMainWindow
{
    Q_OBJECT
signals:
    void startProfileImageDownload(QString imageURL);
    void announceUpdateProfile();
    void setIpVisible(int yesno);
    void getLocalAddresses();
    void setUdpPorts(quint16 begin,quint16 end);
    void setUdpBroadcastAddress(QString address);
public:
    ProfileImageDownloader *downloader=nullptr;
    explicit peerprofileswindow(QWidget *parent=nullptr);
    ~peerprofileswindow();
    void closeEvent(QCloseEvent *event);
    void setLocalAddresses(QHostAddress mainNodeIP,QHostAddress subnetBroadcastAddress);
    void loadMyProfile();
    void addOrUpdateMyProfile();
    void showProfiles();
    void setProfileImageFromDownload(QPixmap image);

private slots:
    void on_btnBack_clicked();
    void on_btnBrowse_clicked();
    void on_boxName_editingFinished();
    void on_boxId_editingFinished();
    void on_boxIdKey_editingFinished();
    void on_btnForceUpdate_clicked();
    void on_btnRegenerate_clicked();
    void on_btnDelete_clicked();
    void on_chkIP_stateChanged(int arg1);
    void on_btnSetFromURL_clicked();
    void on_chkClearOffline_stateChanged(int arg1);
    void on_chkAdvanced_stateChanged(int arg1);
    void showHideAdvanced();
    void on_broadcastAddr_currentIndexChanged(const QString &arg1);
    void on_udpEnd_editingFinished();

private:
    Ui::peerprofileswindow *ui;
    QSettings x;
    QTimer initialShot;
};

#endif // PEERPROFILESWINDOW_H
