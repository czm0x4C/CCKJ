#include "tcptask.h"
#include "qthread.h"

#include <QBuffer>
#include <QDir>
#include <QImage>

tcpTask::tcpTask()
{

}

tcpTask::~tcpTask()
{
    TCP_Socket->disconnectFromHost();
    TCP_Socket->close();
    TCP_Socket->deleteLater();

    tcpHeartBeatTimer->deleteLater();

    qDebug() << "~tcpTask";
}

void tcpTask::startTcpConnect()
{
    TCP_Socket = new QTcpSocket(this);                              /*创建Socket对象*/
    TCP_Socket->connectToHost(mTcpServerIp,mTcpServerPort);         /*连接服务器*/

    /*tcp连接错误信号*/
    connect(TCP_Socket,&QTcpSocket::errorOccurred,this,[=]()
    {
        QThread::currentThread()->quit();                           /* 请求退出线程 */
        emit errorOccurred_signal();
    });
    /*连接成功TCP_Socket发出信号*/
    connect(TCP_Socket,&QTcpSocket::connected,this,[=]()
    {
        /* 设置tcp心跳包定时器 */
        tcpHeartBeatTimer = new QTimer();
        tcpHeartBeatTimer->start(1000);
        connect(tcpHeartBeatTimer,&QTimer::timeout,this,&tcpTask::on_keepTcpHeartBeat,Qt::UniqueConnection);

        TCP_Socket->write(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,SET_PC_DEVICE_FLAG,0,0));/* 发送一帧数据，告诉服务器为PC端 */
        emit tcpConnected_signal();
    });
    /*断开连接TCP_Socket发出信号*/
    connect(TCP_Socket,&QTcpSocket::disconnected,this,&tcpTask::on_tcpDisconnected);
    /*客户端接收到服务器发送的信息*/
    connect(TCP_Socket,&QTcpSocket::readyRead,this,&tcpTask::readData);

    qDebug()<<"tcpTask threadId: "<<QThread::currentThreadId();
}

void tcpTask::sendTcpData(QByteArray data)
{
    TCP_Socket->write(data);
}

void tcpTask::setSaveDateFileName(QString date)
{
    saveDateFileName = date;
}
/* unsigned int 转 QByteArray */
QByteArray tcpTask::uIntToQbyteArray(unsigned int uIntData)
{
    QByteArray dataBuffer;
    dataBuffer.resize(4);
    for(unsigned char i=0;i<4;i++)
    {
        dataBuffer[i] = (uIntData>>i*8);
    }
    return dataBuffer;
}
/* QByteArray 转 unsigned int */
unsigned int tcpTask::qbyteArrayToUint(QByteArray dataBuffer)
{
    unsigned int value = 0;
    for(unsigned char i=0;i<4;i++)
    {
        value |= ((unsigned char)dataBuffer.at(i)<< (i*8));
    }
    return value;
}

QByteArray tcpTask::setCmdFrameFormat(unsigned int dataLen,unsigned char cmd)
{
    QByteArray tempData;
    tempData = uIntToQbyteArray(dataLen);/* 填充后面的数据长度 */
    tempData.append(cmd);/* cmd */
    return tempData;
}

void tcpTask::setTcpserverIpPort(QHostAddress ip, uint16_t port)
{
    mTcpServerIp = ip;
    mTcpServerPort = port;
}

void tcpTask::setBindCameraDevice(QByteArray cameraID)
{
    bindCameraDevice = cameraID;
}

void tcpTask::readData()
{
    static QByteArray receiveData;
    receiveData += TCP_Socket->readAll();
    for(uint32_t i=0;i<receiveData.size();)
    {
        if(0xAA == (uint8_t)receiveData.at(i) && (receiveData.size() - i >= 8))         /* 确认数据帧头同时能得到数据长度 */
        {
            uint32_t dataLen = (uint8_t)receiveData.at(i+4) | (uint8_t)receiveData.at(i+5)<<8 | (uint8_t)receiveData.at(i+6)<<16 | (uint8_t)receiveData.at(i+7)<<24;
            if(receiveData.size() - i >= 8+dataLen+1)                                   /* 能都得到整个帧的数据长度 */
            {
                if(0xBB == (uint8_t)receiveData.at(i+8+dataLen))                        /* 确认数据帧尾 */
                {
                    frameDataAnalysis(receiveData.mid(i,9+dataLen));                    /* 分析帧的数据 */
                    receiveData = receiveData.mid(i+9+dataLen,receiveData.size() - i - 9 - dataLen);
                    i = 0;
                    if(0 == receiveData.size())break;
                    continue;
                }
            }
        }
        i++;
    }
}
/* 发送tcp心跳包槽函数 */
void tcpTask::on_keepTcpHeartBeat()
{
    TCP_Socket->write(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,HERAT_BEAT_PACK,0,0));
}

void tcpTask::on_tcpDisconnected()
{
    qDebug() << "tcp断开连接";
    QThread::currentThread()->quit();                           /* 请求退出线程 */
}
/* 给要发送的tcp数据封帧 */
QByteArray tcpTask::packTcpDataFrame(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, QByteArray data)
{
    /* 0xAA byte1 byte2 byte3  byte4 byte5 byte6 byte7   byteN   0xBB */
    /* 帧头  角色   目标   作用       长度（四字节）          数据载荷   帧尾 */
    QByteArray tempData;
    uint32_t cnt = 0;
    tempData.resize(4 + 4);
    tempData[cnt++] = 0xAA;                 /* 帧头 */
    tempData[cnt++] = charactar;            /* 角色 */
    tempData[cnt++] = target;               /* 目标 */
    tempData[cnt++] = cmd;                  /* 作用 */
    tempData[cnt++] = dataLen;              /* 长度 */
    tempData[cnt++] = dataLen>>8;
    tempData[cnt++] = dataLen>>16;
    tempData[cnt++] = dataLen>>24;
    tempData.append(data);
    tempData.append((char)0xBB);
    return tempData;
}

void tcpTask::frameDataAnalysis(QByteArray data)
{
    switch(data.at(2))
    {
        case SERVER_TO_CLIENT:                      /* 服务器到pc */
        {
            serverToClient(data);
            break;
        }
        case CAMER_TO_PC:                           /* camera到pc */
        {
            cameraToPcDeal(data);
            break;
        }
        default:
            break;
    }
}

void tcpTask::serverToClient(QByteArray data)
{
    switch(data.at(3))
    {
        case CAMERA_ID:                     /* 摄像头ID */
        {
            qDebug()<<"收到一个在线的摄像头ID";
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            QByteArray cameraId = data.mid(8,dataLen);
            deviceIdList.append(cameraId);
            break;
        }
        case SEND_CAMERA_ID_END:            /* 摄像头ID接收完毕 */
        {
            qDebug()<<"接收到所有的摄像头ID";
            emit onlineDeviceName_singal(deviceIdList);
            deviceIdList.clear();
            break;
        }
        case CLIENT_BIND_CAMERA_OK:         /* 绑定成功 */
        {
            emit cameraBindOK_signal();
            break;
        }
        case CLIENT_DISBIND_CAMERA_OK:      /* 解除绑定成功 */
        {
            emit cameraDisbindOK_signal();
            break;
        }
        case DOWNLOAD_PICTURE_END:          /* 请求的图片数据下载完毕 */
        {
            emit downLoadPictureSuccess_signal();
            emit appLogMessage_signal("图片下载完成!");
            break;
        }
        case DOWNLOAD_PICTURE_EMPTY:        /* 请求的图片数据为空 */
        {
            emit appLogMessage_signal("服务器中未找到数据!");
            break;
        }
        case PICTURE_TO_PC_NAME:            /* 请求的图片的名字 */
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            tempSavePicTureName = data.mid(8,dataLen);
            break;
        }
        case PICTURE_TO_PC_DATA:            /* 请求的图片的数据 */
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            /* 保存图像 */
            if(!saveDateFileName.isEmpty())
            {
                QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片";
                QDir *videoDir;
                videoDir = new QDir(picPath);
                if(!videoDir->exists()) videoDir->mkdir(picPath);/* 不存在就创建 */
                delete videoDir;
                picPath += "/" + bindCameraDevice;
                videoDir = new QDir(picPath);
                if(!videoDir->exists()) videoDir->mkdir(picPath);/* 不存在就创建 */
                delete videoDir;

                picPath += "/" + saveDateFileName;
                videoDir = new QDir(picPath);
                if(!videoDir->exists()) videoDir->mkdir(picPath);/* 不存在就创建 */
                delete videoDir;
                videoDir = NULL;

                picPath += "/" + tempSavePicTureName;
                QFile *RecFile = new QFile(picPath);
                RecFile->open(QFile::WriteOnly);
                RecFile->write(data.mid(8,dataLen));
                RecFile->close();
                delete RecFile;
                RecFile = NULL;
            }
            break;
        }
        case CLEAR_SERVER_CACHE_DONE:
        {
            emit appLogMessage_signal("服务器所有图片数据已删除!");
            break;
        }
        default:
            break;
    }
}

void tcpTask::cameraToPcDeal(QByteArray data)
{
    qDebug() << "接收的data size = " << data.size();
    switch((uint8_t)data.at(3))
    {
        case PICTURE_DATA_PACK:
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            tempPicData += data.mid(8,dataLen);
            tempPicDataLen += dataLen;
            float percent = tempPicDataLen/3840000.0 * 100;
            QByteArray tempData = "接收进度:" + QByteArray::number(percent) + "%";
            emit appLogMessage_signal(tempData);
            break;
        }
        case PICTURE_DATA_PACK_INDEX:
        {
            break;
        }
        case TAKE_RGB_PICTURE_END:
        {
            QByteArray tempData = "接收到图片结束,总大小为3840000byte";
            emit appLogMessage_signal(tempData);
            QImage image(reinterpret_cast<const uchar*>(tempPicData.constData()), 1600, 1200, QImage::Format_RGB16);
            QByteArray jpegData;
            QBuffer buffer(&jpegData);
            buffer.open(QIODevice::WriteOnly);

            // 将图像保存为JPEG格式
            image.save(&buffer, "JPEG", 100);
            sendTcpData(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,SAVE_PC_PICTURE,buffer.buffer().size(),buffer.buffer()));
            emit pictureData_signal(buffer.buffer());           /* 主界面预览 */

            tempPicData.clear();
            tempPicDataLen = 0;

            break;
        }
        case TAKE_PICTURE_END:
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            emit pictureData_signal(data.mid(8,dataLen));
            sendTcpData(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,SAVE_PC_PICTURE,dataLen,data.mid(8,dataLen)));
            QByteArray tempData = "接收到图片完成";
            emit appLogMessage_signal(tempData);
            break;
        }
        case SET_RECORD_TIME_SUCCESS:
        {
            emit appLogMessage_signal("固定定时设置成功!");
            break;
        }
        case SET_SCHEDULED_TIME_CMD_SUCCESS:
        {
            emit appLogMessage_signal("间隔定时设置成功!");
            break;
        }
        case OPEN_MOTO_SUCCESS_CMD:
        {
            emit appLogMessage_signal("电机打开成功!");
            break;
        }
        default:
            break;
    }


}


