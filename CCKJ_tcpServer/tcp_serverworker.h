#ifndef TCP_SERVERWORKER_H
#define TCP_SERVERWORKER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

class SocketInformationBind/*将socket的信息和他们的IP和Port绑定*/
{
public:
    QTcpSocket* socket;//socket指针
    QString ClientIP_String;//ip端口字符串
    QString ClientPort_String;//端口字符串
    QString targetIP_string;
    QString targetPort_string;
    bool cameraBindFlag;
    QString cameraBindID;
    unsigned int ClientLocalUserID;/*此socket的用户ID*/
    unsigned int ClientFarUserID;/*此socket的远程连接用户ID*/
    unsigned int clientState;/*此socket的远程连接用户ID*/
    bool cameraDevice;      /* 相机设备标识 */
    QByteArray cameraDeviceId;    /* 相机设备ID */
    bool pcClient;          /* PC客户端标识 */
    bool isActive;


    QByteArray recTcpData;  /* 保存接收对象的TCP数据 */
    unsigned char cmd;      /* 判断当前的数据帧的作用 */
    int tcpDataLen;         /* 记录数据长度 */
    bool recNextData;       /* 判断是都接收剩余的数据 */

    QList<QString> existFileNameList;

    QTimer *takePictureOutTimer;
    bool takePictureOutOfTime;

};

class TCP_ServerWorker : public QObject
{
    Q_OBJECT
public:
    explicit TCP_ServerWorker(QObject *parent = nullptr);

    void StartListenTCPClient(QString TcpServerIP, unsigned short TcpServerPort);/*开始监听客户端连接*/
    void NewTcpConnect();           /*处理连接客户端的槽函数*/
    void CilentDataRead();          /*处理客户端发来的数据*/
    void ClientDisconnected();      /*客户端断开连接处理槽函数*/
    void ProgramOver();             /*结束监听处理的槽函数*/
    void ServerSendDataToClient(unsigned int ClientID,QByteArray ServerData);  /*服务器向指定的客户端发送数据*/
    void checkTcpIsActive();

    QByteArray uIntToQbyteArray(unsigned int uIntData);

    unsigned int qbyteArrayToUint(QByteArray dataBuffer);

    QByteArray setCmdFrameFormat(unsigned int dataLen,unsigned char cmd);

    QByteArray setDataFrameFormat(unsigned int dataLen,unsigned char cmd,QByteArray data);

    void on_takePictureError();


signals:
    void SocketInformationConnectSignal(QString SocketIP,QString SocketPort,unsigned int ClientNumber);

    void SocketInformationDisconnectSignal(QString SocketIP,QString SocketPort, unsigned int ClientNumber);

    void ClientMessageSignal(QByteArray ClientMessage);

    void pictureDataShow_signal(QByteArray pictureData);

    void appLogMessage_signal(QByteArray logMessage);

private:
    QTcpServer *TCP_Server;

//    QTcpSocket *TCP_ServerSocket;

    QList<SocketInformationBind*> Tcp_ClientInformationList;

    bool isDisConnectedAll =false;

    bool isTakePictureError = false;

    QTimer *activeCheckTimer;

    QTimer *takePictureOutTimer;

    enum
    {
        HERAT_BEAT_PACK = 0x00,
        PICTURE_DATA,
        DOWNLOAD_PICTURE,
        EMPTY,
        PICTURE_TO_CLIENT_NAME,
        PICTURE_TO_CLIENT_DATA,
        PICTURE_TO_CLIENT_END,
        TAKE_PICTURE,
        SET_CAMERA_DEVICE_FLAG,
        SET_CAMERA_DEVICE_ID,
        CAMERA_TAKE_PICTURE,
        CAMERA_TAKE_PICTURE_DONE,
        CLEAR_SERVER_CACHE,
        CLEAR_SERVER_CACHE_DONE,
        CLIENT_PICTURE_FILE_NAME,
        PICTURE_ERROR,
        GET_ONLINE_DEVICE,
        SET_PC_DEVICE_FLAG,
        ONLINE_CAMERA_DEVICE_ID_TO_CLIENT,
        ONLINE_CAMERA_DEVICE_LIST_TO_CLIENT_END,
        CLIENT_BIND_CAMERA,
        CLIENT_DISBIND_CAMERA,
        CLIENT_BIND_CAMERA_SUCCESS,
        CLIENT_BIND_CAMERA_FAIL,
        OPEN_MOTO_CMD,          /* 打开水泵的命令 */
        OPEN_MOTO_SUCCESS_CMD,  /* 打开成功反馈 */
        OPEN_MOTO_FAIL_CMD,     /* 打开失败反馈 */
        SET_RECORD_TIME_CMD,    /* 设置定时时间 */
        SET_RECORD_TIME_DONE_CMD,/* 设置定时结束 */
        SET_SCHEDULED_TIME_CMD, /* 设置间隔定时时间 */
        SET_RECORD_TIME_SUCCESS_CMD, /* 设置定时时间成功反馈 */
        SET_LIED_BRIGHTNESS_CMD,
        SET_TAKE_PICTURE_DELAY_TIME_CMD
    };
};

#endif // TCP_SERVERWORKER_H
