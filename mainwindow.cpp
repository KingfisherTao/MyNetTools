#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostInfo>
#include "mserver.h"
#include <QMessageBox>
#include "msocket.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug()<<"char size:"<<sizeof (char);
    m_tcpServer=nullptr;
    m_udpSocket=nullptr;
    m_tcpSocket=nullptr;
    m_serverHexSend=false;
    // 信息收发文本框高度3：1
    ui->splitter->setStretchFactor(0, 3);
    ui->splitter->setStretchFactor(1, 1);
    ui->splitter_2->setStretchFactor(0, 3);
    ui->splitter_2->setStretchFactor(1, 1);
    ui->splitter_3->setStretchFactor(0, 3);
    ui->splitter_3->setStretchFactor(1, 1);
    // 设置端口输入校验
    ui->lineEdit_clientRemotePort->setValidator(new QIntValidator(0,65535));
    ui->lineEdit_serverLocalPort->setValidator(new QIntValidator(0,65535));
    ui->lineEdit_udpLocalPort->setValidator(new QIntValidator(0,65535));
    ui->lineEdit_udpRemotePort->setValidator(new QIntValidator(0,65535));
    // ip校验正则
    QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
    // 设置IP输入校验
    ui->lineEdit_clientRemoteIP->setValidator(new QRegExpValidator(regExpIP ,ui->lineEdit_serverLocalIp));
    ui->lineEdit_serverLocalIp->setValidator(new QRegExpValidator(regExpIP ,ui->lineEdit_serverLocalIp));
    ui->lineEdit_GroupIp->setValidator(new QRegExpValidator(regExpIP ,ui->lineEdit_GroupIp));
    ui->lineEdit_udpLocalIP->setValidator(new QRegExpValidator(regExpIP ,ui->lineEdit_udpLocalIP));
    ui->lineEdit_udpRemoteIP->setValidator(new QRegExpValidator(regExpIP ,ui->lineEdit_udpRemoteIP));
    // 显示本地ip
    QString localip = Get_LocalIp();
    ui->lineEdit_serverLocalIp->setText(localip);
    ui->lineEdit_udpLocalIP->setText(localip);
    ui->lineEdit_udpRemoteIP->setText(localip);
    ui->lineEdit_clientRemoteIP->setText(localip);
    // UDP通讯模式组
    m_udpModeGroup=new QButtonGroup(this);
    m_udpModeGroup->addButton(ui->rad_udpSingle,0);
    m_udpModeGroup->addButton(ui->rad_udpGroup,1);
    m_udpModeGroup->addButton(ui->rad_udpBroadcast,2);
    // 连接信号和槽，radiobutton改变udp模式
    connect(m_udpModeGroup, SIGNAL(buttonClicked(int)),this,SLOT(on_udpModeChange(int)));

    qDebug()<<"UI线程ID:"<<QThread::currentThreadId();
}

MainWindow::~MainWindow()
{

    if(m_tcpServer!=nullptr)
    {
        m_tcpServer->close();
        delete m_tcpServer;
    }

    if(m_tcpSocket!=nullptr)
    {
        m_tcpSocket->abort();
        delete m_tcpSocket;
    }
    if(m_udpSocket!=nullptr)
    {
        m_udpSocket->abort();
        delete m_udpSocket;
    }

    delete ui;
}

// 获取本地IP
QString MainWindow::Get_LocalIp()
{
    // 本地主机名
    QString hostName=QHostInfo::localHostName();
    // 本机IP地址
    QHostInfo hostInfo=QHostInfo::fromName(hostName);
    // IP地址列表
    QList<QHostAddress> addList=hostInfo.addresses();
    for (int i=0;i<addList.count();i++)
    {
        // 每一项是一个QHostAddress
        QHostAddress aHost=addList.at(i);
        if(QAbstractSocket::IPv4Protocol==aHost.protocol())
        {
            // 显示出Ip地址
            return aHost.toString();
        }
    }
    return QString("");
}

// 服务器端收到消息显示
void MainWindow::AddServerMessage(QString message)
{
    // 显示
    ui->text_serverRec->append(message);
    ui->text_serverRec->moveCursor(QTextCursor::End);
}

// udp接收消息
void MainWindow::UdpReadData()
{
    // 获取发来的数据
    QByteArray datagram;
    while (m_udpSocket->hasPendingDatagrams())
    {
        // 等待接收的字节数
        int sizeLen=static_cast<int>(m_udpSocket->pendingDatagramSize());
        datagram.resize(sizeLen);
        // 读取
        m_udpSocket->readDatagram(datagram.data(),datagram.size());

    }

    QString data;
    // hex
    if(ui->chk_udpRecHex->isChecked())
    {
        ByteToHexString(data,datagram);
    }
    // 普通字符串
    else
    {
        data=QString::fromLocal8Bit(datagram);
    }
    // 显示
    ui->text_udpRec->insertPlainText(QString("[%1]：%2\r\n").arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(data));
    ui->text_udpRec->moveCursor(QTextCursor::End);
}

// 点击udp绑定/解绑
void MainWindow::on_btn_udpBind_clicked()
{
    // 解绑udp
    if(m_udpSocket!=nullptr)
    {
        // 断开信号槽
        m_udpSocket->disconnect();
        // 组播模式
        if(ui->chk_udpGroup->isChecked())
        {
            // 获取组播的Ip
            QHostAddress ip(ui->lineEdit_GroupIp->text());
            // 退出组
            m_udpSocket->leaveMulticastGroup(ip);
            ui->lineEdit_GroupIp->setEnabled(true);
        }
        // 关闭udp
        m_udpSocket->abort();
        // 释放
        m_udpSocket->deleteLater();
        m_udpSocket=nullptr;
        // 更新UI
        ui->text_udpRec->appendPlainText("UDP已解绑"+QString("[%1：%2]").arg(ui->lineEdit_udpLocalIP->text()).arg(ui->lineEdit_udpRemoteIP->text()));
        ui->btn_udpBind->setText("绑定");
        ui->btn_udpBind->setIcon(QIcon(":/Resource/Img/start36x36.png"));
        ui->groupBox_udp->setEnabled(true);
        ui->lineEdit_udpLocalIP->setEnabled(true);
        ui->lineEdit_udpLocalPort->setEnabled(true);
    }
    // 绑定端口
    else
    {
        // 获取本地端口
        bool ok;
        quint16 port=ui->lineEdit_udpLocalPort->text().toUShort(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","本地端口设置有误，请重新输入");
            return;
        }
        // 获取本地Ip
        QHostAddress localip(ui->lineEdit_udpLocalIP->text());
        localip.toIPv4Address(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","本地ip设置有误，请重新输入");
            return;
        }

        m_udpSocket=new QUdpSocket(this);
        // 绑定端口
        m_udpSocket->bind(localip,port,QAbstractSocket::ShareAddress);
        // 连接接收信号槽
        connect(m_udpSocket,SIGNAL(readyRead()),this,SLOT(UdpReadData()));
        // 更新UI
        ui->text_udpRec->appendPlainText("UDP已绑定"+QString("[%1：%2]\r\n").arg(ui->lineEdit_udpLocalIP->text()).arg(ui->lineEdit_udpRemoteIP->text()));
        ui->btn_udpBind->setText("解绑");
        ui->btn_udpBind->setIcon(QIcon(":/Resource/Img/stop36x36.png"));
        ui->groupBox_udp->setEnabled(false);
        ui->lineEdit_udpLocalIP->setEnabled(false);
        ui->lineEdit_udpLocalPort->setEnabled(false);
    }
}

// 改变udp通信模式
void MainWindow::on_udpModeChange(int id)
{
    switch (id)
    {
    // 单播
    case 0:
        ui->lineEdit_udpRemoteIP->setEnabled(true);
        ui->lineEdit_udpLocalPort->setEnabled(true);
        ui->lineEdit_udpLocalIP->setEnabled(true);
        ui->lineEdit_udpLocalPort->setEnabled(true);
        ui->chk_udpGroup->setChecked(false);
        ui->chk_udpGroup->setEnabled(false);
        ui->lineEdit_GroupIp->setEnabled(false);
        break;
        // 组播
    case 1:
        ui->lineEdit_udpRemoteIP->setEnabled(true);
        ui->lineEdit_udpLocalPort->setEnabled(true);
        ui->lineEdit_udpLocalIP->setEnabled(true);
        ui->lineEdit_udpLocalPort->setEnabled(true);
        ui->chk_udpGroup->setEnabled(true);
        ui->lineEdit_GroupIp->setEnabled(true);
        break;
        // 广播
    case 2:
        ui->lineEdit_udpRemoteIP->setEnabled(false);
        ui->lineEdit_udpLocalPort->setEnabled(false);
        ui->lineEdit_udpLocalIP->setEnabled(true);
        ui->lineEdit_udpLocalPort->setEnabled(true);
        ui->chk_udpGroup->setChecked(false);
        ui->chk_udpGroup->setEnabled(false);
        ui->lineEdit_GroupIp->setEnabled(false);
        break;
    default:break;
    }
}

// 点击udp发送
void MainWindow::on_btn_udpSend_clicked()
{
    // 获取远程端口
    bool ok;
    quint16 port=ui->lineEdit_udpRemotePort->text().toUShort(&ok);
    if(!ok)
    {
        QMessageBox::warning(this,"错误","远程端口设置有误，请重新输入");
        return;
    }

    QByteArray byteArray;
    QString data=ui->text_udpSend->toPlainText();
    // Hex发送
    if(ui->chk_udpSendHex->isChecked())
    {
        // hex字符串转字节
        if(!HexStringToByte(data,byteArray))
        {
            QMessageBox::information(this,"提示","输入的十六进制字符串有误，请重新输入");
            return;
        }
    }
    // 普通字符串发送
    else
    {
        byteArray= data.toLocal8Bit();
    }


    // 单播
    if(ui->rad_udpSingle->isChecked())
    {
        // 获取远程Ip
        QHostAddress remoteip(ui->lineEdit_udpRemoteIP->text());
        remoteip.toIPv4Address(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","远程ip设置有误，请重新输入");
            return;
        }

        // 发送
        m_udpSocket->writeDatagram(byteArray,remoteip,port);

    }
    // 组播
    else if(ui->rad_udpGroup->isChecked())
    {
        // 获取远程Ip
        QHostAddress remoteip(ui->lineEdit_udpRemoteIP->text());
        remoteip.toIPv4Address(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","远程ip设置有误，请重新输入");
            return;
        }
        // D类IP地址的范围为224.0.0.0～239.255.255.255。前4比特固定为1110，后28比特是组播组地址标识(ID)
        QPair<QHostAddress, int> subnet = QHostAddress::parseSubnet("224.0.0.0/4");
        bool is_DClass = remoteip.isInSubnet(subnet);
        if(!is_DClass)
        {
            QMessageBox::warning(this,"错误","远程ip应在D类IP地址范围224.0.0.0～239.255.255.255内，请重新输入");
            return;
        }
        // 发送
        m_udpSocket->writeDatagram(byteArray,remoteip,port);
    }
    // 广播
    else
    {
        // 发送
        m_udpSocket->writeDatagram(byteArray,QHostAddress::Broadcast,port);
    }
}

// 加入\退出组播
void MainWindow::on_chk_udpGroup_clicked(bool checked)
{
    static QHostAddress currentIP;
    if(checked)
    {
        if(m_udpSocket == nullptr)
        {
            QMessageBox::warning(this,"警告","请先绑定本地IP和端口");
            ui->chk_udpGroup->setChecked(false);
            return;
        }
        // 获取组播Ip
        bool ok;
        QHostAddress groupip(ui->lineEdit_GroupIp->text());
        groupip.toIPv4Address(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","组播IP输入有误，请重新输入");
            ui->chk_udpGroup->setChecked(false);
            return;
        }
        // D类IP地址的范围为224.0.0.0～239.255.255.255。前4比特固定为1110，后28比特是组播组地址标识(ID)
        QPair<QHostAddress, int> subnet = QHostAddress::parseSubnet("224.0.0.0/4");
        bool is_DClass = groupip.isInSubnet(subnet);
        if(!is_DClass)
        {
            QMessageBox::warning(this,"错误","组播IP应在D类IP地址范围224.0.0.0～239.255.255.255内，请重新输入");
            ui->chk_udpGroup->setChecked(false);
            return;
        }
        ui->lineEdit_GroupIp->setEnabled(false);
        //加入组
        m_udpSocket->joinMulticastGroup(groupip);
        currentIP = groupip;//记录ip
    }
    else
    {
        //退出组
        m_udpSocket->leaveMulticastGroup(currentIP);
        ui->lineEdit_GroupIp->setEnabled(true);
    }
}



// tcp客户端读取数据
void MainWindow::ClinetReadData()
{
    QByteArray ba = m_tcpSocket->readAll();
    QString data;
    // hex
    if(ui->chk_clientRecHex->isChecked())
    {
        ByteToHexString(data,ba);
    }
    else// 普通字符串
    {
        data=QString::fromLocal8Bit(ba);
    }
    // 显示
    ui->text_clientRec->insertPlainText(QString("[%1|%2:%3]：%4\r\n")
                                        .arg(QTime::currentTime().toString("hh:mm:ss.zzz"))
                                        .arg(ui->lineEdit_serverLocalIp->text()).arg(ui->lineEdit_serverLocalPort->text())
                                        .arg(data));
    ui->text_clientRec->moveCursor(QTextCursor::End);
}

// tcp客户端发生错误
void MainWindow::ClientReadError(QAbstractSocket::SocketError)
{
    QString err=QString("发生错误:%1").arg(m_tcpSocket->errorString());
    ui->text_clientRec->appendPlainText(err);
    // 断开所有信号
    m_tcpSocket->disconnect();
    // 终止socket连接
    m_tcpSocket->abort();
    // 释放
    m_tcpSocket->deleteLater();
    m_tcpSocket=nullptr;
    // 更新UI
    ui->text_clientRec->appendPlainText("已断开服务器"+QString("[%1：%2]\r\n").arg(ui->lineEdit_clientRemoteIP->text()).arg(ui->lineEdit_clientRemotePort->text()));
    ui->btn_clientConnect->setText("连接");
    ui->btn_clientConnect->setIcon(QIcon(":/Resource/Img/stop36x36.png"));
    ui->lineEdit_clientRemoteIP->setEnabled(true);
    ui->lineEdit_clientRemotePort->setEnabled(true);

}

// tcp客户端连接/断开
void MainWindow::on_btn_clientConnect_clicked()
{
    if(m_tcpSocket == nullptr)
    {
        // 获取本地端口
        bool ok;
        quint16 port=ui->lineEdit_clientRemotePort->text().toUShort(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","远程端口设置有误，请重新输入");
            return;
        }
        // 获取本地Ip
        QHostAddress ip(ui->lineEdit_clientRemoteIP->text());
        ip.toIPv4Address(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","远程ip设置有误，请重新输入");
            return;
        }

        m_tcpSocket=new QTcpSocket(this);
        m_tcpSocket->connectToHost(ip,port);
        connect(m_tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(ClientReadError(QAbstractSocket::SocketError)));
        // 等待连接
        if (m_tcpSocket->waitForConnected(1000))
        {
            connect(m_tcpSocket,SIGNAL(readyRead()),this,SLOT(ClinetReadData()));

            //更新UI
            ui->text_clientRec->appendPlainText("已连接服务器"+QString("[%1：%2]\r\n").arg(ui->lineEdit_clientRemoteIP->text()).arg(ui->lineEdit_clientRemotePort->text()));
            ui->btn_clientConnect->setText("断开");
            ui->btn_clientConnect->setIcon(QIcon(":/Resource/Img/stop36x36.png"));
            ui->lineEdit_clientRemoteIP->setEnabled(false);
            ui->lineEdit_clientRemotePort->setEnabled(false);
        }
        // 连接失败
        else
        {
            m_tcpSocket->disconnect();
            m_tcpSocket->deleteLater();
            m_tcpSocket=nullptr;
            ui->text_clientRec->appendPlainText("连接失败"+QString("[%1：%2]\r\n").arg(ui->lineEdit_clientRemoteIP->text()).arg(ui->lineEdit_clientRemotePort->text()));
        }

    }
    else
    {
        m_tcpSocket->disconnect();//断开信号槽
        m_tcpSocket->abort();//终止
        m_tcpSocket->deleteLater();//释放
        m_tcpSocket=nullptr;
        // 更新UI
        ui->text_clientRec->appendPlainText("已断开服务器"+QString("[%1：%2]\r\n").arg(ui->lineEdit_clientRemoteIP->text()).arg(ui->lineEdit_clientRemotePort->text()));
        ui->btn_clientConnect->setText("连接");
        ui->btn_clientConnect->setIcon(QIcon(":/Resource/Img/start36x36.png"));
        ui->lineEdit_clientRemoteIP->setEnabled(true);
        ui->lineEdit_clientRemotePort->setEnabled(true);
    }
}

// 字节数组转16进制字符串
void MainWindow::ByteToHexString(QString &str, QByteArray &ba)
{
    // str= ba.toHex();//直接转换中间没有空格
    for (int i = 0; i < ba.length(); i++)
    {
        unsigned char n =static_cast<unsigned char>((ba.at(i)));
        QString nhex = QByteArray::number(n, 16).toUpper();
        str.append(nhex);
        // 字节间加空格
        str.append(' ');
    }
}

// 16进制字符串转字节数组
// str：输入字符串，ba：输出字节数组
bool MainWindow::HexStringToByte(QString &str,QByteArray &ba)
{
    // 正则:数字0-9字母a-f、A-F匹配大于等于一次
    QString pattern("[a-fA-F0-9]+");
    QRegExp rx(pattern);
    // 删除所有空格
    str = str.replace(' ', "");
    // 匹配
    if (rx.exactMatch(str))
    {
        bool ok;
        int length = str.length();
        // 双数
        if ((length % 2)==0)
        {
            for (int i = 0; i < length; i+=2)
            {
                // 每两个字符对应一个hex字符串
                QString str_hex = str.mid(i, 2);
                // hex字符串转整数值
                ba.append(static_cast<char>(str_hex.toInt(&ok, 16)));
            }
        }
        // 单数
        else
        {
            for (int i = 0; i < length-1; i += 2)
            {
                // 每两个字符对应一个hex字符串
                QString str_hex = str.mid(i, 2);
                // hex字符串转整数值
                ba.append(static_cast<char>(str_hex.toInt(&ok, 16)));
            }
            // 最后一个单独处理
            // hex字符串转整数值
            ba.append(static_cast<char>(str.mid(length - 1,1).toInt(&ok, 16)));
        }
        return true;
    }
    else
    {
        return false;
    }
}

// 点击TCP服务器发送
void MainWindow::on_btn_serverSend_clicked()
{
    if(m_tcpServer!=nullptr && !m_tcpServer->isListening())
    {
        QMessageBox::information(this,"提示","没有建立TCP连接");
        return;
    }
    if(ui->comboBox->count()==1)
    {
        QMessageBox::information(this,"提示","没有连接任何客户端");
        return;
    }

    QByteArray byteArray;
    QString data =ui->text_serverSend->toPlainText();
    // Hex发送
    if(ui->chk_serverSendHex->isChecked())
    {
        // hex字符串转字节
        if(!HexStringToByte(data,byteArray))
        {
            QMessageBox::information(this,"提示","输入的十六进制字符串有误，请重新输入");
            return;
        }
    }
    // 普通字符串发送
    else
        byteArray= data.toLocal8Bit();

    int n=ui->comboBox->currentIndex();
    // 发送给所有连接
    if(n==0)
    {
        for (int i=1;i<ui->comboBox->count();i++)
        {
            MSocket* socket = ui->comboBox->itemData(i).value<MSocket*>();
            if(socket !=nullptr)
                emit socket->WriteMessage(byteArray);
        }
    }
    // 发送给选择的客户端
    else
    {
        MSocket* mysocket = ui->comboBox->itemData(n).value<MSocket*>();
        if(mysocket !=nullptr)
            emit mysocket->WriteMessage(byteArray);
    }
}

// 点击tcp服务器监听/停止
void MainWindow::on_btn_serverListering_clicked()
{
    // 未连接
    if(m_tcpServer==nullptr)
    {
        bool ok=false;
        // 获取监听的端口号
        quint16 port=ui->lineEdit_serverLocalPort->text().toUShort(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","端口设置有误，请重新输入");
            return;
        }
        // 获取监听的Ip
        QHostAddress ip(ui->lineEdit_serverLocalIp->text());
        ip.toIPv4Address(&ok);
        if(!ok)
        {
            QMessageBox::warning(this,"错误","ip设置有误，请重新输入");
            return;
        }

        m_tcpServer=new MServer(this);
        // 设置线程
        m_tcpServer->SetThread(ui->spinBox_threadNum->value()-1);
        // 监听
        bool islisten = m_tcpServer->listen(ip, port);
        if(!islisten)
        {
            QMessageBox::warning(this,"错误",m_tcpServer->errorString());
            m_tcpServer->close();
            // 释放
            m_tcpServer->deleteLater();
            m_tcpServer=nullptr;
            return;
        }
        // 更新ui
        ui->text_serverRec->append("开始监听"+QString("[%1：%2]\r\n").arg(ui->lineEdit_serverLocalIp->text()).arg(ui->lineEdit_serverLocalPort->text()));//消息框提示信息
        ui->btn_serverListering->setText("停止");
        ui->btn_serverListering->setIcon(QIcon(":/Resource/Img/stop36x36.png"));
        ui->lineEdit_serverLocalIp->setEnabled(false);
        ui->lineEdit_serverLocalPort->setEnabled(false);
        ui->spinBox_threadNum->setEnabled(false);
    }
    // 停止server
    else
    {
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer=nullptr;
        // 更新ui
        ui->text_serverRec->append("停止监听"+QString("[%1：%2]\r\n").arg(ui->lineEdit_serverLocalIp->text()).arg(ui->lineEdit_serverLocalPort->text()));//消息框提示信息
        ui->btn_serverListering->setText("监听");
        ui->btn_serverListering->setIcon(QIcon(":/Resource/Img/start36x36.png"));
        ui->lineEdit_serverLocalIp->setEnabled(true);
        ui->lineEdit_serverLocalPort->setEnabled(true);
        ui->spinBox_threadNum->setEnabled(true);
    }
}

// 清除tcp服务器接收
void MainWindow::on_btn_serverClearRec_clicked()
{
    ui->text_serverRec->clear();
}

// 清除tcp服务器发送
void MainWindow::on_btn_serverClearSend_clicked()
{
    ui->text_serverSend->clear();
}

// 清除tcp客户端接收
void MainWindow::on_btn_clientClearRec_clicked()
{
    ui->text_clientRec->clear();
}

// 清除tcp客户端发送
void MainWindow::on_btn_clientClearSend_clicked()
{
    ui->text_clientSend->clear();
}

// 清除udp接收
void MainWindow::on_btn_udpClearRec_clicked()
{
    ui->text_udpRec->clear();
}

// 清除udp发送
void MainWindow::on_btn_udpClearSend_clicked()
{
    ui->text_udpSend->clear();
}

// 多线程，公开tcp服务器hex发送标志
void MainWindow::on_chk_ServerRecHex_clicked(bool checked)
{
    m_serverHexSend=checked;
}

// 点击tcp客户端发送
void MainWindow::on_btn_clientSend_clicked()
{
    QByteArray byteArray;
    QString data=ui->text_clientSend->toPlainText();
    // Hex发送
    if(ui->chk_clientSendHex->isChecked())
    {
        // hex字符串转字节
        if(!HexStringToByte(data,byteArray))
        {
            QMessageBox::information(this,"提示","输入的十六进制字符串有误，请重新输入");
            return;
        }
    }
    // 普通字符串发送
    else
        byteArray= data.toLocal8Bit();
    m_tcpSocket->write(byteArray);
}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    ui->btn_serverCloseSocket->setEnabled(0==index?false:true);
}

void MainWindow::on_btn_serverCloseSocket_clicked()
{
    int index=ui->comboBox->currentIndex();
    if(index==0) return;
    // 关闭socket
    emit m_tcpServer->list_information[index-1].socket->DeleteSocket();
}
