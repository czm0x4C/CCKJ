#ifndef WIDGET_H
#define WIDGET_H

#include "qsettings.h"
#include <QWidget>
#include <QTcpSocket>  /*添加TCP对象的头*/
#include <QHostAddress>
#include <QDebug>      /*添加Debug输出*/
#include <QMessageBox>
#include <QByteArray>
#include <QStringListModel>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "tcptask.h"
#include "serialport.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

/*
TCP 数据帧:
BYTE0-BYTE3(4个字节) :数据长度
BYTE4(一个字节) :帧的含义
                0x00 :TCP心跳包
                0x01 :
BYTE5-BYTEN :数据
*/


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void windowsInit();

    void openEventSetting();

    QByteArray uIntToQbyteArray(unsigned int uIntData);

    unsigned int qbyteArrayToUint(QByteArray dataBuffer);

    QByteArray setSerialPortStringDataFormat(unsigned char frameHead,unsigned char frameAddress,unsigned char frameID,QByteArray data);

    QByteArray setSerialPortUshortDataFormat(unsigned char frameHead,unsigned char frameAddress,unsigned char frameID,unsigned short data);

private slots:

    void on_ConnectTCPtButton_clicked();

    void on_CloseTCPButton_clicked();

    void on_searchPictureButton_clicked();

    void on_tcpConnectedSuccessfull();

    void on_errorOccurred();

    void on_showPicture(QByteArray picData);

    void on_listViewClicked(const QModelIndex &index);

    void on_takePictureButton_clicked();

    void on_setSavePathButton_clicked();

    void on_clearLocalCahePathButton_clicked();

    void on_clearServerCahePathButton_clicked();

    void on_takePicFinish();

    void on_showLogMessage(QByteArray logMessage);

    void on_devicePushButton_clicked();

    void on_openSerialPortState(bool state);

    void on_writeDevicePushButton_clicked();

    void on_takePictureError();

    void on_searchDeviceButton_clicked();

    void on_showDeviceId(QList<QByteArray> deviceIdList);

    void on_cameraBindOK();

    void on_cameraDisbindOK();

    void on_motoControlPushButton_clicked();

    void on_openMotoSuccess();

    void on_readDeviceInfoPushButton_clicked();

    void on_resetDevicePushButton_clicked();

    void on_addRecordTimePushButton_clicked();

    void on_deleteRecordTimePushButton_clicked();

    void on_tcpSetRecordPushButton_clicked();

    void on_tcpAddRecordTimePushButton_clicked();

    void on_tcpDeleteRecordTimePushButton_clicked();

    void on_tcpRecordCheckBox_clicked();

    void on_tcpScheduledTimeCheckBox_clicked();

    void on_recordCheckBox_clicked();

    void on_scheduledTimeCheckBox_clicked();

    void on_connectDeviceButton_clicked();

    void on_disConnectDeviceButton_clicked();

    void on_tcpScheduledTimeCheckBox_stateChanged(int arg1);

    void on_scheduledTimeCheckBox_stateChanged(int arg1);

    void on_tcpRecordCheckBox_stateChanged(int arg1);

    void on_recordCheckBox_stateChanged(int arg1);

    void on_downLoadPictureSuccess();
signals:

    void tcpConnectToHost_signal(QString tcpServerIp,QString tcpServerPort);

    void stopTcp_signal();

    void sendTcpData_signal(QByteArray data);

    void appLogMessage_signal(QByteArray logMessage);

    void openSerialPort_signal(QString PortName,int BaudRate,
                               QSerialPort::DataBits PortDataBits,
                               QSerialPort::Parity PortParityBits,
                               QSerialPort::StopBits PortStopBits,
                               QSerialPort::FlowControl PortFlowControl);

    void closeSerialPort_signal();

    void sendSerialPortData(QByteArray data);


protected:

    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

    virtual void resizeEvent(QResizeEvent *event) override;

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


    Ui::Widget *ui;

    QList<QWidget*> m_Widget;			    //存储所有的子控件

    QMap<QWidget*, QRect> m_WidgetRect;		//保存每个子控件的初始大小

    QByteArray UintToByteArray(unsigned int UintNumber);/*unsigned int转QBytearray*/

    tcpTask *mTcpTask;

    QThread *mTcpThread;

    QTcpSocket *TCP_Socket;/*创建网络对象*/

    QStringListModel *listmodel;

    QSettings *userSetting;

    QString savePicFilePath;

    QTimer *tcpHeartBeatTimer;

    QThread *serialPortThread;

    SerialPortThread *serialPortTask;

    QByteArray bindCameraDeviceID;          /* 保存当前绑定的设备ID */

    bool isBindFinish = false;              /* 保存当前的设备是否绑定 */

    bool isSerialPortOpen = false;          /* 判断串口是否打开 false：未打开 true：打开 */

    bool copyDirectoryFiles(const QString fromDir, const QString toDir, bool coverFileIfExist);

    bool isTcpThreadAlive = false;

    bool isTcpBackFinish = true;

    QByteArray chooseDate;                  /* 保存请求的下载的时间 */

    void SerialPortSearch();

    void comBoxClick();

    void sendExistPictureFileToServer();

    QByteArray packTcpDataFrame(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, QByteArray data); /* 给发送的TCP数据封帧 */

    bool checkTcpIsConnected();

    void hideTcpSetScheduledTimingGroup();

    void showTcpSetScheduledTimingGroup();

    void hideTcpSetRecordTimingGroup();

    void showTcpSetRecordTimingGroup();

    void hideSetScheduledTimingGroup();

    void showSetScheduledTimingGroup();

    void showSetRecordTimingGroup();

    void hideSetRecordTimingGroup();

};
#endif // WIDGET_H
