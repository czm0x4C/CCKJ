#ifndef TCPTASK_H
#define TCPTASK_H

#include <QObject>
#include <QTcpSocket>
#include <QApplication>
#include <QTimer>


class tcpTask : public QObject
{
    Q_OBJECT
public:
    tcpTask();
    ~tcpTask();

    void startTcpConnect();

    void sendTcpData(QByteArray data);

    void setSaveDateFileName(QString date);

    QByteArray uIntToQbyteArray(unsigned int uIntData);

    unsigned int qbyteArrayToUint(QByteArray dataBuffer);

    void setBindCameraDevice(QByteArray cameraID);

    enum picDownloadState{OK = 0,FAIL,PICTURE_EMPTY};

    QByteArray setCmdFrameFormat(unsigned int dataLen,unsigned char cmd);

    void setTcpserverIpPort(QHostAddress ip,uint16_t port);

signals:
    void errorOccurred_signal();

    void tcpConnected_signal();

    void pictureData_signal(QByteArray data);

    void pictureDownload_signal(picDownloadState state);

    void takePictureDone_signal();

    void tcpServerCacheClearDone_signal();

    void appLogMessage_signal(QByteArray logMessage);

    void pictureError_signal();

    void onlineDeviceName_singal(QList<QByteArray> deviceIdList);

    void cameraBindOK_signal();     /* 摄像头绑定成功信号 */

    void cameraBindFail_signal();   /* 摄像头绑定失败信号 */

    void cameraDisbindOK_signal();   /* 摄像头解除绑定成功信号 */

    void cameraOpenMotoSuccess_signal();

    void downLoadPictureSuccess_signal();  /* 下载图片数据成功 */

private slots:

    void readData();

    void on_keepTcpHeartBeat();

    void on_tcpDisconnected();

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
        PICTURE_DATA_PACK_INDEX,			/* 图像数据包编号 */
        TAKE_RGB_PICTURE_END,               /* 拍摄RGB图片结束 */
        SET_LIED_BRIGHTNESS_CMD,
        SET_TAKE_PICTURE_DELAY_TIME_CMD
    };


    QHostAddress mTcpServerIp;          /* 连接的tcp服务器ip */

    uint16_t mTcpServerPort;            /* 连接服务器的port */

    QTcpSocket *TCP_Socket;             /*创建网络对象*/

    QString saveDateFileName;           /* 用户选择要保存的日期，以其命名 */

    QByteArray bindCameraDevice;

    QTimer *tcpHeartBeatTimer;

    QList<QByteArray> deviceIdList;     /* 存放请求到的摄像头ID */

    QByteArray tempSavePicTureName;     /* 临时存放发送来的图片名称 */

    QByteArray tempPicData;

    uint32_t tempPicDataLen = 0;        /* 临时存放图片的大小 */

    QByteArray packTcpDataFrame(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, QByteArray data);      /* 给要发送的tcp数据封帧 */

    void frameDataAnalysis(QByteArray data);            /* 解析TCP数据帧 */

    void serverToClient(QByteArray data);

    void cameraToPcDeal(QByteArray data);

};

#endif // TCPTASK_H
