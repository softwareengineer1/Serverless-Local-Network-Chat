#include "peerprofileswindow.h"
#include "ui_peerprofileswindow.h"

peerprofileswindow::peerprofileswindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::peerprofileswindow) {
    ui->setupUi(this);
    downloader=new ProfileImageDownloader(this);
    Windower::platformSpecificBackButton(ui->btnBack);

    connect(this,&peerprofileswindow::startProfileImageDownload,downloader,&ProfileImageDownloader::startProfileImageDownload);
    connect(downloader,&ProfileImageDownloader::setProfileImageFromDownload,this,&peerprofileswindow::setProfileImageFromDownload);

    x.setValue("myprofile/clearOfflinePendingMessages",x.value("myprofile/clearOfflinePendingMessages",false).toUInt());
    x.setValue("myprofile/udpRangeBegin",x.value("myprofile/udpRangeBegin","52337").toUInt());
    x.setValue("myprofile/udpRangeEnd",x.value("myprofile/udpRangeEnd","52347").toUInt());
    x.setValue("myprofile/showAdvanced",x.value("myprofile/showAdvanced",0).toInt());
    ui->chkIP->setChecked(x.value("myprofile/showIP",true).toBool());
    ui->chkClearOffline->setChecked(x.value("myprofile/clearOfflinePendingMessages").toBool());
    ui->udpBegin->setText(x.value("myprofile/udpRangeBegin").toString());
    ui->udpEnd->setText((x.value("myprofile/udpRangeEnd").toString()));
    ui->chkAdvanced->setChecked(x.value("myprofile/showAdvanced").toBool());
    initialShot.singleShot(50,this,[&] {
        showHideAdvanced();
        emit setIpVisible(ui->chkIP->isChecked());
        emit getLocalAddresses();
        emit setUdpBroadcastAddress(x.value("myprofile/udpBroadcastAddress","255.255.255.255").toString());
        loadMyProfile();
    });

}

peerprofileswindow::~peerprofileswindow() {
    delete ui;
}

void peerprofileswindow::closeEvent(QCloseEvent *event) {
    if(event) {
        event->ignore();
        hide();
    }
}

void peerprofileswindow::setLocalAddresses(QHostAddress mainNodeIP, QHostAddress subnetBroadcastAddress) {
    Q_UNUSED(mainNodeIP);
    bool dontAdd=false;
    for(int i=0; i<ui->broadcastAddr->count(); i++) {
        if(ui->broadcastAddr[i].currentText()==subnetBroadcastAddress.toString()) {
            dontAdd=true;
            break;
        }
    }
    if(!dontAdd) {
        ui->broadcastAddr->addItem(subnetBroadcastAddress.toString());
    }
    ui->broadcastAddr->setCurrentIndex(x.value("myprofile/udpBroadcastAddressIndex").toInt());
}

void peerprofileswindow::loadMyProfile() {
    x.setValue("myprofile/activeIndex",x.value("myprofile/activeIndex",0).toUInt());
    Profile &p=PeerAuthoritativeProfiles::get()->getMyProfileFromIndex(0);
    if(p.status==0) { //Profile 0 exists
        ui->boxId->setText(p.id);
        ui->boxIdKey->setText(p.idkey);
        ui->boxName->setText(p.name);
        ui->imageProfile->setPixmap(p.image);
    }
    else if(p.status==-1) { //First run of application
        QString newId=tr("%1").arg(QRandomGenerator::global()->bounded(100000000,875000000));
        QString newIdKey=tr("%1").arg(QRandomGenerator::global()->bounded(100000000,875000000));
        QString newName=tr("Anonymous-%1").arg(QRandomGenerator::global()->bounded(10000,87500));
        QPixmap newImage=ui->imageProfile->pixmap(Qt::ReturnByValue);
        ui->boxId->setText(newId);
        ui->boxIdKey->setText(newIdKey);
        ui->boxName->setText(newName);
        PeerAuthoritativeProfiles::get()->addOrUpdateMyProfile(0,newId,newIdKey,newName,newImage);
    }
}

void peerprofileswindow::showProfiles() {
    ui->profilesTree->clear();
    auto &profiles=PeerAuthoritativeProfiles::get()->profiles;
    qDebug() << "Profiles Count:" << profiles.size();
    for(auto &p : profiles) {
        QTreeWidgetItem *item=new QTreeWidgetItem(QStringList() << "" << p.name << p.id << p.idkey);
        item->setData(0,Qt::DecorationRole,p.image.scaled(64,64));
        qDebug() << "p.isOnline:" << p.isOnline;
        if(p.isOnline > 0)
            item->setData(1,Qt::DecorationRole,QPixmap(":/rsrc/greendot2").scaled(16,16));
        else
            item->setData(1,Qt::DecorationRole,QPixmap(":/rsrc/greydot1").scaled(16,16));
        ui->profilesTree->addTopLevelItem(item);
    }
}

void peerprofileswindow::setProfileImageFromDownload(QPixmap image) {
    ui->imageProfile->setPixmap(image);
    addOrUpdateMyProfile();
    qDebug() << "Downloaded profile image from url you provided!";
}

void peerprofileswindow::addOrUpdateMyProfile() {
    QString Id=ui->boxId->text();
    QString IdKey=ui->boxIdKey->text();
    QString Name=ui->boxName->text();
    QPixmap Image=ui->imageProfile->pixmap(Qt::ReturnByValue);

    PeerAuthoritativeProfiles::get()->addOrUpdateMyProfile(0,Id,IdKey,Name,Image);
    emit announceUpdateProfile();
}

void peerprofileswindow::on_btnBack_clicked() {
    hide();
}

void peerprofileswindow::on_btnBrowse_clicked() {
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    if(dialog.exec()) {
        QStringList files=dialog.selectedFiles();
        if(files.size()>0) {
            QPixmap px(files[0]);
            if(!px.isNull()) {
                ui->imageProfile->setPixmap(px);
                addOrUpdateMyProfile();
            }
        }
    }
}

void peerprofileswindow::on_boxName_editingFinished() {
    addOrUpdateMyProfile();
}

void peerprofileswindow::on_boxId_editingFinished() {
    addOrUpdateMyProfile();
}

void peerprofileswindow::on_boxIdKey_editingFinished() {
    addOrUpdateMyProfile();
}

void peerprofileswindow::on_btnForceUpdate_clicked() {
    addOrUpdateMyProfile();
}

void peerprofileswindow::on_btnRegenerate_clicked() {
    QString newId=tr("%1").arg(QRandomGenerator::global()->bounded(100000000,875000000));
    QString newIdKey=tr("%1").arg(QRandomGenerator::global()->bounded(100000000,875000000));
    ui->boxId->setText(newId);
    ui->boxIdKey->setText(newIdKey);
    addOrUpdateMyProfile();
}

void peerprofileswindow::on_btnDelete_clicked() {
    auto &profiles=PeerAuthoritativeProfiles::get()->profiles;
    auto selection=ui->profilesTree->selectedItems();
    int selectedSize=selection.size();
    for(int i=0; i<selectedSize; i++)
    {
        QTreeWidgetItem *item=selection.takeAt(i);
        int z=0;
        for(auto &p : profiles) {
            if(p.name==item->text(1) && p.id==item->text(2) && p.idkey==item->text(3)) {
                qDebug() << "Erasing profile:" << p.toString();
                profiles.erase(profiles.begin()+z);
                break;
            }
            z++;
        }
        delete item;
    }
}

void peerprofileswindow::on_chkIP_stateChanged(int arg1) {
    emit setIpVisible(arg1);
    x.setValue("myprofile/showIP",arg1);
}

void peerprofileswindow::on_btnSetFromURL_clicked() {
    QUrl url=ui->boxURL->text();
    if(url.isValid())
        emit startProfileImageDownload(url.toString());
}

void peerprofileswindow::on_chkClearOffline_stateChanged(int arg1) {
    x.setValue("myprofile/clearOfflinePendingMessages",arg1);
}

void peerprofileswindow::on_chkAdvanced_stateChanged(int arg1) {
    x.setValue("myprofile/showAdvanced",arg1);
    showHideAdvanced();
}

void peerprofileswindow::showHideAdvanced() {
    if(x.value("myprofile/showAdvanced").toBool()) { //Show
        ui->btnRegenerate->show();
        ui->labelProfileId->show();
        ui->labelProfileIdKey->show();
        ui->boxId->show();
        ui->boxIdKey->show();
        ui->labelBroadcastAddr->show();
        ui->broadcastAddr->show();
        ui->labelUdpPorts->show();
        ui->udpBegin->show();
        ui->udpEnd->show();
    }
    else { //Hide
        ui->btnRegenerate->hide();
        ui->labelProfileId->hide();
        ui->labelProfileIdKey->hide();
        ui->boxId->hide();
        ui->boxIdKey->hide();
        ui->labelBroadcastAddr->hide();
        ui->broadcastAddr->hide();
        ui->labelUdpPorts->hide();
        ui->udpBegin->hide();
        ui->udpEnd->hide();
    }
}

void peerprofileswindow::on_broadcastAddr_currentIndexChanged(const QString &arg1) {
    x.setValue("myprofile/udpBroadcastAddress",ui->broadcastAddr->currentText());
    x.setValue("myprofile/udpBroadcastAddressIndex",ui->broadcastAddr->currentIndex());
    emit setUdpBroadcastAddress(arg1);
}

void peerprofileswindow::on_udpEnd_editingFinished() {
    quint16 b=ui->udpBegin->text().toUInt();
    quint16 e=ui->udpEnd->text().toUInt();
    if(b < e) {
        x.setValue("myprofile/udpRangeBegin",b);
        x.setValue("myprofile/udpRangeEnd",e);
        qDebug() << "udpBegin:" << ui->udpBegin->text() << "udpEnd:" << ui->udpEnd->text()
                 << x.value("myprofile/udpRangeBegin") << x.value("myprofile/udpRangeEnd");
        emit setUdpPorts(x.value("myprofile/udpRangeBegin").toUInt(),x.value("myprofile/udpRangeEnd").toUInt());
    }
}

