#ifndef LOCALNETWORKCHAT_H
#define LOCALNETWORKCHAT_H

#include <QMainWindow>
#include <QPushButton>
#include <QtNetwork>
#include <QtWebSockets>
#include <QTreeWidget>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QRandomGenerator64>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCborMap>
#include <QCborValue>
#include <QDateTime>
#include <QMutex>
#include <QFile>
#include <QSettings>
#include <QStyleFactory>
#include <QStyle>
#include <QDebug>
#include <QCloseEvent>
#include <memory>
#include <vector>
#include "suspendable.h"
#include "httpsclient.h"
#include "qt-notification/QtNotification.h"

template<class T> using ptr=std::unique_ptr<T>;
template<class T> using sptr=std::shared_ptr<T>;
template<class T> using wptr=std::weak_ptr<T>;
template<class T> using vec=std::vector<T>;
template<class T> using p2v=ptr<vec<T>>;
template<class T> using v2p=vec<ptr<T>>;
template<class T,class ...Args> ptr<T> make(Args... args) { return std::make_unique<T>(args...); }

template<class T> class Singleton {
private:
    inline static T *instance;
public:
    Singleton() { }
    ~Singleton() { }
    Singleton(Singleton&)=delete;
    void operator=(Singleton&)=delete;
    inline static void init() {
        get();
    }
    inline static T* get() {
        if(instance==nullptr)
            instance=new T();
        return instance;
    }
    inline static void free() {
        if(instance != nullptr) {
            delete instance;
            instance=nullptr;
        }
    }
};

template<class T> class LockedSingleton {
private:
    inline static T *instance=nullptr;
    std::atomic<bool> locked=false;
    QMutex mutex;
public:
    LockedSingleton() { }
    ~LockedSingleton() { }
    //LockedSingleton(LockedSingleton&)=delete;
    //void operator=(LockedSingleton&)=delete;
    inline static void init() {
        get();
        release();
    }
    inline static T* get() {
        if(instance==nullptr)
            instance=new T();
        if(!instance->locked) {
            instance->mutex.lock();
            instance->locked=true;
        }
        return instance;
    }
    inline static T* release() {
        if(instance != nullptr && instance->locked) {
            instance->mutex.unlock();
            instance->locked=false;
            return instance;
        }
        return nullptr;
    }
    inline static void free() {
        release();
        if(instance != nullptr) {
            delete instance;
            instance=nullptr;
        }
    }
};

enum DataFormat {
    CBOR,
    JSON
};

class Windower {
public:
    static bool saveWindowPositionAndSize(QMainWindow *window=nullptr)
    {
        if(window==nullptr) return false;
        QSettings settings;
        settings.beginGroup(window->windowTitle());
        qDebug() << "Saving" << window->windowTitle() << "window position and size." << window->size() << window->pos();
        settings.setValue("size",window->size());
        settings.setValue("pos",window->pos());
        settings.setValue("visible",window->isVisible());
        settings.endGroup();
        return true;
    }

    static bool restoreWindowPositionAndSize(QMainWindow *window=nullptr)
    {
        if(window==nullptr) return false;
        QSettings settings;
        settings.beginGroup(window->windowTitle());
        if(!settings.childKeys().isEmpty()) {
            QPoint pos=settings.value("pos").toPoint();
            window->resize(settings.value("size").toSize());
            window->move(pos);
            window->setVisible(settings.value("visible").toBool());
            settings.endGroup();
            qDebug() << "Restored" << window->windowTitle() << "window position and size." << window->size() << window->pos();
            return true;
        }
        settings.endGroup();
        return false;
    }
    static void platformSpecificBackButton(QPushButton *btn) {
        #ifdef Q_OS_IOS
        btn->setVisible(true);
        #else
        #ifdef Q_OS_ANDROID
        btn->setVisible(true);
        #else
        btn->setVisible(false);
        #endif
        #endif
    }
};

class Profile {
public:
    QString name,id,idkey,userStatus,newId,newIdKey;
    QPixmap image;
    QDateTime lastOnline;
    int status,isOnline;
    Profile() { status=0; isOnline=0; }
    ~Profile() { }
    //Profile(const Profile &p) { name=p.name; id=p.id; idkey=p.idkey; image=p.image; lastOnline=p.lastOnline; status=p.status; }
    QString toString() {
        return QString("Name: %1, Id: %2, IdKey: %3, userStatus: %4, imageWidth: %5, lastOnline: %6, status: %7")
                .arg(name).arg(id).arg(idkey).arg(userStatus).arg(image.size().width()).arg(lastOnline.toString()).arg(status);
    }
};

class Profiles : public QObject {
    Q_OBJECT
signals:
    void successfullyAddedOrUpdatedCallback();
    void receivedBadKeyCallback();
public:
    vec<Profile> profiles;
    vec<Profile> myProfiles;
    Profile blankProfile,badKeyProfile;
    QSettings x;
    Profiles() : blankProfile(),badKeyProfile() {
        blankProfile.status=-1;
        badKeyProfile.status=-2;
        profilesLoadSave(0);
    }
    ~Profiles() { deinit(); }

    void deinit() {
        profilesLoadSave(1);
    }

    void profilesLoadSave(int loadsave=0) {
        if(loadsave==0) { //Load
            Profile p;
            //x.remove("profiles");
            //x.remove("myprofiles");
            int size=x.beginReadArray("profiles");
            for(int i=0; i<size; i++) {
                x.setArrayIndex(i);
                p.id=x.value("id").toString();
                p.idkey=x.value("idkey").toString();
                p.name=x.value("name").toString();
                p.image=x.value("image").value<QPixmap>();

                //qDebug() << "ReadProfile::" << p.toString();
                auto &p2=getProfileFromId(p.id);
                if(p2.status==-1)
                    profiles.push_back(p);
            }
            x.endArray();

            size=x.beginReadArray("myprofiles");
            for(int i=0; i<size; i++) {
                x.setArrayIndex(i);
                p.id=x.value("id").toString();
                p.idkey=x.value("idkey").toString();
                p.name=x.value("name").toString();
                p.image=x.value("image").value<QPixmap>();

                qDebug() << "ReadMyProfile::" << p.toString();
                auto &p2=getProfileFromId(p.id);
                if(p2.status==-1)
                    myProfiles.push_back(p);
            }
            x.endArray();
        }
        else { //Save
            x.beginWriteArray("profiles");
            for (size_t i=0; i<profiles.size(); i++) {
                x.setArrayIndex((int)i);
                x.setValue("id",profiles[i].id);
                x.setValue("idkey",profiles[i].idkey);
                x.setValue("name",profiles[i].name);
                x.setValue("image",profiles[i].image);

                //qDebug() << "WroteProfile::" << profiles[i].toString();
            }
            x.endArray();

            x.beginWriteArray("myprofiles");
            for (size_t i=0; i<myProfiles.size(); i++) {
                x.setArrayIndex((int)i);
                x.setValue("id",myProfiles[i].id);
                x.setValue("idkey",myProfiles[i].idkey);
                x.setValue("name",myProfiles[i].name);
                x.setValue("image",myProfiles[i].image);
                qDebug() << "WroteMyProfile::" << myProfiles[i].toString();
            }
            x.endArray();
        }
    }

    Profile &getMyActiveProfile() {
        return getMyProfileFromIndex(x.value("myprofile/activeIndex").toUInt());
    }

    Profile &getMyProfileFromIndex(size_t index) {
        size_t i=0;
        for(auto &p : myProfiles) {
            if(i==index) return p;
            i++;
        }
        return blankProfile;
    }

    Profile &getProfileFromId(QString id) {
        for(auto &p : profiles) {
            if(p.id==id)
                return p;
        }
        return blankProfile;
    }

    Profile &getProfileFromIdAndKey(QString id,QString idkey) {
        Profile &p=getProfileFromId(id);
        if(p.status >= 0) {
            if(p.idkey==idkey)
                return p;
            else
                return badKeyProfile;
        }
        return blankProfile;
    }

    Profile &addOrUpdateProfile(QString id,QString idkey,QString name,QPixmap image=QPixmap(":/rsrc/blankprofileimage")) {
        Profile p1,*p=&p1;
        bool update=false;
        for(auto &p2 : profiles) {
            if(p2.id==id) {
                p=&p2;
                update=true;
                break;
            }
        }
        p->id=id;
        p->idkey=idkey;
        p->name=name;
        p->image=image;
        if(!update)
            profiles.push_back(*p);
        //emit successfullyAddedOrUpdatedCallback();
        profilesLoadSave(1);
        return *p;
    }

    void addOrUpdateMyProfile(size_t index,QString id,QString idkey,QString name,QPixmap image=QPixmap(":/rsrc/blankprofileimage")) {
        Profile p1,*p=&p1;
        bool update=false;
        size_t i=0;
        for(auto &p2 : myProfiles) {
            if(i==index) {
                p=&p2;
                if(p->id != id || p->idkey != idkey) {
                    p->newId=id;
                    p->newIdKey=idkey;
                }
                update=true;
                break;
            }
            i++;
        }
        p->name=name;
        p->image=image;
        if(!update) {
            p->id=id;
            p->idkey=idkey;
            myProfiles.push_back(*p);
        }
        //emit successfullyAddedOrUpdatedCallback();
        profilesLoadSave(1);
    }
};

class PeerAuthoritativeProfiles : public Singleton<Profiles> { };

class NodeAddr {
public:
    QWebSocket *ws=nullptr;
    QHostAddress addr=QHostAddress::AnyIPv4;
    Profile profile;
    quint16 port=53337,sport=0,acked=0,serverConnectedNode=0;
    qint32 status=0;
    QDateTime lastConnectedTime;
    NodeAddr(QHostAddress address,quint16 theport) : addr(address),port(theport) { }
    NodeAddr(const NodeAddr &a) { ws=a.ws; addr=a.addr; profile=a.profile; port=a.port; lastConnectedTime=a.lastConnectedTime; }
    NodeAddr() { }
    ~NodeAddr() { }
};

class Message {
public:
    vec<QString> usersThatHaventReceivedMessageYet;
    QString msg,fromid;
    QDateTime msgtime;
    QByteArray msghash;
    int status=0;
    void hash() {
        msghash=QCryptographicHash::hash(msg.toUtf8(),QCryptographicHash::Sha256);
    }
    bool operator==(Message &message) {
        return message.msgtime==msgtime && message.msg==msg && message.fromid==fromid;
    }
    int indexOfUserInList(QString user) {
        if(user.isEmpty()) return -1;
        int i=0;
        for(auto &u : usersThatHaventReceivedMessageYet) {
            if(u==user) return i;
            i++;
        }
        return -1;
    }
    void addUserToList(QString user) {
        if(user.isEmpty()) return;
        for(auto &u : usersThatHaventReceivedMessageYet) {
            if(u==user) return;
        }
        usersThatHaventReceivedMessageYet.push_back(user);
    }
    void removeUserFromList(QString user) {
        int i=indexOfUserInList(user);
        if(i != -1) {
            usersThatHaventReceivedMessageYet.erase(usersThatHaventReceivedMessageYet.begin()+i);
            if(usersThatHaventReceivedMessageYet.empty())
                status=1; //Message completely delivered and can be erased...
        }
    }
};

class MessageQueue {
public:
    vec<Message> msgs;
    Message blankMessage;
    QString category;
    QSettings x;

    MessageQueue() : blankMessage() { blankMessage.status=-1; }
    MessageQueue(QString messagesCategory) : category(messagesCategory) { blankMessage.status=-1; }

    Message &getMessageFromMessage(Message &msg) {
        for(auto &m : msgs) {
            if(m==msg)
                return m;
        }
        return blankMessage;
    }

    void clear() {
        msgs.clear();
        x.remove(category);
        qDebug() << category << "messages cleared...";
    }

    void clearAndSave() {
        clear();
        save();
    }

    void addMessage(Message &msg) {
        if(msg.msg.isEmpty()) return;
        auto &m=getMessageFromMessage(msg);
        if(m.status==-1)
            msgs.push_back(msg);
    }

    void addMessage(QString msg,QString fromid) {
        if(msg.isEmpty()) return;
        Message m;
        m.msg=msg;
        m.fromid=fromid;
        m.msgtime=QDateTime::currentDateTime();
        auto &m2=getMessageFromMessage(m);
        if(m2.status==-1) {
            msgs.push_back(m);
            //qDebug() << "Adding message:" << msg << "from:" << fromid << "at time:" << m.msgtime;
        }
    }
    void load() {
        if(category.isEmpty()) return;
        int size=x.beginReadArray(category);
        Message m;
        for(int i=0; i<size; i++) {
            x.setArrayIndex(i);
            int size2=x.beginReadArray("usersThatHaventReceivedYet");
            for(int i2=0; i2<size2; i2++) {
                x.setArrayIndex(i2);
                m.addUserToList(x.value("id").toString());
                //qDebug() << "loaded users that havent received yet:" << m.usersThatHaventReceivedMessageYet[i2];
            }
            for(auto &u : m.usersThatHaventReceivedMessageYet)
            {
                qDebug() << "Queued message has users waiting for message:" << u;
            }
            x.endArray();
            m.msg=x.value("message").toString();
            m.fromid=x.value("fromid").toString();
            QString time=x.value("queuedWhen").toString();
            m.msgtime=QDateTime::fromString(time);
            qDebug() << category << "Loaded message:" << m.fromid << m.msg << m.msgtime << time;
            if(getMessageFromMessage(m).status==-1)
                msgs.push_back(m);
        }
        x.endArray();
    }

    void save() {
        if(category.isEmpty()) return;
        x.remove(category);
        x.beginWriteArray(category);
        for (size_t i=0; i<msgs.size(); i++) {
            x.setArrayIndex((int)i);
            x.beginWriteArray("usersThatHaventReceivedYet");
            for(size_t i2=0; i2<msgs[i].usersThatHaventReceivedMessageYet.size(); i2++) {
                x.setArrayIndex((int)i2);
                x.setValue("id",msgs[i].usersThatHaventReceivedMessageYet[i2]);
            }
            x.endArray();
            x.setValue("message",msgs[i].msg);
            x.setValue("fromid",msgs[i].fromid);
            x.setValue("queuedWhen",msgs[i].msgtime.toString());
            qDebug() << category << "Saved message:" << msgs[i].fromid << msgs[i].msg << msgs[i].msgtime;
        }
        x.endArray();
    }
};

class MessageStorage : public Singleton<MessageStorage> {
public:
    MessageQueue queued=MessageQueue("queuedmessages");
    MessageQueue saved=MessageQueue("savedmessages");
};

class NetworkNode : public Thread<SuspendableWorker> {
    Q_OBJECT
signals:
    void setLocalAddresses(QHostAddress mainNodeIP,QHostAddress subnetBroadcastAddress);
    void discoverOtherNodes();
    void ensureConnectionWithNode(QHostAddress addr,quint16 port);
    void ensureConnectionWithNodeSocket(QWebSocket *sck);
    void showMyPeerId(QString peerId);
    void sendNotification(const QString title,const QString message);
    void showUpdatedProfiles();
    void loadSavedMessages();
    void updateUnsentPendingOfflineMessages();
    void displayReceivedMessage(QString from,QString msg,QString id="");
    void displayMyOwnMessage(QString from,QString msg,QString id="");
public:
    QWebSocketServer *ws=nullptr;
    QtNotification *notifier=nullptr;
    NodeAddr nodeAddress,broadcastAddress,blankNode;
    vec<NodeAddr> discoveredOtherNodes,connectedOtherNodes;
    QList<QUdpSocket*> udpBroadcasters;
    QList<QHostAddress> localAddresses;
    QHostAddress firstLocalAddress,subnetBroadcastAddress;
    QTimer initialShot,timedDiscovery,timedEnsureConnectivity;
    QRandomGenerator r;
    QSettings x;
    quint32 retriesForAquiringListeningPort=10,singleShotDelay=150,discoveryInterval=1000,connectivityInterval=10000;
    quint16 udpPortsBegin=52337,udpPortsEnd=52347;

    explicit NetworkNode(QObject *parent=nullptr) : Thread<SuspendableWorker>(new SuspendableWorker,parent),notifier(new QtNotification(this)) {
        connect(this,&NetworkNode::discoverOtherNodes,&NetworkNode::onDiscoverOtherNodes);
        init();
    }
    ~NetworkNode() {
        deinit();
    }

    bool init() {
        r=r.securelySeeded();
        blankNode.status=-1;
        broadcastAddress.addr=QHostAddress::Broadcast;
        //Read configuration
        configReadWrite(0);

        getLocalAddresses();
        //Set Udp Broadcast Ports (For listening for udp broadcasts for auto node/peer discovery)
        setUdpPorts(udpPortsBegin,udpPortsEnd);

        //Start node's server using web sockets protocol...
        if(startNodeServer()) {
            //Do initial node discovery via udp broadcast.
            emit discoverOtherNodes();
            //Keep discovering and ensure connections are established with all discovered and then known nodes.
            // { Ensure either they are connected to you or you are connected to them. }
            connect(&timedDiscovery,&QTimer::timeout,this,&NetworkNode::onDiscoverOtherNodes);
            connect(&timedEnsureConnectivity,&QTimer::timeout,this,&NetworkNode::ensureOptimalConnectivity);
            timedDiscovery.start(discoveryInterval);
            timedEnsureConnectivity.start(connectivityInterval);
            initialShot.singleShot(singleShotDelay,this,&NetworkNode::onDiscoverOtherNodes);
            initialShot.singleShot(singleShotDelay,this,&NetworkNode::showMyPeerIdFunc);
            initialShot.singleShot(singleShotDelay,this,&NetworkNode::loadSavedMessages);
            return true;
        }
        return false;
    }

    bool deinit() {
        //Write configuration
        configReadWrite(1);
        //And stop everything...
       return stopNodeServer();
    }

    void configReadWrite(int readwrite) {
        if(readwrite==0) { //read
            nodeAddress.port=x.value("net/nodePort",nodeAddress.port).toUInt();
            retriesForAquiringListeningPort=x.value("net/usablePorts",retriesForAquiringListeningPort).toUInt();
            udpPortsBegin=x.value("myprofile/udpRangeBegin",52337).toUInt();
            udpPortsEnd=x.value("myprofile/udpRangeEnd",52347).toUInt();

            qDebug() << "read udp ports:" << udpPortsBegin << udpPortsEnd;
        }
        else { //write
            x.setValue("net/nodePort",nodeAddress.port);
            x.setValue("net/usablePorts",retriesForAquiringListeningPort);
            x.setValue("myprofile/udpRangeBegin",udpPortsBegin);
            x.setValue("myprofile/udpRangeEnd",udpPortsEnd);
        }
    }

    void getLocalAddresses() {
        localAddresses=QNetworkInterface::allAddresses();
        for(auto &l : localAddresses) {
            if(!l.toString().startsWith("169.254") && l.toString() != "127.0.0.1" && !l.isLoopback() && l.toIPv4Address()>0) {
                auto subnetBroadcastAddr=l.toString().split(".");
                subnetBroadcastAddr.last()="255";
                firstLocalAddress=l;
                subnetBroadcastAddress=QHostAddress(subnetBroadcastAddr.join("."));
                emit setLocalAddresses(firstLocalAddress,subnetBroadcastAddress);
                break;
            }
        }
    }

    bool startNodeServer() {
        if(ws==nullptr)
            ws=new QWebSocketServer("LocalNetworkChat-v1.0",QWebSocketServer::NonSecureMode,this);
        for(quint32 z=0; z<retriesForAquiringListeningPort; z++) {
            if(ws->listen(QHostAddress::AnyIPv4,0)) {
                //Setup signals and slots for websocket accepting and ensuring connectivity for local network chat!
                connect(ws,&QWebSocketServer::newConnection,this,&NetworkNode::onNodeIncomingConnection);
                connect(this,&NetworkNode::ensureConnectionWithNode,this,&NetworkNode::onEnsureConnectionWithNode);
                connect(this,&NetworkNode::ensureConnectionWithNodeSocket,this,&NetworkNode::onEnsureConnectionWithNodeSocket);
                nodeAddress.port=ws->serverPort();
                ws->setHandshakeTimeout(10);
                MessageStorage::get()->queued.load(); //Load queued messages for sending to likely now online users they've been queued up for! :)
                qDebug() << peerId() << "Local Network Chat Node listening on port:" << nodeAddress.port;
                return true;
            }
        }
        return false;
    }

    bool stopNodeServer() {
        if(ws != nullptr && ws->isListening()) {
            for(auto &n : connectedOtherNodes)
                if(n.ws != nullptr)
                    n.ws->deleteLater();
            connectedOtherNodes.clear();
            discoveredOtherNodes.clear();
            disconnect(ws,&QWebSocketServer::newConnection,this,&NetworkNode::onNodeIncomingConnection);
            disconnect(this,&NetworkNode::ensureConnectionWithNode,this,&NetworkNode::onEnsureConnectionWithNode);
            ws->deleteLater();
            MessageStorage::get()->queued.save(); //Save queued messages for offline users as even you go offline yourself...
            MessageStorage::get()->saved.save(); //Save chat messages until user clears them...
            qDebug() << peerId() << "Local Network Chat Node stopped listening on port" << nodeAddress.port;
            return true;
        }
        return false;
    }

    void showMyPeerIdFunc() {
        emit showMyPeerId(peerId());
    }

    QString peerId() {
        return tr("[%1:%2]").arg(firstLocalAddress.toString()).arg(nodeAddress.port);
    }

    QString peerWebSocketUrl(NodeAddr &nodeAddr) {
        return tr("ws://%1:%2").arg(nodeAddr.addr.toString()).arg(nodeAddr.port);
    }

    void setUdpBroadcastAddress(QString address) {
        qDebug() << "Setting broadcast address to:" << address;
        broadcastAddress.addr=QHostAddress(address);
    }

    void setUdpPorts(quint16 begin,quint16 end) {
        qDebug() << "Setting udp ports";
        udpPortsBegin=begin;
        udpPortsEnd=end;
        for(auto &udp : udpBroadcasters) {
            if(udp != nullptr) udp->deleteLater();
        }
        udpBroadcasters.clear();
        if(begin>0) {
            qDebug() << "Setting" << begin << "to" << end << "udp ports";
            for(int port=begin; port<(end+1); port++) {
                udpBroadcasters.push_back(new QUdpSocket(this));
                udpBroadcasters.back()->bind(QHostAddress::AnyIPv4,port,QUdpSocket::ShareAddress);
                connect(udpBroadcasters.back(),&QUdpSocket::readyRead,this,&NetworkNode::onReceiveUdpBroadcast);
            }
        }
    }

    void onDiscoverOtherNodes() {
        //qDebug() << peerId() << "Sending peer discovery udp broadcast! to:" << broadcastAddress.addr << udpPortsBegin << udpPortsEnd;
        QByteArray broadcastingMyWebSocketNodePort=tr("%1").arg(nodeAddress.port).toUtf8();
        for(int port=udpPortsBegin; port<udpPortsEnd; port++)
            udpBroadcasters[0]->writeDatagram(broadcastingMyWebSocketNodePort,broadcastAddress.addr,port);
    }

    void onReceiveUdpBroadcast() {
        QHostAddress sender;
        quint16 senderPort;
        QByteArray data;
        //qDebug() << peerId() << "Received UDP Broadcast";
        for(auto &udp : udpBroadcasters) {
            while(udp->hasPendingDatagrams()) {
                data.resize((int)udp->pendingDatagramSize());
                udp->readDatagram(data.data(),data.size(),&sender,&senderPort);
                quint16 webSocketPort=data.toUInt();
                //qDebug() << "Raw datagram:" << data << "from:" << sender << senderPort;
                //Do not add our own local address as another node.
                bool dontAdd=false;
                for(auto &a : localAddresses) {
                    //qDebug() << "Checking peer discovery for local addresses:" << peerId() << sender.toString() << senderPort << webSocketPort;
                    if((a.toIPv4Address()==sender.toIPv4Address() && webSocketPort==nodeAddress.port)
                            || sender.toString().startsWith("169.254")) {
                        //qDebug() << "Is local peer:" << peerId() << sender.toString() << senderPort << webSocketPort;
                        dontAdd=true;
                        break;
                    }
                }
                //Do not add nodes we already know about
                for(auto &n : discoveredOtherNodes) {
                    if(n.addr==sender && n.port==webSocketPort) {
                        dontAdd=true;
                        break;
                    }
                }

                if(!dontAdd) {
                    qDebug () << peerId() << tr("{Peer Discovery} Other Node Announced Themselves @ %1:%2").arg(sender.toString()).arg(webSocketPort);
                    discoveredOtherNodes.push_back(NodeAddr(sender,webSocketPort));
                    emit ensureConnectionWithNode(sender,webSocketPort);
                    onDiscoverOtherNodes();
                    printDiscoveredNodes();
                }
            }
        }
    }

    int isNodeInList(NodeAddr &node,vec<NodeAddr> &nodeList) {
        int i=0;
        if(nodeList.empty()) return -1;
        for(auto &n : nodeList) {
            if(n.addr==node.addr && n.port==node.port) {
                //qDebug() << peerId() << "nodeIsInlist:" << n.addr << n.port;
                return i;
            }
            i++;
        }
        return -1;
    }

    int isSocketInNodeList(QWebSocket *sck,vec<NodeAddr> &nodeList) {
        int i=0;
        if(nodeList.empty()) return -1;
        for(auto &n : nodeList) {
            if(n.ws==sck) {
                //qDebug() << peerId() << "socketIsInNodeList:" << n.addr << n.port;
                return i;
            }
            i++;
        }
        return -1;
    }

    NodeAddr& getNodeFromAddrAndPort(QHostAddress addr,quint16 port,vec<NodeAddr> &nodeList) {
        NodeAddr node(addr,port);
        return getNodeFromNode(node,nodeList);
    }

    NodeAddr& getNodeFromNode(NodeAddr &node,vec<NodeAddr> &nodeList) {
        int i=isNodeInList(node,nodeList);
        if(i != -1)
            return nodeList[i];
        return blankNode;
    }

    NodeAddr& getNodeFromSocket(QWebSocket *sck,vec<NodeAddr> &nodeList) {
        int i=isSocketInNodeList(sck,nodeList);
        if(i != -1)
            return nodeList[i];
        return blankNode;
    }

    void ensureOptimalConnectivity() {
        int x=0;
        for(auto &n : discoveredOtherNodes) {
            auto &node=getNodeFromNode(n,connectedOtherNodes);
            if(node.status==-1) {
                if(QDateTime::currentDateTime() > n.lastConnectedTime.addSecs(60*3)) {
                    //Its past the old node prune time (default 3 minutes since last connected)
                    //So erase it and stop trying to connect to this long gone node...
                    qDebug() << peerId() << "Pruning old node:" << peerWebSocketUrl(n) << "Last connected time:" << n.lastConnectedTime;
                    discoveredOtherNodes.erase(discoveredOtherNodes.begin()+x);
                }
                else {
                    qDebug() << peerId() << "{Is continuing to ensure connectivity with:}" << peerWebSocketUrl(n);
                    emit ensureConnectionWithNode(n.addr,n.port);
                }
            }
            x++;
        }
    }

    void onEnsureConnectionWithNode(QHostAddress addr,quint16 port) {
        NodeAddr n(addr,port);
        auto &node=getNodeFromNode(n,connectedOtherNodes);
        if(node.status==-1) {
            //If node is not currently connected
            QWebSocket *sck=new QWebSocket();
            sck->setObjectName(tr("isOutgoingNodeSocket %1:%2").arg(addr.toString()).arg(port));
            connect(sck,&QWebSocket::connected,this,&NetworkNode::onNodeWebSocketConnected);
            connect(sck,&QWebSocket::disconnected,this,&NetworkNode::onNodeWebSocketDisconnected);
            sck->open(QUrl(peerWebSocketUrl(n)));
            int i=isNodeInList(n,discoveredOtherNodes);
            if(i != -1)
                n.lastConnectedTime=QDateTime::currentDateTime();
            qDebug() << peerId() << "{Ensuring Connection With Node:} " << peerWebSocketUrl(n);
        }
    }

    void onEnsureConnectionWithNodeSocket(QWebSocket *sck) {
        auto &node=getNodeFromSocket(sck,connectedOtherNodes);
        if(node.status==-1) {
            sck->setObjectName(tr("isIncomingNodeSocket %1:%2").arg(sck->peerAddress().toString()).arg(sck->peerPort()));
            connect(sck,&QWebSocket::connected,this,&NetworkNode::onNodeWebSocketConnected);
            connect(sck,&QWebSocket::disconnected,this,&NetworkNode::onNodeWebSocketDisconnected);
            NodeAddr n(sck->peerAddress(),sck->peerPort());
            n.ws=sck;
            n.lastConnectedTime=QDateTime::currentDateTime();
            connectedOtherNodes.push_back(n);
            qDebug() << peerId() << "{Ensuring Connection With Node Socket:} " << peerWebSocketUrl(n);
            printConnectedNodes();
        }
    }

    void onNodeWebSocketConnected() {
        QWebSocket *connectedNode=qobject_cast<QWebSocket*>(sender());
        NodeAddr n(connectedNode->peerAddress(),connectedNode->peerPort());
        connect(connectedNode,&QWebSocket::textMessageReceived,this,&NetworkNode::onNodeTextMessageReceived);
        connect(connectedNode,&QWebSocket::binaryMessageReceived,this,&NetworkNode::onNodeDataMessageReceived);
        auto &node=getNodeFromSocket(connectedNode,connectedOtherNodes);
        if(node.status==-1) {
            n.ws=connectedNode;
            connectedOtherNodes.push_back(n);
            sendUpdateProfileMessage(n.ws);
            qDebug() << peerId() << tr("{{Connected To Node:}} %1").arg(peerWebSocketUrl(n)) << "at:" << n.lastConnectedTime;
            printConnectedNodes();
        }
    }

    void onNodeWebSocketDisconnected() {
        QWebSocket *disconnectedNode=qobject_cast<QWebSocket*>(sender());
        disconnect(disconnectedNode,&QWebSocket::textMessageReceived,this,&NetworkNode::onNodeTextMessageReceived);
        disconnect(disconnectedNode,&QWebSocket::binaryMessageReceived,this,&NetworkNode::onNodeDataMessageReceived);
        if(disconnectedNode->peerAddress().isNull()) return;

        NodeAddr n(disconnectedNode->peerAddress(),disconnectedNode->peerPort());
        qDebug() << peerId() << tr("{Node Disconnected From Me} %1").arg(peerWebSocketUrl(n));
        int i=isNodeInList(n,discoveredOtherNodes);
        if(i != -1)
            discoveredOtherNodes[i].lastConnectedTime=QDateTime::currentDateTime();
        i=isSocketInNodeList(disconnectedNode,connectedOtherNodes);
        if(i != -1) {
            auto &p=PeerAuthoritativeProfiles::get()->getProfileFromId(connectedOtherNodes[i].profile.id);
            if(p.status==0) {
                bool isOnlineStill=false;
                for(auto &node : connectedOtherNodes) {
                    if(node.profile.id==connectedOtherNodes[i].profile.id && node.ws != disconnectedNode) {
                        isOnlineStill=true;
                        break;
                    }
                }
                if(!isOnlineStill)
                    p.isOnline=0;
                emit showUpdatedProfiles();
            }

            qDebug() << "Erasing: " << peerWebSocketUrl(connectedOtherNodes[i]) << "at index:" << i;
            connectedOtherNodes.erase(connectedOtherNodes.begin()+i);
            disconnectedNode->deleteLater();
        }
        printConnectedNodes();
    }

    void onNodeTextMessageReceived(QString text) {
        QWebSocket *sck=qobject_cast<QWebSocket*>(sender());
        NodeAddr &n=getNodeFromSocket(sck,connectedOtherNodes);
        if(n.profile.isOnline) {
            QString displayName=n.profile.name;
            if(displayName.isEmpty())
                displayName=tr("%1:%2").arg(n.addr.toString()).arg(n.port);

            MessageStorage::get()->saved.addMessage(text,n.profile.id);
            emit displayReceivedMessage(displayName,text,n.profile.id);
            //QtNotification notify;
            //notify.show(displayName,text);
            emit sendNotification(displayName,text);
            sendAnyQueuedMessages();
            //qDebug() << peerId() << tr("{Message Received From: [%1] %2:}").arg(peerWebSocketUrl(n)).arg(displayName) << text;
        }
    }

    void sendMessageToOnlineUsers(QString &msg) {
        auto &profiles=PeerAuthoritativeProfiles().get()->profiles;
        for(auto &p : profiles) {
            for(auto &n : connectedOtherNodes) {
                if(p.id==n.profile.id) {
                    if(n.ws==nullptr) continue;
                    //qDebug() << peerId() << "{Sending Message To:}" << p.name << "@" << peerWebSocketUrl(n) << "With profile id:" << p.id;
                    n.ws->sendTextMessage(msg);
                    break;
                }
            }
        }
    }

    void queueMessageForOfflineUsers(QString &msg) {
        auto &profiles=PeerAuthoritativeProfiles().get()->profiles;

        vec<Profile*> offlineProfiles;
        for(auto &p : profiles) {
            if(!p.isOnline)
                offlineProfiles.push_back(&p);
        }
        auto &myProfile=PeerAuthoritativeProfiles::get()->getMyActiveProfile();
        if(!offlineProfiles.empty()) {
            Message m;
            m.msg=msg;
            m.fromid=myProfile.id;
            m.msgtime=QDateTime::currentDateTime();
            for(auto &p : offlineProfiles)
                m.addUserToList(p->id);
            MessageStorage::get()->queued.msgs.push_back(m);
        }
        MessageStorage::get()->saved.addMessage(msg,myProfile.id);
        emit displayMyOwnMessage(myProfile.name,msg);
    }

    void sendTextMessage(QString msg) {
        //qDebug() << peerId() << "{Trying message send:}" << connectedOtherNodes.size();
        //Added message queue:
        //Go through currently connected profiles and send the message to whos currently online
        sendMessageToOnlineUsers(msg);
        //For any remaining offline profiles the message will be stored in the queue for next time they come back online
        //If they don't come back online or their profile is deleted the queued message will be deleted after so much time...
        //Queue is saved and reloaded for seamless offline messaging...
        queueMessageForOfflineUsers(msg);
        emit updateUnsentPendingOfflineMessages();
    }

    void sendAnyQueuedMessages() {
        auto &msgs=MessageStorage::get()->queued.msgs;
        for(auto &m : msgs) {
            qDebug() << "starting with messages:" << m.msg << m.fromid << m.usersThatHaventReceivedMessageYet;
        }

        //Removing completely delivered messages
        msgs.erase(std::remove_if(msgs.begin(),msgs.end(),
                                  [](Message m) {
                       return m.status==1;
                   }),msgs.end());

        int messageIndex=0;
        for(auto &m : msgs) {
            if(m.status==1) {
                qDebug() << "[1] message somehow still left that shouldnt be:" << m.msg << m.fromid << m.usersThatHaventReceivedMessageYet << m.status;
                msgs.erase(msgs.begin()+messageIndex);
            }
            messageIndex++;
        }

        //For all connected online nodes
        for(auto &n : connectedOtherNodes) {
            //Send any messages queued for this online user
            for(auto &m : msgs) {
                int i=m.indexOfUserInList(n.profile.id);
                if(i != -1 && n.ws != nullptr) {
                    n.ws->sendTextMessage(m.msg);
                    m.removeUserFromList(n.profile.id);
                    qDebug() << "Queued message:" << m.msg << "sent to:" << n.profile.id << "user index:" << i << "message index:" << messageIndex;
                }
                messageIndex++;
            }
        }

        //Removing completely delivered messages
        msgs.erase(std::remove_if(msgs.begin(),msgs.end(),
                                  [](Message m) {
                       return m.status==1;
                   }),msgs.end());

        messageIndex=0;
        for(auto &m : msgs) {
            if(m.status==1) {
                qDebug() << "[2] message somehow still left that shouldnt be:" << m.msg << m.fromid << m.usersThatHaventReceivedMessageYet << m.status;
                msgs.erase(msgs.begin()+messageIndex);
            }
            messageIndex++;
        }

        MessageStorage::get()->queued.save();
        emit updateUnsentPendingOfflineMessages();
    }

    void doAnnouceUpdateProfile() {
        auto &profiles=PeerAuthoritativeProfiles().get()->profiles;
        for(auto &p : profiles) {
            for(auto &n : connectedOtherNodes) {
                if(p.id==n.profile.id) {
                    if(n.ws==nullptr) continue;
                    sendUpdateProfileMessage(n.ws);
                }
            }
        }
    }

    void sendUpdateProfileMessage(QWebSocket *sck) {
        if(sck==nullptr) return;
        QByteArray bytes;
        quint8 messageClass=0;
        quint8 messageType=0;
        messageClassAndTypePack(bytes,messageClass,messageType);

        Profile &p=PeerAuthoritativeProfiles::get()->getMyActiveProfile();
        if(!p.id.isEmpty() && !p.idkey.isEmpty()) {
            QByteArray pxBytes;
            QBuffer pxBuffer(&pxBytes);
            pxBuffer.open(QIODevice::WriteOnly);
            p.image.save(&pxBuffer,"PNG");

            QString concatinatedAuthorizationInfo=tr("%1::::%2::::%3::::%4::::%5::::%6")
                    .arg(p.id).arg(p.idkey).arg(p.name).arg(p.newId).arg(p.newIdKey)
                    .arg(QString::fromUtf8(pxBytes.toBase64(QByteArray::Base64UrlEncoding)));

            bytes+=concatinatedAuthorizationInfo.toUtf8();
            sck->sendBinaryMessage(bytes.toBase64(QByteArray::Base64UrlEncoding));

            p.id=p.newId.isEmpty() ? p.id : p.newId;
            p.idkey=p.newIdKey.isEmpty() ? p.idkey : p.newIdKey;
        }
    }

    void sendInvalidIdKeyResponse(QWebSocket *sck) {
        QByteArray bytes;
        quint8 messageClass=1;
        quint8 messageType=0;
        messageClassAndTypePack(bytes,messageClass,messageType);
        sck->sendBinaryMessage(bytes.toBase64(QByteArray::Base64UrlEncoding));
    }

    void messageClassAndTypePack(QByteArray &data,quint8 messageClass,quint8 messageType) {
            data+=messageClass;
            data+=messageType;
    }
    void messageClassAndTypeUnpack(QByteArray &data,quint8 &messageClass,quint8 &messageType) {
        if(data.size() >= 2) {
            messageClass=data[0];
            messageType=data[1];
            data.remove(0,2);
        }
    }

    void onNodeIncomingConnection() {
        QWebSocket *incomingNode=ws->nextPendingConnection();
        connect(incomingNode,&QWebSocket::textMessageReceived,this,&NetworkNode::onNodeTextMessageReceived);
        connect(incomingNode,&QWebSocket::binaryMessageReceived,this,&NetworkNode::onNodeDataMessageReceived);

        sendUpdateProfileMessage(incomingNode);
        emit ensureConnectionWithNodeSocket(incomingNode);
    }

    void onNodeDataMessageReceived(QByteArray data) {
        QWebSocket *sck=qobject_cast<QWebSocket*>(sender());
        data=QByteArray::fromBase64(data,QByteArray::Base64UrlEncoding);
        quint8 messageClass;
        quint8 messageType;
        messageClassAndTypeUnpack(data,messageClass,messageType);
        if(messageClass==0) { //Data Message
            if(messageType==0) { // Set/Update Profile Message
                //qDebug() << "Received update profile message!";
                QStringList splat=QString::fromUtf8(data).split("::::");
                if(!splat.isEmpty() && splat.size() >= 6) {
                    QString id=splat[0];
                    QString idkey=splat[1];
                    QString name=splat[2];
                    QString newId=splat[3];
                    QString newIdKey=splat[4];
                    QByteArray profileImageUnbased=QByteArray::fromBase64(splat[5].toUtf8(),QByteArray::Base64UrlEncoding);
                    QPixmap profileImage;
                    profileImage.loadFromData(profileImageUnbased);

                    Profile &p=PeerAuthoritativeProfiles::get()->getProfileFromIdAndKey(id,idkey);
                    if(p.status==0) { //Successfully authed as user
                        //Load their profile;
                        qDebug() << "Successfully authed user:" << p.name << p.id << "idkey:" << p.idkey << sck;
                        if(p.name != name) {
                            qDebug() << "User also changing name simultaneously to:" << name;
                            p.name=name;
                        }
                        p.id=newId.isEmpty() ? p.id : newId;
                        p.idkey=newIdKey.isEmpty() ? p.idkey : newIdKey;

                        if(p.image.toImage() != profileImage.toImage()) {
                            qDebug() << "User also changing profile image to:" << profileImage;
                            p.image=profileImage;
                        }
                        p.isOnline=1;
                        auto &node=getNodeFromSocket(sck,connectedOtherNodes);
                        if(node.status != -1)
                            node.profile=p;
                        sendAnyQueuedMessages();
                        emit showUpdatedProfiles();
                    }
                    else if(p.status==-1) { //Not yet existant user
                        if(!id.isEmpty() && !idkey.isEmpty() && !name.isEmpty()) {
                            p=PeerAuthoritativeProfiles::get()->addOrUpdateProfile(id,idkey,name,profileImage);
                            qDebug() << "Successfully added new user:" << name << id << idkey;
                            p.isOnline=1;
                            auto &node=getNodeFromSocket(sck,connectedOtherNodes);
                            if(node.status != -1)
                                node.profile=p;
                            sendAnyQueuedMessages();
                            emit showUpdatedProfiles();
                        }
                        else {
                            sendInvalidIdKeyResponse(sck);
                            qDebug() << "Received invalid credentials..." << name << id << idkey << "from:" << sck->peerAddress().toString() << "port:" << sck->peerPort();
                            emit showUpdatedProfiles();
                        }

                    }
                    else if(p.status==-2) { //Correct id but bad idkey :)
                        sendInvalidIdKeyResponse(sck);
                        qDebug() << "Received invalid credentials..." << name << id << idkey;
                        emit showUpdatedProfiles();
                    }
                }
            }
            else if(messageType==1) { // Group Game Random Data Pool Message
            }
        }
        else if(messageClass==1) { //Error Message
            if(messageType==0) { //Invalid Idkey Error Message
                //refreshAsNewUser(sck);
            }
        }
    }

//    void refreshAsNewUser(QWebSocket *sck) {
//        qDebug() << "Received invalid IdKey Error...Regenerating a new id and idkey for you...";
//        Profile &p=PeerAuthoritativeProfiles::get()->getMyActiveProfile();
//        p.id=tr("%1").arg(r.bounded(100000000,875000000));
//        p.idkey=tr("%1").arg(r.bounded(100000000,875000000));
//        p.newId=p.id;
//        p.newIdKey=p.idkey;
//        PeerAuthoritativeProfiles::get()->addOrUpdateMyProfile(0,p.id,p.idkey,p.name);
//        //sck->close();
//        sendUpdateProfileMessage(sck);
//        sck->close();
//        //emit showUpdatedProfiles();
//    }

    void printDiscoveredNodes() {
        qDebug() << peerId() << "{Displaying UDP Broadcast Discovered Nodes:}" << discoveredOtherNodes.size();
        for(auto &n : discoveredOtherNodes) {
            qDebug() << tr("%1:%2").arg(n.addr.toString()).arg(n.port);
        }
    }
    void printConnectedNodes() {
        qDebug() << peerId() << "{Displaying Currently Connected Nodes:}" << connectedOtherNodes.size();
        for(auto &n : connectedOtherNodes) {
            qDebug() << tr("[%1] %2").arg(peerWebSocketUrl(n)).arg(n.profile.name);
        }
    }
};

#endif // LOCALNETWORKCHAT_H
