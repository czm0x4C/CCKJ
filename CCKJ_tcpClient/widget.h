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

    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result);

private:
    Ui::Widget *ui;
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

    bool copyDirectoryFiles(const QString fromDir, const QString toDir, bool coverFileIfExist);

    enum {HERAT_BEAT_PACK = 0x00,PICTURE_DATA,DOWNLOAD_PICTURE,EMPTY,
          PICTURE_TO_CLIENT_NAME,PICTURE_TO_CLIENT_DATA,PICTURE_TO_CLIENT_END,
         TAKE_PICTURE,SET_CAMERA_DEVICE_FLAG,CAMERA_TAKE_PICTURE,CAMERA_TAKE_PICTURE_DONE,
         CLEAR_SERVER_CACHE,CLEAR_SERVER_CACHE_DONE,
          CLIENT_PICTURE_FILE_NAME,
          PICTURE_ERROR};
    bool isTcpThreadAlive = false;

    bool isTcpBackFinish = true;

    void SerialPortSearch();

    void comBoxClick();

    void sendExistPictureFileToServer();
};
#endif // WIDGET_H
