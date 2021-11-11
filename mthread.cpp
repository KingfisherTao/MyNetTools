#include "mthread.h"
#include "mserver.h"
#include "msocket.h"
#include "mainwindow.h"

MThread::MThread(QObject *parent)
{
    this->m_server=static_cast<MServer*>(parent);
    this->sockethelper=nullptr;
    this->ThreadLoad = 0;
}

MThread::~MThread()
{
    if(this->sockethelper!=nullptr)
    {
        sockethelper->disconnect();
        delete this->sockethelper;//释放sockethelper
    }
}

void MThread::run()
{
    //在线程内创建对象，槽函数在这个线程中执行
    this->sockethelper=new SocketHelper(this->m_server);
    connect(sockethelper,&SocketHelper::Create,sockethelper,&SocketHelper::CreateSocket);
    connect(sockethelper,&SocketHelper::AddList,m_server,&MServer::AddInf);
    connect(sockethelper,&SocketHelper::RemoveList,m_server,&MServer::RemoveInf);

    exec();
}

SocketHelper::SocketHelper(QObject *parent)
{
    this->myserver=static_cast<MServer*>(parent);

}

void SocketHelper::CreateSocket(qintptr socketDescriptor,int index)
{
    qDebug()<<"子线程:"<<QThread::currentThreadId();

    MSocket* tcpsocket = new MSocket(this->myserver);
    tcpsocket->sockethelper = this;
    // 初始化socket
    tcpsocket->setSocketDescriptor(socketDescriptor);
    // 发送到UI线程记录信息
    emit AddList(tcpsocket,index);
    // 非UI线程时
    if( index!= -1)
    {
        // 负载+1
        myserver->list_thread[index]->ThreadLoad+=1;//负载+1
        // 关联释放socket,非UI线程需要阻塞
        connect(tcpsocket , &MSocket::DeleteSocket , tcpsocket, &MSocket::deal_disconnect,Qt::ConnectionType::BlockingQueuedConnection);
    }
    else
    {
        connect(tcpsocket , &MSocket::DeleteSocket , tcpsocket, &MSocket::deal_disconnect,Qt::ConnectionType::AutoConnection);
    }

    // 关联显示消息
    connect(tcpsocket,&MSocket::AddMessage,myserver->mainwindow,&MainWindow::AddServerMessage);
    // 发送消息
    connect(tcpsocket,&MSocket::WriteMessage,tcpsocket,&MSocket::deal_write);
    // 关联接收数据
    connect(tcpsocket , &MSocket::readyRead , tcpsocket , &MSocket::deal_readyRead);
    // 关联断开连接时的处理槽
    connect(tcpsocket , &MSocket::disconnected , tcpsocket, &MSocket::deal_disconnect);

    QString ip = tcpsocket->peerAddress().toString();
    quint16 port = tcpsocket->peerPort();
    QString message = QString("[%1:%2] 已连接").arg(ip).arg(port);
    // 发送到UI线程显示
    emit tcpsocket->AddMessage(message);
}

