#include "mytcpsocket.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QThread>


myTcpSocket::myTcpSocket(QObject *parent)
    : QObject{parent}
{

}

myTcpSocket::~myTcpSocket()
{
    myTcpSocketList =nullptr;

    tcpSocket->close();
    delete tcpSocket;
    tcpSocket = nullptr;

    tcpHeartBeatCheckTimer->stop();
    delete tcpHeartBeatCheckTimer;
    tcpHeartBeatCheckTimer = nullptr;
    qDebug()<<"~myTcpSocket()";
}
/* 设置socketDescriptor */
void myTcpSocket::setSocketDescriptor(qintptr socketDescriptor)
{
    mSocketDescriptor = socketDescriptor;
}
/* 根据socketDescriptor在子线程创建socket */
void myTcpSocket::createTcpSocket()
{
    qDebug() << "socketDescriptor = " << mSocketDescriptor ;
    tcpSocket = new QTcpSocket();
    tcpSocket->setSocketDescriptor(mSocketDescriptor);                                  /* 实例化socket */

    connect(tcpSocket,&QTcpSocket::readyRead,this,&myTcpSocket::readTcpData);           /*关联读取数据的处理*/
    connect(tcpSocket,&QTcpSocket::errorOccurred,this,&myTcpSocket::on_errorOccurred);  /*关联读取数据的处理*/
    connect(tcpSocket,&QTcpSocket::disconnected,this,&myTcpSocket::tcpDisconnect);      /*关联断开连接信号的处理*/

    tcpHeartBeatCheckTimer = new QTimer(this);                                          /* 创建检查tcp心跳包的定时器 */
    tcpHeartBeatCheckTimer->start(60000);                                               /* 60s检查一次连接是否正常 */
    connect(tcpHeartBeatCheckTimer,&QTimer::timeout,this,&myTcpSocket::on_checkTcpHeartBeat,Qt::UniqueConnection);

    mTcpClientIp = tcpSocket->peerAddress();                                            /* tcp socket的IP */
    mTcpClientPort = tcpSocket->peerPort();                                             /* tcp socket的PORT */
    emit UI_SocketInformationConnectShow_signal(mTcpClientIp,mTcpClientPort);           /* 发送连接的tcp客户端 */
    tcpConnectIsAlive = true;                                                           /* tcp连接正常 */
}

QHostAddress myTcpSocket::returnIp()
{
    return mTcpClientIp;
}

uint16_t myTcpSocket::returnPort()
{
    return mTcpClientPort;
}
/* tcp心跳包确认保活 */
void myTcpSocket::on_checkTcpHeartBeat()
{
    if(tcpConnectIsAlive)
    {
        tcpConnectIsAlive = false;
    }
    else
    {
        deleteFlag = true;
        QThread::currentThread()->quit();/* 请求退出线程 */
    }
}
/* tcp连接错误处理函数 */
void myTcpSocket::on_errorOccurred(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
        case QAbstractSocket::ConnectionRefusedError:
            // 连接被拒绝
            qDebug() << "Connection refused error";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            // 远程主机关闭连接
            qDebug() << "Remote host closed error";
            break;
        case QAbstractSocket::HostNotFoundError:
            // 无法找到主机
            qDebug() << "Host not found error";
            break;
        case QAbstractSocket::SocketAccessError:
            // 操作被拒绝
            qDebug() << "Socket access error";
            break;
        case QAbstractSocket::SocketTimeoutError:
            // 超时错误
            qDebug() << "Socket timeout error";
            break;
        case QAbstractSocket::NetworkError:
            // 网络错误
            qDebug() << "Network error";
            break;
        default:
            // 其他错误
            qDebug() << "Unknown error occurred";
            break;
    }
}

void myTcpSocket::setTcpSocketList(QList<myTcpSocket *> *p)
{
    myTcpSocketList = p;
    qDebug() << "myTcpSocketList->count()" << myTcpSocketList->count();
}
/* tcp数据接收槽函数 */
void myTcpSocket::readTcpData()
{
    tcpConnectIsAlive = true;
    receiveData += tcpSocket->readAll();
    for(uint32_t i=0;i<receiveData.size();)
    {
        if(0xAA == (uint8_t)receiveData.at(i) && (receiveData.size() - i >= 8))         /* 确认数据帧头同时能得到数据长度 */
        {
            uint32_t dataLen =(uint8_t) receiveData.at(i+4)| (uint8_t)receiveData.at(i+5)<<8 |(uint8_t) receiveData.at(i+6)<<16 |(uint8_t) receiveData.at(i+7)<<24 ;
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
/* 发送TCP数据槽函数 */
void myTcpSocket::sendTcpData(QByteArray tcpData)
{
    if(tcpSocket->isWritable())/* 连接正常再发送数据 */
    {
        tcpSocket->write(tcpData);

    }
    qDebug()<<"SendTcpData" << tcpData;
}
/* 断开连接槽函数 */
void myTcpSocket::tcpDisconnect()
{
    qDebug()<<"断开连接";
    deleteFlag = true;
    QThread::currentThread()->quit();/* 请求退出线程 */
}

void myTcpSocket::frameDataAnalysis(QByteArray data)
{
    switch(data.at(2))
    {
        case PC_TO_SERVER:                      /* pc到服务器 */
        {
            pcToServerDeal(data);
            break;
        }
        case PC_TO_CAMER:                       /* pc转发数据到camera */
        {
            pcToCameraDeal(data);
            break;
        }
        case CAMER_TO_SERVER:                   /* camera到服务器 */
        {
            cameraToServerDeal(data);
            break;
        }
        case CAMER_TO_PC:                       /* camera转发数据到pc */
        {
            cameraToPcDeal(data);
            break;
        }
        default:
            break;
    }
}

void myTcpSocket::pcToServerDeal(QByteArray data)
{
    switch(data.at(3))
    {
        case HERAT_BEAT_PACK:           /* 心跳包 */
        {
            tcpConnectIsAlive = true;
            break;
        }
        case SET_PC_DEVICE_FLAG:        /* PC角色设置 */
        {
            qDebug()<<"设置客户端为PC标志";
            deviceLabel = DEVICE_LABEL_PC;
            break;
        }
        case GET_ONLINE_CAMERA_DEVICE_ID:/* 获取连接的camera设备ID */
        {
            qDebug()<<"获取camera设备ID";
            for(int i=0;i<myTcpSocketList->count();i++)
            {
                /* 检索设备标识是CAMERA的SOCKET列表 */
                if(DEVICE_LABEL_CAMERA == myTcpSocketList->at(i)->deviceLabel)
                {
                    qDebug() << myTcpSocketList->at(i)->cameraId;
                    if(!myTcpSocketList->at(i)->cameraId.isEmpty())
                    {
                        sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,CAMERA_ID,
                                                 myTcpSocketList->at(i)->cameraId.size(),myTcpSocketList->at(i)->cameraId));
                    }
                }
            }
            /* 检索完成后发送结束标志 */
            sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,SEND_CAMERA_ID_END,0,0));
            break;
        }
        case CLIENT_BIND_CAMERA:           /* 相机绑定命令 */
        {
            qDebug()<<"绑定设备ID";
            /* 取出帧中包含的设备ID */
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            QByteArray cameraId = data.mid(8,dataLen);
            bindCameraID = cameraId;
            for(int i=0;i<myTcpSocketList->count();i++)
            {
                /* 检索设备标识是CAMERA的SOCKET列表 */
                if(DEVICE_LABEL_CAMERA == myTcpSocketList->at(i)->deviceLabel)
                {
                    qDebug() << myTcpSocketList->at(i)->cameraId;
                    if(!myTcpSocketList->at(i)->cameraId.isEmpty() &&
                            cameraId == myTcpSocketList->at(i)->cameraId)
                    {
                        qDebug()<<"检索到待绑定的设备ID";
                        connect(this,&myTcpSocket::sendBindTcpMessage,myTcpSocketList->at(i),&myTcpSocket::sendTcpData,Qt::UniqueConnection);
                        connect(myTcpSocketList->at(i),&myTcpSocket::sendBindTcpMessage,this,&myTcpSocket::sendTcpData,Qt::UniqueConnection);
                        /* 发送绑定成功信号 */
                        sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,CLIENT_BIND_CAMERA_OK,0,0));
                        break;/* 检索到设备，退出检索 */
                    }
                }
            }
            break;
        }
        case CLIENT_DISBIND_CAMERA:         /* 相机解除绑定命令 */
        {
            /* 取出帧中包含的设备ID */
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            QByteArray cameraId = data.mid(8,dataLen);
            bindCameraID.clear();
            for(int i=0;i<myTcpSocketList->count();i++)
            {
                /* 检索设备标识是CAMERA的SOCKET列表 */
                if(DEVICE_LABEL_CAMERA == myTcpSocketList->at(i)->deviceLabel)
                {
                    qDebug() << myTcpSocketList->at(i)->cameraId;
                    if(!myTcpSocketList->at(i)->cameraId.isEmpty() &&
                            cameraId == myTcpSocketList->at(i)->cameraId)
                    {
                        qDebug()<<"检索到绑定的设备ID";
                        disconnect(this,&myTcpSocket::sendBindTcpMessage,myTcpSocketList->at(i),&myTcpSocket::sendTcpData);
                        disconnect(myTcpSocketList->at(i),&myTcpSocket::sendBindTcpMessage,this,&myTcpSocket::sendTcpData);
                        /* 发送接触绑定成功信号 */
                        sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,CLIENT_DISBIND_CAMERA_OK,0,0));
                        break;/* 检索到设备，退出检索 */
                    }
                }
            }
            break;
        }
        case CLIENT_EXIST_PICTURE_FILE_NAME:                                                        /* 客户端发送本地存在的图片文件命令 */
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;        /* 得到数据长度 */
            QByteArray existPictureName = cameraId = data.mid(8,dataLen);                           /* 得到数据 */
            tempExistFileNameList.append(existPictureName);                                         /* 添加到临时列表 */
            break;
        }
        case CLIENT_EXIST_PICTURE_FILE_NAME_END:                                                    /* 客户端发送本地存在的图片文结束件命令 */
        {
            tempExistFileNameList.clear();
            existFileNameList.clear();
            existFileNameList.append(tempExistFileNameList);                                        /* 添加到列表 */
            break;
        }
        case SAVE_PC_PICTURE:                                                                       /* 保存客户端的图片数据 */
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;        /* 得到数据长度 */
            /* 保存图像 */
            /* 创建图片文件夹 */
            QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
            QDir *videoDir;
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            delete videoDir;
            /* 创建 图片/设备ID 文件夹 */
            picName += "/" + bindCameraID;
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            /* 创建 图片/设备ID/日期年月日 文件夹 */
            picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd");
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            delete videoDir;
            /* 创建 图片/设备ID/日期年月日/时分秒 文件 */
            picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".jpeg";
            QFile *RecFile = new QFile(picName);
            RecFile->open(QFile::WriteOnly);
            RecFile->write(data.mid(8,dataLen));
            RecFile->close();
            delete RecFile;
            break;
        }
        case SET_TEMP_DEVICE_ID:
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;        /* 得到数据长度 */
            bindCameraID = data.mid(8,dataLen);
            break;
        }
        case DOWNLOAD_PICTURE:
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;        /* 得到数据长度 */
            QByteArray date = data.mid(8,dataLen);
            /* 遍历服务器存储的所有图片文件 */
            QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraID+ "/" + date;
            QDir dir(picPath);
            QStringList filename ;
            filename << "*.jpeg";//可叠加，可使用通配符筛选
            QStringList fileResults;
            fileResults = dir.entryList(filename,QDir::Files | QDir::Readable,QDir::Name);
            if(fileResults.size() == 0)         /* 没有找到数据 */
            {
                sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,DOWNLOAD_PICTURE_EMPTY,0,0));
                return;
            }
            for(int i=0;i<fileResults.size();i++)
            {
                for(int j=0;j<existFileNameList.count();j++)
                {
                    if(fileResults.at(i) == existFileNameList.at(j))
                    {
                        fileResults.removeAt(i);
                    }
                }
            }
            existFileNameList.clear();
            /* 发送图片到客户端 */
            for(int i=0;i<fileResults.size();i++)
            {
                QByteArray picData;
                QString picName = fileResults.at(i);
                QString readFilePath = picPath + "/" + picName;
                QFile file(readFilePath);
                qDebug() << readFilePath;
                bool isReadOK = file.open(QIODevice::ReadOnly); //只读模式打开
                if(isReadOK)
                {
                    qDebug() << "读取成功";
                    picData = file.readAll();
                    file.close();
                }
                else
                {
                    qDebug() << "读取失败";
                    file.close();
                }
                /* 发送图片名字 */
                sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,PICTURE_TO_PC_NAME,picName.size(),picName.toLocal8Bit()));
                /* 发送图片数据 */
                sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,PICTURE_TO_PC_DATA,picData.size(),picData));
            }
            /* 告诉客户端图片数据发送完成 */
            sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,DOWNLOAD_PICTURE_END,0,0));
            break;
        }
        case CLEAR_SERVER_CACHE:
        {
            QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
            QDir dir;
            dir.setPath(picName);
            dir.removeRecursively();
            sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,CLEAR_SERVER_CACHE_DONE,0,0));
            break;
        }
        default:
            break;
    }
}

void myTcpSocket::pcToCameraDeal(QByteArray data)
{
    emit sendBindTcpMessage(data);
}

void myTcpSocket::cameraToServerDeal(QByteArray data)
{
    switch(data.at(3))
    {
        case HERAT_BEAT_PACK:                                       /* 心跳包 */
        {
            tcpConnectIsAlive = true;
            break;
        }
        case CAMERA_TRY_SERVER_CHECK:                               /* 心跳包 */
        {
            sendTcpData(packTcpDataFrame(DEVICE_LABEL_SERVER,SERVER_TO_CLIENT,SERVER_CHECK_BACK,0,0));
            break;
        }
        case SET_CAMERA_DEVICE_FLAG:                                /* CAMERA角色设置 */
        {
            qDebug()<<"设置客户端为CAMERA标志";
            deviceLabel = DEVICE_LABEL_CAMERA;
            break;
        }
        case SET_CAMERA_DEVICE_ID:                                  /* CAMERA设置ID */
        {
            qDebug()<<"设置客户端ID";
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            cameraId = data.mid(8,dataLen);
            qDebug() << "ID = " << cameraId;
            break;
        }
        case TAKE_PICTURE_END:                                      /* 设备拍照,JPEG格式 */
        {
            /* 保存图像 */
            /* 创建图片文件夹 */
            QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
            QDir *videoDir;
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            delete videoDir;
            /* 创建 图片/设备ID 文件夹 */
            picName += "/" + cameraId;
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            /* 创建 图片/设备ID/日期年月日 文件夹 */
            picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd");
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            delete videoDir;
            /* 创建 图片/设备ID/日期年月日/时分秒 文件 */
            picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".jpeg";
            QFile *RecFile = new QFile(picName);
            RecFile->open(QFile::WriteOnly);
            RecFile->write(tempPicData);
            RecFile->close();
            delete RecFile;
            break;
        }
        case JPEG_DATA_PACK_SIZE:
        {
            getJpegDataPackSize = (uint8_t)data.at(8) | (uint8_t)data.at(9)<<8 | (uint8_t)data.at(10)<<16 | (uint8_t)data.at(11)<<24;
            tempPicDataLen = 0;
            tempPicData.clear();
            break;
        }
        case PICTURE_DATA_PACK:
        {
            uint32_t dataLen = (uint8_t)data.at(4) | (uint8_t)data.at(5)<<8 | (uint8_t)data.at(6)<<16 | (uint8_t)data.at(7)<<24;
            tempPicData += data.mid(8,dataLen);
            tempPicDataLen += dataLen;
            float percent = (float)tempPicDataLen/(float)getJpegDataPackSize * 100.0;
            QByteArray tempData = "接收进度:" + QByteArray::number(percent) + "%";
            emit appLogMessage_signal(tempData);
            break;
        }
        case TAKE_RGB_PICTURE_END:                                  /* 设备拍照,RGB格式 */
        {
            QByteArray tempData = "接收到图片结束,总大小为3840000byte";
            emit appLogMessage_signal(tempData);
            QImage image(reinterpret_cast<const uchar*>(tempPicData.constData()), 1600, 1200, QImage::Format_RGB16);
            QByteArray jpegData;
            QBuffer buffer(&jpegData);
            buffer.open(QIODevice::WriteOnly);

            // 将图像保存为JPEG格式
            image.save(&buffer, "JPEG", 100);

            /* 保存图像 */
            /* 创建图片文件夹 */
            QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
            QDir *videoDir;
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            delete videoDir;
            /* 创建 图片/设备ID 文件夹 */
            picName += "/" + cameraId;
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            /* 创建 图片/设备ID/日期年月日 文件夹 */
            picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd");
            videoDir = new QDir(picName);
            if(!videoDir->exists()) videoDir->mkdir(picName);/* 不存在就创建 */
            delete videoDir;
            /* 创建 图片/设备ID/日期年月日/时分秒 文件 */
            picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".jpeg";
            QFile *RecFile = new QFile(picName);
            RecFile->open(QFile::WriteOnly);
            RecFile->write(buffer.buffer());
            RecFile->close();
            delete RecFile;

            tempPicData.clear();
            tempPicDataLen = 0;
            break;
        }
        default:
            break;
    }
}

void myTcpSocket::cameraToPcDeal(QByteArray data)
{
    emit sendBindTcpMessage(data);
}

QByteArray myTcpSocket::packTcpDataFrame(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, QByteArray data)
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

/* myTcpServer构造函数 */
myTcpServer::myTcpServer(QHostAddress tcpIp, uint16_t tcpPort, QObject *parent)
{
    Q_UNUSED(parent);
    /* 监听客户端的连接 */
    this->listen(QHostAddress(tcpIp),tcpPort);
}
/* myTcpServer析构函数 */
myTcpServer::~myTcpServer()
{
    this->close();
    for(int i=0;i<myTcpSocketList.count();i++)
    {
        myTcpSocketList[i]->deleteLater();
    }
    myTcpSocketList.clear();
    for(int i=0;i<mySocketThread.count();i++)
    {
        mySocketThread.at(i)->quit();
        mySocketThread.at(i)->wait();
        delete mySocketThread[i];
    }
    mySocketThread.clear();
    qDebug()<<"~myTcpServer()";
}

QList<myTcpSocket *> myTcpServer::returnQlist()
{
    return myTcpSocketList;
}
/* 监听套接字监听到一个客户端连接 */
void myTcpServer::incomingConnection(qintptr socketDescriptor)
{
    myTcpSocketList.append(new myTcpSocket());                      /* 添加一个socket子对象 */
    mySocketThread.append(new QThread());                           /* 新建一个线程 */

    myTcpSocketList.last()->setSocketDescriptor(socketDescriptor);  /* 设置客户端的socket的描述符 */
    myTcpSocketList.last()->setTcpSocketList(&myTcpSocketList);
    myTcpSocketList.last()->moveToThread(mySocketThread.last());    /* 将socket对象移动到线程中运行 */

    mySocketThread.last()->start();                                 /* 线程启动 */

    connect(myTcpSocketList.last(),&myTcpSocket::appLogMessage_signal,this,&myTcpServer::appLogMessage_signal); /* log信息打印 */
    connect(myTcpSocketList.last(),&myTcpSocket::UI_SocketInformationConnectShow_signal,this,&myTcpServer::UI_SocketInformationConnectShow_signal); /* 连接tcp的客户端信息 */
    connect(mySocketThread.last(), &QThread::started, myTcpSocketList.last(),&myTcpSocket::createTcpSocket);    /* 连接线程启动信号 */
    connect(mySocketThread.last(), &QThread::finished, this,[=]()                                               /* 连接线程的结束信号 */
    {
        /* 清除保存的socket */
        for(int i=0;i<myTcpSocketList.count();i++)
        {
            if(myTcpSocketList.at(i)->deleteFlag)
            {
                qDebug() << "find socket";
                emit UI_SocketInformationDisonnectShow_signal(myTcpSocketList.at(i)->returnIp(),myTcpSocketList.at(i)->returnPort());
                myTcpSocketList[i]->deleteLater();
                myTcpSocketList.removeAt(i);
            }
        }
        for(int i=0;i<mySocketThread.count();i++)
        {
            /* 查找并且清除停止的线程 */
            if(!mySocketThread.at(i)->isRunning())
            {
                qDebug() << "find Thread";
                mySocketThread.at(i)->wait();   /* 等待线程的退出 */
                delete mySocketThread[i];       /* 清除资源 */
                mySocketThread.removeAt(i);     /* 清除列表中的元素 */
                break;
            }
        }
    });

    qDebug() << myTcpSocketList.count();
    qDebug() << mySocketThread.count();
    qDebug()<<"Incomming Connect...";

}
