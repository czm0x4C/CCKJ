#include "tcp_serverwidget.h"
#include "ui_tcp_serverwidget.h"

#include <QThread>
#include <QNetworkInterface>

TCP_ServerWidget::TCP_ServerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TCP_ServerWidget)
{
    ui->setupUi(this);
    this->setWindowTitle("TCP_Sercer_CCKJ");
    openEventSetting();
    /*窗口的构造函数中初始化相关参数*/
    /*程序调试时，默认监听8000端口*/

    ui->logShowTextEdit->setReadOnly(true);/* 设置不可编辑 */
    connect(this,&TCP_ServerWidget::appLogMessage_signal,this,&TCP_ServerWidget::on_showLogMessage);
    connect(ui->logShowTextEdit,&QTextEdit::textChanged,this,[=]()
    {
        ui->logShowTextEdit->moveCursor(QTextCursor::End);
    });
    emit appLogMessage_signal("应用启动");

    getLocalNetInformation();

    ui->TCPServerPortLineEdit->setText("8000");
    ui->TCPServerCloseListenPushButton->setEnabled(false);/*未开始监听前，结束监听按键无效*/
    QFont font = QFont("Microsoft YaHei UI", 15);
    ui->TcpClientNumberLabel->setAlignment(Qt::AlignCenter);                 /*设置文本居中显示*/
    ui->TcpClientNumberLabel->setFont(font);
    ui->TcpClientNumberLabel->setText("已连接客户端数量：0");

    TaskOne = new QThread;                          /*创建子线程对象*/
    TaskOneWorker = new TCP_ServerWorker;           /*创建任务对象*/
    TaskOneWorker->moveToThread(TaskOne);           /*将任务对象移动到子线程中去*/

    /*注意信号的重复连接，Qt::UniqueConnection(保证信号只被连接一次)*/
    connect(this,&TCP_ServerWidget::StartListenTCPClientSignal,TaskOneWorker,&TCP_ServerWorker::StartListenTCPClient,Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    connect(TaskOneWorker,&TCP_ServerWorker::SocketInformationConnectSignal,this,&TCP_ServerWidget::UI_SocketInformationConnectShow);
    connect(TaskOneWorker,&TCP_ServerWorker::SocketInformationDisconnectSignal,this,&TCP_ServerWidget::UI_SocketInformationDisonnectShow);
    connect(TaskOneWorker,&TCP_ServerWorker::appLogMessage_signal,this,&TCP_ServerWidget::on_showLogMessage);
    connect(this,&TCP_ServerWidget::ProgramOverSignal,TaskOneWorker,&TCP_ServerWorker::ProgramOver);
    connect(this,&TCP_ServerWidget::ServerSendDataToClientSignal,TaskOneWorker,&TCP_ServerWorker::ServerSendDataToClient);

    connect(TaskOneWorker,&TCP_ServerWorker::pictureDataShow_signal, this ,&TCP_ServerWidget::pictureShowFromData);/*图片显示处理*/
    qDebug()<<"TCP_ServerWidget threadId: "<<QThread::currentThreadId();

}

TCP_ServerWidget::~TCP_ServerWidget()
{
    userSetting->setValue("userData/saveTcpIp",ui->TCPServerIPLineEdit->text());
    userSetting->setValue("userData/saveTcpPort",ui->TCPServerPortLineEdit->text());
    delete ui;
}

void TCP_ServerWidget::UI_SocketInformationConnectShow(QString TcpClientIP, QString TcpClientPort, unsigned int ClientNumber)
{
    ui->TcpClientNumberLabel->setText("已连接客户端数量：" + QString::number(ClientNumber));
    if(!TcpClientIP.isEmpty() || !TcpClientPort.isEmpty())
    {
        emit appLogMessage_signal("有新加入的客户端:" + TcpClientIP.toLocal8Bit() + "," + TcpClientPort.toLocal8Bit());
    }
}

void TCP_ServerWidget::UI_SocketInformationDisonnectShow(QString TcpClientIP, QString TcpClientPort, unsigned int ClientNumber)
{
    ui->TcpClientNumberLabel->setText("已连接客户端数量：" + QString::number(ClientNumber));
    emit appLogMessage_signal("有新断开的客户端:" + TcpClientIP.toLocal8Bit() + "," + TcpClientPort.toLocal8Bit());
}

void TCP_ServerWidget::pictureShowFromData(QByteArray pictureData)
{
    QImage* pictureImg = new QImage;
    bool isLoadPictureData = false;
    isLoadPictureData = pictureImg->loadFromData(pictureData);
    if(isLoadPictureData)
    {
//        ui->pictureLabel->setPixmap(QPixmap::fromImage(pictureImg->scaled(ui->pictureLabel->size(),
//                                                                          Qt::KeepAspectRatio,Qt::SmoothTransformation)));
        delete pictureImg;
    }
    else
    {
        delete pictureImg;
        emit appLogMessage_signal("图片可能损坏");
        return;
    }
}

void TCP_ServerWidget::on_showLogMessage(QByteArray logMessage)
{
    ui->logShowTextEdit->append(logMessage);
}

void TCP_ServerWidget::on_TCPServerListenPushButton_clicked()
{
    if((ui->TCPServerIPLineEdit->text() == "") || (ui->TCPServerPortLineEdit->text() == ""))
    {
        emit appLogMessage_signal("请检查f服务器地址或端口设置是否为空");
        return;
    }
    /*开始监听按钮的槽函数*/
    ui->TCPServerListenPushButton->setEnabled(false);/*监听按键无效*/
    ui->TCPServerCloseListenPushButton->setEnabled(true);/*结束监听按键有效*/
    unsigned short TCP_Port = ui->TCPServerPortLineEdit->text().toUShort(); /*获取用户输入的端口号*/
    QString TCP_IP = ui->TCPServerIPLineEdit->text();
    TaskOne->start();                               /*启动线程，让线程中的任务去连接TCP*/
    emit appLogMessage_signal("设置的服务器地址为:" + TCP_IP.toLocal8Bit());
    emit appLogMessage_signal("设置的服务器端口为:" + QByteArray::number(TCP_Port));
    emit appLogMessage_signal("已经启动服务器监听");
    emit StartListenTCPClientSignal(TCP_IP,TCP_Port);/*向子线程发送坚挺的端口号，启动监听*/
}


void TCP_ServerWidget::on_TCPServerCloseListenPushButton_clicked()
{
    emit ProgramOverSignal();/*向子线程发送结束监听的信号*/
    emit appLogMessage_signal("断开所有客户端的连接");
    ui->TCPServerListenPushButton->setEnabled(true);/*监听按键无效*/
    ui->TCPServerCloseListenPushButton->setEnabled(false);/*结束监听按键无效*/
}

void TCP_ServerWidget::getLocalNetInformation()
{
    QByteArray NetInformation("可用IPV4地址\r\n");
    char ipCount = 0;;
    QList<QHostAddress> list =QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
       if(address.protocol() ==QAbstractSocket::IPv4Protocol)
       {
           ipCount++;
           NetInformation.append("IP地址" + QByteArray::number(ipCount) + ":" + address.toString().toLocal8Bit() + "\r\n");
       }
    }
    emit appLogMessage_signal(NetInformation);
}

void TCP_ServerWidget::openEventSetting()
{
    /*读取配置文件*/

    userSetting = new QSettings("CCKJ_SERVER","userConfig");

    if(userSetting->value("userData/saveTcpIp").isNull())/*判断键值是否存在 */
    {
        emit appLogMessage_signal("默认服务器IP并未设置");
        userSetting->setValue("userData/saveTcpIp","127.0.0.1");
        return;
    }
    else
    {
        ui->TCPServerIPLineEdit->setText(userSetting->value("userData/saveTcpIp").toString());
    }

    if(userSetting->value("userData/saveTcpPort").isNull())/*判断键值是否存在 */
    {
        emit appLogMessage_signal("默认服务器PORT并未设置");
        userSetting->setValue("userData/saveTcpPort","8000");
    }
    else
    {
        ui->TCPServerPortLineEdit->setText(userSetting->value("userData/saveTcpPort").toString());
    }
}



