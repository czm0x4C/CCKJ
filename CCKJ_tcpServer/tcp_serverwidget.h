#ifndef TCP_SERVERWIDGET_H
#define TCP_SERVERWIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include "qsettings.h"
#include "tcp_serverworker.h"


QT_BEGIN_NAMESPACE
namespace Ui { class TCP_ServerWidget; }
QT_END_NAMESPACE

class TCP_ServerWidget : public QWidget
{
    Q_OBJECT

public:
    TCP_ServerWidget(QWidget *parent = nullptr);
    ~TCP_ServerWidget();

    void UI_SocketInformationConnectShow(QString TcpClientIP, QString TcpClientPort,unsigned int ClientNumber);

    void UI_SocketInformationDisonnectShow(QString TcpClientIP,QString TcpClientPort, unsigned int ClientNumber);

    void pictureShowFromData(QByteArray pictureData);

    void on_showLogMessage(QByteArray logMessage);

signals:
    void StartListenTCPClientSignal(QString TcpServerIP,unsigned short TcpServerPort);/*监听的端口信号*/

    void ProgramOverSignal();/*结束监听信号*/

    void ServerSendDataToClientSignal(unsigned int ClientID,QByteArray ServerData);

    void appLogMessage_signal(QByteArray logMessage);


private slots:
    void on_TCPServerListenPushButton_clicked();

    void on_TCPServerCloseListenPushButton_clicked();

private:
    Ui::TCP_ServerWidget *ui;

    QTcpServer *TCP_Server;

    QThread *TaskOne;

    TCP_ServerWorker *TaskOneWorker;

    QSettings *userSetting;

    void getLocalNetInformation();

    void openEventSetting();

};
#endif // TCP_SERVERWIDGET_H
