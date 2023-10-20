#include "serialport.h"
#include <QDebug>


SerialPortThread::SerialPortThread(QObject *parent)
    :QObject(parent)
{

}

void SerialPortThread::OpenSerialPort(QString PortName, int BaudRate, QSerialPort::DataBits PortDataBits, QSerialPort::Parity PortParityBits, QSerialPort::StopBits PortStopBits, QSerialPort::FlowControl PortFlowControl)
{
    qDebug()<<"SerialPortThread threadId: "<<QThread::currentThreadId();
    SerialPort = new QSerialPort();                                         /*创建串口对象*/

    SerialPort->setPortName(PortName);                                      /*设置选中的COM口*/
    SerialPort->setBaudRate(BaudRate);                                      /*设置串口的波特率*/
    SerialPort->setDataBits(PortDataBits);                                  /*设置数据位数*/
    SerialPort->setParity(PortParityBits);                                  /*设置奇偶校验,NoParit无校验*/
    SerialPort->setStopBits(PortStopBits);                                  /*设置停止位，OneStop一个停止位*/
    SerialPort->setFlowControl(PortFlowControl);                            /*设置流控制，NoFlowControl无流控制*/

    if(SerialPort->open(QIODevice::ReadWrite) == true)                      /*ReadWrite设置的是可读可写的属性*/
    {

        SerialPortIsOK = true;
        emit OpenSerialPortOK_signal(true);                                 /*发送串口打开成功信号*/
    }
    else
    {
        SerialPortIsOK = false;
        emit OpenSerialPortOK_signal(false);                                /*发送串口打开失败信号*/
    }

    connect(SerialPort,&QSerialPort::readyRead,this,&SerialPortThread::DataRead);               /*关联串口数据接收*/

    bufferObj = new ringbufer;
    serialPortBuffer = new RingBuff_t;
    bufferObj->RingBuffer_Init(serialPortBuffer);

}

void SerialPortThread::SerialPortClose()
{
    if(SerialPort->isOpen() == true)
    {
        SerialPort->close();        /*关闭串口*/
        SerialPort->deleteLater();
    }
    qDebug("关闭串口设备");
}
/* 读取串口数据 */
void SerialPortThread::DataRead()
{
    QByteArray receiveData = SerialPort->readAll();
    unsigned int writeDataLen = 0;
    bufferObj->WriteBytes(serialPortBuffer,receiveData,&writeDataLen);
    if(writeDataLen != receiveData.size())
    {
        qDebug("1 写入数据不完整");
    }
    QByteArray bufferData;
    unsigned int nowBufferSize = bufferObj->ValidDataLen(serialPortBuffer);
    bufferObj->ReadBytes(serialPortBuffer,&bufferData,nowBufferSize);/*读取缓冲区中的所有数据*/
    unsigned int startFramePlace = 0;
    unsigned int endFramePlace = 0;
    for(unsigned int i=0;i<nowBufferSize;i++)
    {
        if((unsigned char)bufferData.at(i) == 0xAA)
        {
            startFramePlace = i;
            if((nowBufferSize - i) > 3)
            {
                if((nowBufferSize - i) > i + 3 + bufferData.at(i+3) + 2)
                {
                    endFramePlace = i + 4 + bufferData.at(i+3) + 2;
                    QByteArray frameData = bufferData.mid(startFramePlace,endFramePlace - startFramePlace);
                    if(FrameDataCheck(frameData) == true)/*检验帧数据是否正确*/
                    {
                        /***********************得到下位机的完整数据**********************************/
                        qDebug() << frameData;
                        if((unsigned char)frameData.at(1) == MCU)
                        {
                            switch((unsigned char)frameData.at(2))
                            {
                                case CMD_LOG_MESSAGE:
                                {
                                    emit appLogMessage_signal(frameData.mid(4,frameData.at(3)));
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                        /***********************帧数据处理结束**********************************/
                        QByteArray leaveData = bufferData.mid(endFramePlace,nowBufferSize - endFramePlace);
                        bufferObj->WriteBytes(serialPortBuffer,leaveData,&writeDataLen);
                        if(leaveData.size() == 0)break;
                        if(writeDataLen != leaveData.size())
                        {
                            qDebug("2 写入数据不完整");
                        }
                        break;/*结束此次的读取*/
                    }
                }
                else
                {
                    bufferObj->WriteBytes(serialPortBuffer,bufferData,&writeDataLen);
                    if(writeDataLen != bufferData.size())
                    {
                        qDebug("3 写入数据不完整");
                    }
                    break;
                }
            }
            else
            {
                bufferObj->WriteBytes(serialPortBuffer,bufferData,&writeDataLen);
                if(writeDataLen != bufferData.size())
                {
                    qDebug("4 写入数据不完整");
                }
                break;
            }
        }
    }

}
int SerialPortThread::QByteToUint(QByteArray Data)
{
    int IntTemp = 0;
    IntTemp  = (Data[0] & 0x00FF);
    IntTemp |= ((Data[1]<<8 ) & 0xFF00);
    IntTemp |= ((Data[2]<<16 ) & 0xFF0000);
    IntTemp |= ((Data[3]<<24 ) & 0xFF000000);

    return IntTemp;
}
void SerialPortThread::DataSend(QByteArray SendDataArray)
{
    if(SerialPortIsOK == true)
    {
       SerialPort->write(SendDataArray);
    }
}

bool SerialPortThread::SerialPortState()
{
    return SerialPortIsOK;
}

void SerialPortThread::SerialPortSetState(bool State)
{
    SerialPortIsOK = State;
}

bool SerialPortThread::FrameDataCheck(QByteArray Frame)
{
    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;

    //qDebug() << Frame;
    for(unsigned char i=0; i < Frame[3] + 4 ; i++)
    {
        sumcheck += Frame[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    unsigned char Len = Frame[3];
//    qDebug() << "计算的sumcheck = " << sumcheck;
//    qDebug() << "计算的addcheck = " << addcheck;
//    qDebug() << "帧中sumcheck = " << (unsigned char)Frame.at(Len + 4);
//    qDebug() << "帧中addcheck = " << (unsigned char)Frame.at(Len + 5);

    if(sumcheck == (unsigned char)Frame.at(Len + 4) && addcheck == (unsigned char)Frame.at(Len + 5))
    {
        return true; //校验通过
    }
    else
    {
        emit appLogMessage_signal("接收数据帧校验失败");
        return false; //校验失败
    }
}


