#ifndef TCPTASK_H
#define TCPTASK_H

#include <QObject>
#include <QTcpSocket>
#include <QApplication>



class tcpTask : public QObject
{
    Q_OBJECT
public:
    tcpTask();

    void startTcpConnect(QString tcpServerIp,QString tcpServerPort);

    void sendTcpData(QByteArray data);

    void setSaveDateFileName(QString date);

    QByteArray uIntToQbyteArray(unsigned int uIntData);

    unsigned int qbyteArrayToUint(QByteArray dataBuffer);

    void setBindCameraDevice(QByteArray cameraID);

    enum picDownloadState{OK = 0,FAIL,PICTURE_EMPTY};

signals:
    void tcpConnected_signal();

    void tcpDisconnected_signal();

    void pictureData_signal(QByteArray data);

    void pictureDownload_signal(picDownloadState state);

    void takePictureDone_signal();

    void tcpServerCacheClearDone_signal();

    void appLogMessage_signal(QByteArray logMessage);

    void pictureError_signal();

    void onlineDeviceName_singal(QList<QByteArray> deviceIdList);

    void cameraBindOK_signal();

    void cameraBindFail_signal();

private slots:

    void readData();


private:
    QTcpSocket *TCP_Socket;/*创建网络对象*/

    QString saveDateFileName;/* 用户选择要保存的日期，以其命名 */

    enum {HERAT_BEAT_PACK = 0x00,PICTURE_DATA,DOWNLOAD_PICTURE,EMPTY,
           PICTURE_TO_CLIENT_NAME,PICTURE_TO_CLIENT_DATA,PICTURE_TO_CLIENT_END,
           TAKE_PICTURE,SET_CAMERA_DEVICE_FLAG,SET_CAMERA_DEVICE_ID,CAMERA_TAKE_PICTURE,CAMERA_TAKE_PICTURE_DONE,
           CLEAR_SERVER_CACHE,CLEAR_SERVER_CACHE_DONE,
           CLIENT_PICTURE_FILE_NAME,
           PICTURE_ERROR,
           GET_ONLINE_DEVICE,SET_PC_DEVICE_FLAG,
           ONLINE_CAMERA_DEVICE_ID_TO_CLIENT,ONLINE_CAMERA_DEVICE_LIST_TO_CLIENT_END,
           CLIENT_BIND_CAMERA,CLIENT_DISBIND_CAMERA,CLIENT_BIND_CAMERA_SUCCESS,CLIENT_BIND_CAMERA_FAIL};

    bool isCreated = false;

    QByteArray bindCameraDevice;
};

#endif // TCPTASK_H
