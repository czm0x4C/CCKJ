#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "ringbufer.h"
#include <QObject>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>


#include <QWidget>
#include <windows.h>
#include <QAbstractNativeEventFilter>
#include <dbt.h>

class SerialPortThread : public QObject
{
    Q_OBJECT
public:

    enum framesize{
        FRAMESIZE_96X96,    // 96x96
        FRAMESIZE_QQVGA,    // 160x120
        FRAMESIZE_QCIF,     // 176x144
        FRAMESIZE_HQVGA,    // 240x176
        FRAMESIZE_240X240,  // 240x240
        FRAMESIZE_QVGA,     // 320x240
        FRAMESIZE_CIF,      // 400x296
        FRAMESIZE_HVGA,     // 480x320
        FRAMESIZE_VGA,      // 640x480
        FRAMESIZE_SVGA,     // 800x600
        FRAMESIZE_XGA,      // 1024x768
        FRAMESIZE_HD,       // 1280x720
        FRAMESIZE_SXGA,     // 1280x1024
        FRAMESIZE_UXGA,     // 1600x1200
        // 3MP Sensors
        FRAMESIZE_FHD,      // 1920x1080
        FRAMESIZE_P_HD,     //  720x1280
        FRAMESIZE_P_3MP,    //  864x1536
        FRAMESIZE_QXGA,     // 2048x1536
        // 5MP Sensors
        FRAMESIZE_QHD,      // 2560x1440
        FRAMESIZE_WQXGA,    // 2560x1600
        FRAMESIZE_P_FHD,    // 1080x1920
        FRAMESIZE_QSXGA,    // 2560x1920
        FRAMESIZE_INVALID
    } ;

    enum frameAddress
    {
                    PC = 0xA5,
                    MCU = 0xA6
    };

    enum frameCmd{
                    CDM_WIFI_NAME = 0,/* 帧携带的数据为WIFI的名称 */
                    CMD_WIFI_PASSWORD,
                    CMD_SERVER_IP,
                    CMD_SERVER_PORT,
                    CMD_PICTURE_SIZE,
                    CMD_PICTURE_QUALITY,
                    CMD_SET_PARA_END,
                    CMD_LOG_MESSAGE,
                    CMD_DEVICE_ID,
                    CMD_SHOW_DEVICE_INFO,
                    CMD_RESET_DEVICE,
                    CMD_SET_RECORD_TIME,
                    CMD_SET_DELAY_TIME,
                    CMD_SET_LIED_BRIGHTNESS,
                    CMD_SET_TAKE_PICTURE_DELAY_TIME
    };

    QSerialPort *SerialPort;

    explicit SerialPortThread(QObject *parent = nullptr);

    void OpenSerialPort( QString PortName,int BaudRate,
                           QSerialPort::DataBits PortDataBits,
                           QSerialPort::Parity PortParityBits,
                           QSerialPort::StopBits PortStopBits,
                           QSerialPort::FlowControl PortFlowControl);

    void SerialPortClose();                         /*串口关闭*/

    void DataRead();         /*串口接收数据处理*/

    void DataSend(QByteArray SendDataArray);            /*串口发送数据处理*/

    bool SerialPortState();

    void SerialPortSetState(bool State);

signals:

    void OpenSerialPortOK_signal(bool IsOpen);

    void ReadData_signal(QByteArray ReadData);

    void getData_signal(signed int temperature,signed int pressure,signed int filterValue);

    void appLogMessage_signal(QByteArray logMessage);

private:

    bool SerialPortIsOK = false;

    bool FrameDataCheck(QByteArray Frame);

    int QByteToUint(QByteArray Data);

    RingBuff_t *serialPortBuffer;

    ringbufer *bufferObj;


};



#endif // SERIALPORT_H
