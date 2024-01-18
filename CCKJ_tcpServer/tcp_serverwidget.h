#ifndef TCP_SERVERWIDGET_H
#define TCP_SERVERWIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include "qsettings.h"
#include "tcp_serverworker.h"
#include "mytcpsocket.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TCP_ServerWidget; }

QT_END_NAMESPACE

class TCP_ServerWidget : public QWidget
{
    Q_OBJECT

public:
    TCP_ServerWidget(QWidget *parent = nullptr);
    ~TCP_ServerWidget();

    void UI_SocketInformationConnectShow(QHostAddress ip,uint16_t port);

    void UI_SocketInformationDisonnectShow(QHostAddress ip,uint16_t port);

    void pictureShowFromData(QByteArray pictureData);

    void on_showLogMessage(QByteArray logMessage);

signals:
    void StartListenTCPClientSignal(QString TcpServerIP,unsigned short TcpServerPort);/*监听的端口信号*/

    void ProgramOverSignal();/*结束监听信号*/

    void ServerSendDataToClientSignal(unsigned int ClientID,QByteArray ServerData);

    void appLogMessage_signal(QByteArray logMessage);
protected:

    virtual void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_TCPServerListenPushButton_clicked();

    void on_TCPServerCloseListenPushButton_clicked();

private:
    Ui::TCP_ServerWidget *ui;

    uint32_t ClientNumber = 0;              /* 记录连接的客户端数量 */

    myTcpServer *mmyTcpServer;

    QThread *TaskOne;

    TCP_ServerWorker *TaskOneWorker;

    QSettings *userSetting;

    QList<QWidget*> m_Widget;			    //存储所有的子控件

    QMap<QWidget*, QRect> m_WidgetRect;		//保存每个子控件的初始大小

    void getLocalNetInformation();

    void openEventSetting();

};
#endif // TCP_SERVERWIDGET_H
