#ifndef SUSPENDABLE_H
#define SUSPENDABLE_H

#include <QPointer>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QWaitCondition>
#include <QDebug>

//Thanks to Andrei Smirnov!
//SuspendableWorker and Thread are for managing threads of objects that require signals/slots working (requiring a QEventLoop)
class SuspendableWorker : public QObject
{
    Q_OBJECT
private:
    QMutex waitMutex;
    QWaitCondition waitCondition;
private slots:
    void suspendImpl()
    {
        waitCondition.wait(&waitMutex);
    }
public:
    std::atomic<bool> suspended=false;
    explicit SuspendableWorker(QObject *parent=nullptr) : QObject(parent)
    {
        waitMutex.lock();
        qDebug() << "Worker created!";
    }
    ~SuspendableWorker()
    {
        waitCondition.wakeAll();
        waitMutex.unlock();
        qDebug() << "Worker destroyed...";
    }
    // resume() must be called from the outer thread.
    void resume()
    {
        suspended=false;
        waitCondition.wakeAll();
    }
    // suspend() must be called from the outer thread.
    // the function would block the caller's thread until
    // the worker thread is suspended.
    void suspend()
    {
        suspended=true;
        QMetaObject::invokeMethod(this, &SuspendableWorker::suspendImpl);
        // acquiring mutex to block the calling thread
        waitMutex.lock();
        waitMutex.unlock();
    }
};

template <typename TWorker> class Thread : public QThread
{
public:
    TWorker *worker;
    explicit Thread(TWorker *_worker,QObject *parent=nullptr) : QThread(parent),worker(_worker)
    {
        worker->moveToThread(this);
        start();
    }
    ~Thread()
    {
        resume();
        quit();
        wait();
    }
    void suspend()
    {
        auto suspendableworker=qobject_cast<SuspendableWorker*>(worker);
        if (suspendableworker != nullptr)
            suspendableworker->suspend();
    }
    void resume()
    {
        auto suspendableworker=qobject_cast<SuspendableWorker*>(worker);
        if (suspendableworker != nullptr)
            suspendableworker->resume();
    }
protected:
    void run() override
    {
            qDebug() << QThread::currentThread() << "started...";
            QThread::run();
            delete worker;
            qDebug() << "Worker deleted, Thread exited...";
    }
};

//SuspendableThread is for creating auto starting suspendable threads when no signals/slots are required for the thread
class SuspendableThread : public QThread
{
private:
    inline static std::atomic<bool> allthreadsshouldstop=false,allthreadssuspended=false;
    std::atomic<bool> threadrunning=false,threadsuspended=false,threadshouldstop=false,shouldrunonce=false;
    std::function<void()> executefunction;
public:
    explicit SuspendableThread(std::function<void()> func,bool runonce=false,QObject *parent=nullptr) : QThread(parent)
    {
        executefunction=func;
        shouldrunonce=runonce;
        start();
    }
    ~SuspendableThread()
    {
        stop();
    }
    inline static void setAllShouldStop(bool shouldstop=true) { allthreadsshouldstop=shouldstop; }
    inline static bool shouldAllStop() { return allthreadsshouldstop; }
    inline static void setAllSuspended(bool allsuspended=true) { allthreadssuspended=allsuspended; }
    inline static void setAllResume(bool allresume=true) { allthreadssuspended=!allresume; }
    void setShouldStop(bool shouldstop=true) { threadshouldstop=shouldstop; }
    bool shouldStop() { return threadshouldstop; }
    void stop() { setShouldStop(); while(threadrunning) { msleep(1); } }
    void suspend() { threadsuspended=true; }
    void resume() { threadsuspended=false; }
protected:
    void run() override
    {
        threadrunning=true;
        qDebug() << QThread::currentThread() << "started!";
        if(shouldrunonce)
            executefunction();
        else
        {
            forever
            {
                if(threadshouldstop || allthreadsshouldstop)
                    break;
                if(!threadsuspended && !allthreadssuspended)
                    executefunction();
                else
                    msleep(1);
            }
        }
        threadrunning=threadsuspended=threadshouldstop=false;
        qDebug() << QThread::currentThread() << "exited!";
    }
};

//This is a non-blocking time delayer for threads
class Waiter : QObject
{
    Q_OBJECT
public:
    qint64 waitUntilTime=0,sleepTime=1;
    Waiter(qint64 delay=1000)
    {
        waitUntilTime=QDateTime::currentMSecsSinceEpoch()+delay;
    }
    bool timeNotElapsed()
    {
        QThread::msleep(sleepTime);
        return waitUntilTime > QDateTime::currentMSecsSinceEpoch();
    }
    void wait(std::function<void()> func=[](){})
    {
        while(waitUntilTime > QDateTime::currentMSecsSinceEpoch())
        {
            func();
            QThread::msleep(sleepTime);
        }
    }
};

#endif // SUSPENDABLE_H
