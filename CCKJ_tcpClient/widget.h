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

    void windowSetEnable();

    void windowSetDisable();

    void openEventSetting();

    bool checkTcpServerIsOK();

    QByteArray uIntToQbyteArray(unsigned int uIntData);

    unsigned int qbyteArrayToUint(QByteArray dataBuffer);

    QByteArray setCmdFrameFormat(unsigned int dataLen,unsigned char cmd);

    QByteArray setDataFrameFormat(unsigned int dataLen,unsigned char cmd,QByteArray data);

    QByteArray setUshortFrameFormat(unsigned int dataLen,unsigned char cmd,unsigned short data);

    QByteArray setSerialPortStringDataFormat(unsigned char frameHead,unsigned char frameAddress,unsigned char frameID,QByteArray data);
    QByteArray setSerialPortUshortDataFormat(unsigned char frameHead,unsigned char frameAddress,unsigned char frameID,unsigned short data);

private slots:

    void on_ConnectTCPtButton_clicked();

    void on_CloseTCPButton_clicked();

    void on_searchPictureButton_clicked();

    void on_tcpConnectedSuccessfull();

    void on_tcpDisconnected();

    void on_showPicture(QByteArray picData);

    void on_listViewClicked(const QModelIndex &index);

    void on_takePictureButton_clicked();

    void on_readPictureDownLoadState(tcpTask::picDownloadState state);

    void on_setSavePathButton_clicked();

    void on_keepTcpHeartBeat();

    void on_clearLocalCahePathButton_clicked();

    void on_clearServerCahePathButton_clicked();

    void on_takePicFinish();

    void on_tcpServerCacheClearDone();

    void on_showLogMessage(QByteArray logMessage);

    void on_devicePushButton_clicked();

    void on_openSerialPortState(bool state);

    void on_writeDevicePushButton_clicked();

    void on_takePictureError();

    void on_searchDeviceButton_clicked();

    void on_showDeviceId(QList<QByteArray> deviceIdList);

    void on_cameraBindOK();

    void on_cameraBindFail();

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
    Ui::Widget *ui;

    QList<QWidget*> m_Widget;			    //存储所有的子控件

    QMap<QWidget*, QRect> m_WidgetRect;		//保存每个子控件的初始大小

    QList<QWidget*> m1_Widget;			    //存储所有的子控件

    QMap<QWidget*, QRect> m1_WidgetRect;		//保存每个子控件的初始大小


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

    bool isTcpOpen = false;

    QByteArray bindCameraDevice;

    bool isBindFinish = false;

    bool copyDirectoryFiles(const QString fromDir, const QString toDir, bool coverFileIfExist);

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
        SET_LIED_BRIGHTNESS_CMD,        /* 设置闪光灯的亮度 */
        SET_TAKE_PICTURE_DELAY_TIME_CMD /* 设置拍照延时 */
    };

    bool isTcpThreadAlive = false;

    bool isTcpBackFinish = true;

    void SerialPortSearch();

    void comBoxClick();

    void sendExistPictureFileToServer();
};
#endif // WIDGET_H
