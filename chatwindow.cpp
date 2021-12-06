#include "chatwindow.h"
#include "ui_chatwindow.h"

chatwindow::chatwindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::chatwindow) {
    ui->setupUi(this);
    ui->chatBox->installEventFilter(this);
    ui->chatTree->installEventFilter(this);
    netNode=new NetworkNode();
    peerprofiles=new peerprofileswindow(this);
    notifier=new NotificationBroadcaster();
    connect(netNode,&NetworkNode::sendNotification,notifier,&NotificationBroadcaster::broadcastNotification);
    connect(this,&chatwindow::clearBadgeNumberIOS,notifier,&NotificationBroadcaster::clearBadgeNumberIOS);
    Windower::platformSpecificBackButton(ui->quitButton);
    Windower::restoreWindowPositionAndSize(peerprofiles);
    Windower::restoreWindowPositionAndSize(this);

    connect(peerprofiles,&peerprofileswindow::getLocalAddresses,netNode,&NetworkNode::getLocalAddresses);
    connect(netNode,&NetworkNode::setLocalAddresses,peerprofiles,&peerprofileswindow::setLocalAddresses);
    connect(peerprofiles,&peerprofileswindow::setUdpBroadcastAddress,netNode,&NetworkNode::setUdpBroadcastAddress);
    connect(peerprofiles,&peerprofileswindow::setUdpPorts,netNode,&NetworkNode::setUdpPorts);
    connect(peerprofiles,&peerprofileswindow::announceUpdateProfile,netNode,&NetworkNode::doAnnouceUpdateProfile);
    connect(peerprofiles,&peerprofileswindow::setIpVisible,this,&chatwindow::setIpVisible);
    connect(netNode,&NetworkNode::showMyPeerId,this,&chatwindow::showMyPeerId);
    connect(netNode,&NetworkNode::showUpdatedProfiles,peerprofiles,&peerprofileswindow::showProfiles);
    connect(netNode,&NetworkNode::loadSavedMessages,this,&chatwindow::loadSavedMessages);
    connect(netNode,&NetworkNode::updateUnsentPendingOfflineMessages,this,&chatwindow::updateUnsentPendingOfflineMessages);
    connect(this,&chatwindow::showProfiles,peerprofiles,&peerprofileswindow::showProfiles);

    connect(netNode,&NetworkNode::displayReceivedMessage,this,&chatwindow::displayMessage);
    connect(netNode,&NetworkNode::displayMyOwnMessage,this,&chatwindow::displayMyOwnMessage);

    connect(this,&chatwindow::sendTextMessage,netNode,&NetworkNode::sendTextMessage);
    connect(this,&chatwindow::printConnectedNodes,netNode,&NetworkNode::printConnectedNodes);
    connect(this,&chatwindow::printKnownNodes,netNode,&NetworkNode::printDiscoveredNodes);
}

chatwindow::~chatwindow() {
    //PeerAuthoritativeProfiles::get()->deinit();
    Windower::saveWindowPositionAndSize(peerprofiles);
    Windower::saveWindowPositionAndSize(this);
    delete notifier;
    delete netNode;
    delete ui;
}

bool chatwindow::eventFilter(QObject *object,QEvent *event) {
    if((object==ui->chatBox || object==ui->chatTree) && event->type()==QEvent::KeyPress) {
        QKeyEvent *keyEvent=static_cast<QKeyEvent*>(event);
        if((keyEvent->key()==Qt::Key_Return || keyEvent->key()==Qt::Key_Enter) && !keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
            on_btnSend_clicked();
            return true;
        }
        else if(keyEvent->key()==Qt::Key_Plus) {
            QFont f=ui->chatTree->font();
            f.setPointSize(f.pointSize()-1);
            ui->chatTree->setFont(f);
            return true;
        }
        else if(keyEvent->key()==Qt::Key_Minus) {
            QFont f=ui->chatTree->font();
            f.setPointSize(f.pointSize()+1);
            ui->chatTree->setFont(f);
            return true;
        }
        else
            return QMainWindow::eventFilter(object,event);
    }
    else
        return QMainWindow::eventFilter(object,event);
}

void chatwindow::displayMessage(QString from,QString msg,QString id) {
    //QTreeWidgetItem *lastItem=ui->chatTree->topLevelItem(ui->chatTree->topLevelItemCount());
    //QTextEdit *lastEdit=qobject_cast<QTextEdit*>(ui->chatTree->itemWidget(lastItem,1));
    /*if(lastItem && lastEdit && lastItem->text(0)==from && lastEdit->toPlainText()==msg) {
        qDebug() << "Identical message received from:" << from;
    }
    else */ {
        QTreeWidgetItem *i=new QTreeWidgetItem(QStringList() << from);
        QTextEdit *t=new QTextEdit(msg);
        t->setWordWrapMode(QTextOption::WordWrap);
        t->setReadOnly(true);
        t->setMaximumHeight(64);
        t->setFrameStyle(0);
        //t->setStyleSheet("QTextEdit:!enabled { background: #ffffffff; color: black; }");

        Profile &p=PeerAuthoritativeProfiles::get()->getProfileFromId(id);
        if(p.status==0)
            i->setData(0,Qt::DecorationRole,p.image.scaled(64,64));
        else if(p.status==-1)
            i->setData(0,Qt::DecorationRole,QPixmap(":/rsrc/blankprofileimage").scaled(64,64));

        ui->chatTree->addTopLevelItem(i);
        ui->chatTree->setItemWidget(i,1,t);
        ui->chatTree->resizeColumnToContents(0);
        ui->chatTree->scrollToBottom();
    }
}

void chatwindow::displayMyOwnMessage(QString from,QString msg) {
    Profile &p=PeerAuthoritativeProfiles::get()->getMyActiveProfile();
    QTreeWidgetItem *newItem=new QTreeWidgetItem(QStringList() << from);
    QTextEdit *t=new QTextEdit(msg);
    t->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    t->setReadOnly(true);
    t->setMaximumHeight(64);
    t->setFrameStyle(0);
    if(p.status==0) {
        if(from.isEmpty()) from=p.name;
        if(from.isEmpty()) from=tr("127.0.0.1:%1").arg(x.value("net/nodePort",52337).toString());
        newItem->setText(0,from);
        newItem->setData(0,Qt::DecorationRole,p.image.scaled(64,64));
    }
    else if(p.status==-1) {
        if(from.isEmpty()) from=tr("127.0.0.1:%1").arg(x.value("net/nodePort",52337).toString());
        newItem->setText(0,from);
        newItem->setData(0,Qt::DecorationRole,QPixmap(":/rsrc/blankprofileimage").scaled(64,64));
    }
    ui->chatTree->addTopLevelItem(newItem);
    ui->chatTree->setItemWidget(newItem,1,t);
    ui->chatTree->resizeColumnToContents(0);
    ui->chatTree->scrollToBottom();
    //t->setStyleSheet("QTextEdit:!enabled { background: #ffffffff; color: black; }");
}

void chatwindow::loadSavedMessages() {
    qDebug() << "Loading saved messages:";
    ui->chatTree->clear();
    MessageStorage::get()->saved.load();
    auto &msgs=MessageStorage::get()->saved.msgs;
    auto &profiles=PeerAuthoritativeProfiles::get()->profiles;
    auto &myProfile=PeerAuthoritativeProfiles::get()->getMyActiveProfile();
    for(auto &m : msgs) {
        qDebug() << "Message:" << m.msg << m.msgtime << "from:" << m.fromid;
        for(auto &p : profiles) {
            if(m.fromid==p.id) {
                displayMessage(p.name,m.msg,p.id);
                break;
            }
            else if(m.fromid==myProfile.id) {
                displayMyOwnMessage(myProfile.name,m.msg);
                break;
            }
        }
    }
}

void chatwindow::updateUnsentPendingOfflineMessages() {
    QString newPendingCount=tr("%1").arg(MessageStorage::get()->queued.msgs.size());
    ui->labelPending->setText("Pending Offline Messages: "+newPendingCount);
}

void chatwindow::showMyPeerId(QString peerId) {
    ui->ipLabel->setText(peerId);
}

void chatwindow::setIpVisible(int yesno) {
    ui->ipLabel->setVisible(yesno);
}

void chatwindow::on_btnClear_clicked() {
    ui->chatTree->clear();
    MessageStorage::get()->saved.clearAndSave();
    if(x.value("myprofile/clearOfflinePendingMessages").toBool()) {
        MessageStorage::get()->queued.clearAndSave();
        updateUnsentPendingOfflineMessages();
    }
    emit clearBadgeNumberIOS();
}

void chatwindow::on_btnSend_clicked() {
    emit sendTextMessage(ui->chatBox->toPlainText());
    ui->chatBox->clear();
}

void chatwindow::on_btnProfile_clicked() {
    emit showProfiles();
    !peerprofiles->isVisible() ? peerprofiles->show() : peerprofiles->hide();
}

void chatwindow::on_quitButton_clicked() {
    close();
}

