#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTcpSocket>
#include <QUdpSocket>
#include "mserver.h"
#include "msocket.h"
#include <QButtonGroup>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow *ui;
    // tcp服务器hex发送标志
    bool m_serverHexSend;
    // 16进制字符串转字节数组
    bool HexStringToByte(QString &str,QByteArray &ba);
    // 字节数组转16进制字符串
    void ByteToHexString(QString &str,QByteArray &ba);
    // 获取本地IP
    QString Get_LocalIp();

private:  

    MServer* m_tcpServer;
    QButtonGroup* m_udpModeGroup;
    QUdpSocket* m_udpSocket;
    QTcpSocket* m_tcpSocket;

private slots:
    void on_btn_udpBind_clicked();
    void on_btn_udpSend_clicked();
    void on_udpModeChange(int id);
    void on_chk_udpGroup_clicked(bool checked);
    void on_btn_clientConnect_clicked();
    void on_btn_serverSend_clicked();
    void on_btn_serverListering_clicked();
    void on_btn_serverClearRec_clicked();
    void on_btn_serverClearSend_clicked();
    void on_btn_clientClearRec_clicked();
    void on_btn_clientClearSend_clicked();
    void on_btn_udpClearRec_clicked();
    void on_btn_udpClearSend_clicked();
    void on_chk_ServerRecHex_clicked(bool checked);
    void on_btn_clientSend_clicked();
    void on_comboBox_currentIndexChanged(int index);
    void on_btn_serverCloseSocket_clicked();

public slots:
    void AddServerMessage(QString message);
    void UdpReadData();
    void ClinetReadData();
    void ClientReadError(QAbstractSocket::SocketError);

};
#endif // MAINWINDOW_H
