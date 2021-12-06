#ifndef HTTPSCLIENT_H
#define HTTPSCLIENT_H

#include <QSslSocket>
#include <QDateTime>
#include <QRandomGenerator>
#include <QString>
#include "qcompressor.h"

class cooookies
{
public:
    inline static QList<QString> jarOfCookies;
    inline static QString urlEncode(QString str)
    {
        std::string url_encoded_str;
        char c,bufHex[10];
        int ic;
        std::string String=str.toUtf8().toStdString();
        const char *chars=String.c_str();
        size_t len=String.length();
        for(size_t i=0; i < len; i++)
        {
            c=chars[i];
            ic=c;
            if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~')
                url_encoded_str+=c;
            else
            {
                sprintf(bufHex,"%X",c);
                if(ic < 16)
                    url_encoded_str+="%0";
                else
                    url_encoded_str+="%";
                url_encoded_str+=bufHex;
            }
        }
        return url_encoded_str.c_str();
    }
};

class httpsrequest
{
public:
    enum class protocol
    {
        HTTP,
        HTTPS,
    };
    enum class type
    {
        GET,
        POST,
    };
    enum class multipart
    {
        OCTET_STREAM,
        TEXT,
        PNG,
        JPG,
        GIF,
    };
    protocol proto=protocol::HTTPS;
    type type=type::GET;
    QString url,host,action,cookies,headers,custom,authorization,userAgent,accept,acceptLanguage,acceptEncoding,referer,origin,dnt,upgrade,contentType,boundary;
    QByteArray text,content;
    uint16_t port=443;
    bool keepalive=true;
    httpsrequest(enum type t=type::GET,QString requesturl="",QString data="",QString contenttype="application/json")
    {
        type=t;
        userAgent="Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:75.0) Gecko/20100101 Firefox/75.0"; //Default user agent string: x64 win10 firefox 75.0
        accept="text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
        acceptLanguage="en-US,en;q=0.5";
        acceptEncoding="gzip, deflate, br";
        referer="";
        origin="";
        dnt="1";
        upgrade="1";
        contentType=contenttype; //application/x-www-form-urlencoded
        boundary=getRandomBoundary();
        addContent(data);
        setUrl(requesturl);
    }
    inline static httpsrequest get(QString requesturl="",QString data="")
    {
        return httpsrequest(type::GET,requesturl,data);
    }
    inline static httpsrequest post(QString requesturl="",QString data="",QString contenttype="application/json")
    {
        return httpsrequest(type::POST,requesturl,data,contenttype);
    }
    void setUrl(QString requesturl)
    {
        url=requesturl;
        if(requesturl.isEmpty()) return;
        if(requesturl.startsWith("https://"))
        {
            proto=protocol::HTTPS;
            port=443;
            requesturl.remove(0,8);
        }
        else if(url.startsWith("http://"))
        {
            proto=protocol::HTTP;
            port=80;
            requesturl.remove(0,7);
        }
        //ex. url=www.whatever.com/?request=x
        //->proto=protocol::HTTPS
        //->port=443
        //->host=www.whatever.com
        //->action=/?request=x
        int firstForwardSlash=requesturl.indexOf("/");
        host=requesturl.left(firstForwardSlash);
        action=requesturl.mid(firstForwardSlash);
        if(type==type::GET)
            headers=QString("GET %1 HTTP/1.1\r\n").arg(action);
        else if(type==type::POST)
            headers=QString("POST %1 HTTP/1.1\r\n").arg(action);
    }
    void addCustomHeader(QString name,QString value)
    {
        custom+=name+": "+value+"\r\n";
    }
    void addContent(QString data)
    {
        content+=data.toUtf8();
    }
    void addMultiPartData(enum multipart mp,QString name,QByteArray data)
    {
        contentType="multipart/form-data; boundary="+boundary;
        content+=("--"+boundary+"\r\n").toUtf8();
        if(mp==multipart::OCTET_STREAM)
        {
            content+=("Content-Type: application/octet-stream\r\nContent-Disposition: form-data; name=\"file\"; filename=\""+name+"\"; filename*=utf-8''"+name+"\r\n").toUtf8();
        }
        else if(mp==multipart::TEXT)
        {
            content+=("Content-Type: text/plain; charset=utf-8\r\nContent-Disposition: form-data; name=\""+name+"\"\r\n").toUtf8();
        }
        else if(mp==multipart::PNG)
        {
            content+=("Content-Type: image/png\r\nContent-Disposition: form-data; name=\"file\"; filename=\""+name+"\"; filename*=utf-8''"+name+"\r\n").toUtf8();
        }
        else if(mp==multipart::JPG)
        {
            content+=("Content-Type: image/jpeg\r\nContent-Disposition: form-data; name=\"file\"; filename=\""+name+"\"; filename*=utf-8''"+name+"\r\n").toUtf8();
        }
        else if(mp==multipart::GIF)
        {
            content+=("Content-Type: image/gif\r\nContent-Disposition: form-data; name=\"file\"; filename=\""+name+"\"; filename*=utf-8''"+name+"\r\n").toUtf8();
        }
        content+=("Content-Length: "+QString("%1\r\n\r\n").arg(data.size())).toUtf8();
        content+=data+"\r\n";
    }
    void generate()
    {
        if(contentType.startsWith("multipart/form-data")) content+=("--"+boundary+"--\r\n").toUtf8();
        headers+="Host: "+host+"\r\n";
        if(!authorization.isEmpty())
            headers+="Authorization: "+authorization+"\r\n";
        headers+="User-Agent: "+userAgent+"\r\n";
        headers+="Accept: "+accept+"\r\n";
        headers+="Accept-Language: "+acceptLanguage+"\r\n";
        headers+="Accept-Encoding: "+acceptEncoding+"\r\n";
        headers+=custom;
        if(!referer.isEmpty())
            headers+="Referer: "+referer+"\r\n";
        if(type==type::POST)
        {
            headers+="Content-Type: "+contentType+"\r\n";
            headers+=QString("Content-Length: %1\r\n").arg(content.size());
            headers+="Origin: "+origin+"\r\n";
        }
        headers+="DNT: "+dnt+"\r\n";
        if(keepalive)
            headers+="Connection: keep-alive\r\n";
        else
            headers+="Connection: close\r\n";
        if(!cookies.isEmpty())
            headers+="Cookie: "+cookies+"\r\n";
        else if(!cooookies::jarOfCookies.isEmpty())
            headers+="Cookie: "+cooookies::jarOfCookies[0]+"\r\n";
        headers+="Upgrade-Insecure-Requests: "+upgrade+"\r\n";
        headers+="\r\n";
        text.clear();
        text.append(headers.toUtf8());
        text+=content;
    }
    void compress()
    {
        QCompressor::gzipCompress(content,content);
        text.clear();
        text.append(headers.toUtf8());
        text+=content;
    }
private:
    QString getRandomBoundary()
    {
        QString boundary=QString().asprintf("%u",QRandomGenerator().bounded(0,9));
        while(boundary.size() < 27)
            boundary+=QString().asprintf("%u",QRandomGenerator().bounded(0,9));
        return boundary;
    }
};

class httpsresponse
{
public:
    QByteArray text,headers,content,uncompressed;
    bool success=false;
    httpsresponse() { }
    httpsresponse(QByteArray bytes)
    {
        setresponse(bytes);
    }
    httpsresponse(QByteArray httpheaders,QByteArray httpscontent) { setresponse(httpheaders,httpscontent); }
    void setresponse(QByteArray httpheaders,QByteArray httpcontent)
    {
        headers=httpheaders;
        content=httpcontent;
        if(headers.size() > 0)
            success=true;
    }
    void setresponse(QByteArray bytes)
    {
        text=bytes;
        if(text.size() > 0)
            success=true;
        int contentIndex=text.indexOf("\r\n\r\n");
        if(contentIndex == -1) return;
        headers=text.left(contentIndex+4);
        content=text.mid(contentIndex+4);
        //qDebug() << "headers size:" << headers.size() << "content size:" << content.size();
        //qDebug() << "headers:" << headers << "\r\ncontent:" << content;
    }
    bool decompress()
    {
        if(QCompressor::gzipDecompress(content,uncompressed))
        {
            content=uncompressed;
            return true;
        }
        return false;
    }
};

class httpsclient : public QObject
{
    Q_OBJECT
private:
    QSslSocket ssl;
    bool connected=false,usecompression=true,keepalive=true;
    int waittime=2000,readsize=0x4000,numretries=3;
    QString authorization,useragent;
    QByteArray compressedcontent,compressedrequest,uncompressedcontent,uncompressedresponse;
    bool open(httpsrequest &request)
    {
        keepalive=request.keepalive;
        if(request.proto==httpsrequest::protocol::HTTPS)
        {
            ssl.connectToHostEncrypted(request.host,request.port);
            if(!ssl.waitForEncrypted())
            {
                qDebug() << "httpsclient error:" << ssl.errorString() << "error:" << ssl.error();
                return connected=false;
            }
        }
        else if(request.proto==httpsrequest::protocol::HTTP)
        {
            ssl.connectToHost(request.host,request.port);
            if(!ssl.waitForConnected())
            {
                qDebug() << "httpsclient error:" << ssl.errorString() << "error:" << ssl.error();
                return connected=false;
            }
        }
        qDebug() << "Connected!";
        return connected=true;
    }
    bool readyRead()
    {
        for(int i=0; i < (waittime); i++)
        {
            if(ssl.waitForReadyRead(1))
                return true;
        }
        return false;
    }
    void close()
    {
        if(connected)
        {
            ssl.close();
            connected=false;
        }
    }
public:
    httpsresponse send(httpsrequest request)
    {
        for(int i=0; i < numretries; i++)
        {
            if(!open(request)) return httpsresponse();

            if(!authorization.isEmpty())
                request.authorization=authorization;
            if(!useragent.isEmpty())
                request.userAgent=useragent;

            if(usecompression)
            {
                request.acceptEncoding="gzip, deflate, br";
                request.generate();
                request.compress();
            }
            else
            {
                request.acceptEncoding="identity";
                request.generate();
            }

            ssl.write(request.text);
            qDebug() << "Request sent! : " << request.text;
            if(readyRead())
            {
                httpsresponse response;
                QByteArray buildresponse=ssl.read(readsize);
                response.setresponse(buildresponse);
                QByteArray keepalive="!";
                QCompressor::gzipCompress(keepalive,keepalive);
                if(response.headers.contains("Transfer-Encoding: chunked"))
                {
                    buildresponse.clear();
                    bool allchunksreceived=false;
                    int chunkindex=0;
                    do
                    {
                        ssl.write(keepalive);
                        if(readyRead())
                        {
                            int contentsize=response.content.size();
                            response.content.resize(contentsize+readsize);
                            qint64 r=ssl.read(response.content.data()+contentsize,readsize);
                            if(r==0 || r==-1)
                                allchunksreceived=true;
                            if(r > 0)
                                response.content.resize(contentsize+(int)r);
                            else
                                response.content.resize(contentsize);
                        }

                        QString extractedChunkSize=extract(response.content,"\r\n",chunkindex);
                        int chunksize=extractedChunkSize.toInt(nullptr,16);
                        if(chunksize==0)
                        {
                            allchunksreceived=true;
                            break;
                        }
                        QByteArray chunk=response.content.mid(chunkindex,chunksize);
                        if(chunk.size()==chunksize)
                        {
                            buildresponse+=chunk;
                            chunkindex+=chunksize+2;
                        }
                    } while(!allchunksreceived);
                    response.content=buildresponse;
                }
                else if(response.headers.contains("Content-Length: "))
                {
                    QString extractedContentLen=extract(response.headers,"Content-Length: ","\r\n");
                    int contentlen=extractedContentLen.toInt();
                    int writeoffset=response.content.size();
                    response.content.resize(contentlen);
                    while(writeoffset < contentlen)
                    {
                        ssl.write(keepalive);
                        if(readyRead())
                        {
                            qint64 r=ssl.read(response.content.data()+writeoffset,readsize);
                            if(r >= 0)
                                writeoffset+=r;
                            if(r==0 || r==-1) break;
                        }
                        else break;
                    }
                }
                close();

                if(usecompression && response.decompress())
                {
                    qDebug() << "Decompressed successfully! size:" << response.content.size() << "->" << response.content;
                }
                else
                {
                    qDebug() << "Response size:" << response.content.size() << "->" << response.content;
                }

                return response;
            }
            close();
        }
        qDebug() << "Failed receiving response after" << numretries << "retries...";
        close();
        return httpsresponse();
    }
    void setUseCompression(bool shouldusecompression=true)
    {
        usecompression=shouldusecompression;
    }
    void setAuthorization(QString auth)
    {
        authorization=auth;
    }
    void setUserAgent(QString userAgent)
    {
        useragent=userAgent;
    }
    inline static QByteArray extract(const QByteArray &from,const char *upto,int &startingpos)
    {
        if(startingpos != -1)
        {
            int extractend=from.indexOf(upto,startingpos);
            if(extractend != -1)
            {
                //qDebug() << "startingpos:" << startingpos << "extractend:" << extractend << "e-s=" << extractend-startingpos;
                QByteArray extraction=from.mid(startingpos,extractend-startingpos);
                startingpos=extractend+(int)strlen(upto);
                return extraction;
            }
        }
        return QByteArray();
    }
    inline static QByteArray extract(QByteArray &from,const char *startingat, const char *upto,int startingpos=0)
    {
        int extractstart=from.indexOf(startingat,startingpos);
        if(extractstart != -1)
        {
            extractstart+=(int)strlen(startingat);
            int extractend=from.indexOf(upto,extractstart);
            if(extractend != -1)
            {
                return from.mid(extractstart,extractend-extractstart);
            }
        }
        return QByteArray();
    }
    inline static QString timestamp()
    {
        return QDateTime::currentDateTime().toString("dddd, MMMM d yyyy hh:mm:ss");
    }
};

#endif // HTTPSCLIENT_H
