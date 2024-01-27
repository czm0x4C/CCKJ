#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QThread>
#include <QTimer>

class myTcpSocket : public QObject
{
    Q_OBJECT
public:
    explicit myTcpSocket(QObject *parent = nullptr);
    ~myTcpSocket();

    bool deleteFlag = false;                            /* socket清除标志 */

    uint8_t deviceLabel;                                /* 保存设备角色标签 PC IOT_CAMERA */

    QByteArray cameraId;                                /* 保存相机的ID */

    QByteArray bindCameraID;                            /* 保存绑定的摄像头ID */

    QHostAddress returnIp();

    uint16_t returnPort();

    void setSocketDescriptor(qintptr socketDescriptor); /* 设置socket描述符 */

    void createTcpSocket();                             /* 创建socket对象 */

    void setTcpSocketList(QList<myTcpSocket *> *p);
private slots:
    void on_checkTcpHeartBeat();                        /* tcp心跳检查槽函数 */

    void on_errorOccurred(QAbstractSocket::SocketError socketError);
private:
    enum
    {
        /* 角色 */
        DEVICE_LABEL_PC = 0x00,             /* 设备标签PC端 */
        DEVICE_LABEL_CAMERA,                /* 设备标签CAMERA端 */
        DEVICE_LABEL_SERVER,                /* 设备标签SERVER端 */
        /* target */
        PC_TO_SERVER,                       /* PC发SERVER */
        PC_TO_CAMER,                        /* PC发CAMERA */
        CAMER_TO_SERVER,                    /* CAMERA发SERVER */
        CAMER_TO_PC,                        /* CAMERA发PC */
        SERVER_TO_CLIENT,                   /* SERVER发CLIENT */
        /* cmd */
        HERAT_BEAT_PACK,                    /* 心跳包 */
        SET_PC_DEVICE_FLAG,                 /* 设置PC标识 */
        SET_CAMERA_DEVICE_FLAG,             /* 设置CAMERA标识 */
        SET_CAMERA_DEVICE_ID,               /* 设置相机的ID */
        GET_ONLINE_CAMERA_DEVICE_ID,        /* 请求所有连接的camera设备ID */
        TAKE_PICTURE,                       /* 拍照命令 */
        JPEG_DATA_PACK_SIZE,                /* JPEG数据包 */
        TAKE_PICTURE_END,                   /* 拍照结束 */
        TAKE_PICTURE_ERROR,                 /* 拍照错误 */
        CAMERA_ID,                          /* 相机ID */
        SEND_CAMERA_ID_END,                 /* 发送相机ID结束 */
        CLIENT_BIND_CAMERA,                 /* 客户端绑定相机 */
        CLIENT_BIND_CAMERA_OK,              /* 客户端绑定相机成功 */
        CLIENT_DISBIND_CAMERA,              /* 客户端解绑摄像头 */
        CLIENT_DISBIND_CAMERA_OK,           /* 客户端解绑摄像头成功 */
        CLIENT_EXIST_PICTURE_FILE_NAME,     /* 客户端存在的图片文件名称 */
        CLIENT_EXIST_PICTURE_FILE_NAME_END, /* 客户端存在的图片文件名称发送结束 */
        DOWNLOAD_PICTURE,                   /* 客户端请求下载图片数据 */
        DOWNLOAD_PICTURE_END,               /* 客户端请求下载图片数据结束 */
        DOWNLOAD_PICTURE_EMPTY,             /* 客户端请求下载图片的数据为空 */
        PICTURE_TO_PC_NAME,                 /* 将图片名字发送到PC端 */
        PICTURE_TO_PC_DATA,                 /* 将图像数据发送PC端 */
        SET_RECORD_TIME,                    /* 设置定时时间 */
        SET_RECORD_TIME_END,                /* 设置定时结束 */
        SET_RECORD_TIME_SUCCESS,            /* 设置定时时间成功反馈 */
        SET_SCHEDULED_TIME,                 /* 设置间隔定时时间 */
        SET_SCHEDULED_TIME_CMD_SUCCESS,     /* 设置间隔定时时间成功反馈 */
        SAVE_PC_PICTURE,                    /* 保存由PC端发来的图片 */
        SET_TEMP_DEVICE_ID,                 /* 未绑定时，临时设置绑定ID */
        CLEAR_SERVER_CACHE,                 /* 清除服务器上的图片缓存 */
        CLEAR_SERVER_CACHE_DONE,            /* 缓存清除完毕 */
        OPEN_MOTO_CMD,                      /* 打开水泵的命令 */
        OPEN_MOTO_SUCCESS_CMD,              /* 打开成功反馈 */
        OPEN_MOTO_FAIL_CMD,                 /* 打开失败反馈 */
        PICTURE_DATA_PACK,                  /* 图像数据包 */
        TAKE_RGB_PICTURE_END,               /* 拍摄RGB图片结束 */
        CAMERA_TRY_SERVER_CHECK,            /* 服务器检测 */
        SERVER_CHECK_BACK,                  /* 服务器检测响应 */
        SET_LIED_BRIGHTNESS_CMD,
        SET_TAKE_PICTURE_DELAY_TIME_CMD
    };

    qintptr mSocketDescriptor;

    QTcpSocket *tcpSocket;

    bool tcpConnectIsAlive = false;                     /* 判断连接是否正常 */

    QTimer *tcpHeartBeatCheckTimer;                     /* tcp心跳连接检测 */

    QHostAddress mTcpClientIp;                          /* tcp socket的IP */

    uint16_t mTcpClientPort;                            /* tcp socket的PORT */

    QList<myTcpSocket *> *myTcpSocketList;              /*  */

    QByteArray receiveData;

    QList<QByteArray> tempExistFileNameList;            /* 临时保存客户端发送来的已经存在的图片文件 */

    QList<QByteArray> existFileNameList;                /* 保存客户端发送来的已经存在的图片文件 */

    QByteArray tempPicData;

    uint32_t tempPicDataLen = 0;        /* 临时存放图片的大小 */

    uint32_t getJpegDataPackSize = 0;   /* 接收的JPEG图片大小 */

    void readTcpData();

    void sendTcpData(QByteArray tcpData);

    void tcpDisconnect();

    void frameDataAnalysis(QByteArray data);            /* 解析TCP数据帧 */

    void pcToServerDeal(QByteArray data);

    void pcToCameraDeal(QByteArray data);

    void cameraToServerDeal(QByteArray data);

    void cameraToPcDeal(QByteArray data);

    QByteArray packTcpDataFrame(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, QByteArray data);/* 给发送的TCP数据封帧 */
signals:

    void appLogMessage_signal(QByteArray logMessage);   /* UI界面显示的log信息 */

    void UI_SocketInformationConnectShow_signal(QHostAddress ip,uint16_t port);/* 发送连接的tcp客户端信息 */

    void sendBindTcpMessage(QByteArray data);


};



class myTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit myTcpServer(QHostAddress tcpIp,uint16_t tcpPort,QObject *parent = nullptr);
    ~myTcpServer();

    QList<myTcpSocket *>myTcpSocketList;

    QList<QThread *>mySocketThread;/* 保存socket线程 */

    QList<myTcpSocket *> returnQlist();
protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void appLogMessage_signal(QByteArray logMessage);/* UI界面显示的log信息 */

    void UI_SocketInformationConnectShow_signal(QHostAddress ip,uint16_t port);/* 发送连接的tcp客户端信息 */
    void UI_SocketInformationDisonnectShow_signal(QHostAddress ip,uint16_t port);/* 发送断开连接的tcp客户端信息 */
private:


};
#endif // MYTCPSOCKET_H
