#include "tcp_serverworker.h"
#include "qthread.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

/*主要实现TCP连接中的线程任务处理*/
TCP_ServerWorker::TCP_ServerWorker(QObject *parent)
    : QObject(parent)
{

}

void TCP_ServerWorker::StartListenTCPClient(QString TcpServerIP,unsigned short TcpServerPort)
{
    qDebug()<<"StartListenTCPClient threadId: "<<QThread::currentThreadId();
    TCP_ServerWorker::TCP_Server = new QTcpServer;

    QHostAddress LocalIP = QHostAddress(TcpServerIP);
    TCP_ServerWorker::TCP_Server->listen(LocalIP,TcpServerPort);/*开始监听*//*QHostAddress::AnyIPv4*//*QTcpServer同一端口只能绑定一次！！！新建对象完成其他端口的绑定*/

    connect(TCP_Server,&QTcpServer::newConnection,this,&TCP_ServerWorker::NewTcpConnect,Qt::UniqueConnection);
    qDebug()<<"TCP   IP:  "<<TCP_Server->serverAddress().toString();
    qDebug()<<"TCP PORT:  "<<TcpServerPort;

    activeCheckTimer = new QTimer;
    activeCheckTimer->start(30000);
    connect(activeCheckTimer,&QTimer::timeout,this,&TCP_ServerWorker::checkTcpIsActive);
}

void TCP_ServerWorker::NewTcpConnect()
{
    /*有新的客户端连接加入*/
    TCP_ServerSocket = new QTcpSocket;
    TCP_ServerSocket = TCP_Server->nextPendingConnection();/*连接TCP客户端*/
    qDebug()<<"TCP_Server->serverAddress: "<<TCP_Server->serverAddress();
    QString ClientIP   = TCP_ServerSocket->peerAddress().toString();
    QString ClientPort = QString::number(TCP_ServerSocket->peerPort()); /*获取当前的socket的IP和端口号*/

    SocketInformationBind *SocketsInformation = new SocketInformationBind;
    SocketsInformation->socket = TCP_ServerSocket;
    SocketsInformation->ClientIP_String = ClientIP;
    SocketsInformation->ClientPort_String = ClientPort;
    SocketsInformation->targetIP_string = "";
    SocketsInformation->targetPort_string = "";
    SocketsInformation->ClientLocalUserID = 0;
    SocketsInformation->ClientFarUserID = 0;
    SocketsInformation->isActive = true;
    SocketsInformation->cameraDevice = false;
    SocketsInformation->cameraDeviceId = "";
    SocketsInformation->pcClient = true;
    SocketsInformation->clientState = 0;
    SocketsInformation->tcpDataLen = 0;
    SocketsInformation->recNextData = false;
    SocketsInformation->recTcpData.clear();
    SocketsInformation->existFileNameList.clear();/*写入信息*/

    Tcp_ClientInformationList.append(SocketsInformation);/*把当前的socket加入链表中*/
    qDebug() <<"新用户加入";
    connect(TCP_ServerSocket,&QTcpSocket::readyRead,this,&TCP_ServerWorker::CilentDataRead);        /*关联读取数据的处理*/
    connect(TCP_ServerSocket,&QTcpSocket::disconnected,this,&TCP_ServerWorker::ClientDisconnected); /*关联断开连接信号的处理*/
    emit TCP_ServerWorker::SocketInformationConnectSignal(ClientIP,ClientPort,Tcp_ClientInformationList.count()); /*向UI线程发送当前的连接客户端的socket信息*/
}

/* TCP接收槽函数 */
void TCP_ServerWorker::CilentDataRead()
{
    QTcpSocket *TempSocket = (QTcpSocket*)sender();/*获取产生信号来源的socket*/
    QByteArray receiveData = TempSocket->readAll();/*读出所有数据*/
    unsigned int clientIndex = 0;

    for(int i=0;i<Tcp_ClientInformationList.count();i++)
    {
        if((TempSocket->peerAddress().toString() == Tcp_ClientInformationList.at(i)->ClientIP_String) &&
                (TempSocket->peerPort() == Tcp_ClientInformationList.at(i)->ClientPort_String.toInt()))
        {
            clientIndex = i;
            break;
        }
    }

    Tcp_ClientInformationList.at(clientIndex)->isActive = true;
    Tcp_ClientInformationList.at(clientIndex)->recTcpData += receiveData;/* 保存客户端发送来的数据 */

    if(Tcp_ClientInformationList.at(clientIndex)->recNextData == true)/* 判断是否一个数据帧的数据还没有接收完毕 */
    {
        if(Tcp_ClientInformationList.at(clientIndex)->recTcpData.size() >
                            Tcp_ClientInformationList.at(clientIndex)->tcpDataLen)/* 接收总数大于数据长度了，可以开始解析数据了 */
        {
            Tcp_ClientInformationList.at(clientIndex)->recNextData = false;
        }
        else
        {
            return;/* 数据不够继续接受 */
        }
    }

    if(Tcp_ClientInformationList.at(clientIndex)->recTcpData.size() < 4)/* 接收的数据小于能够计算的数据长度字节数4，即不能得到后面的数据长度 */
    {
        return;/* 退出 */
    }
    else
    {
        /* TCP传输使命必达，此时前四个字节保存后面要发送数据长度 */
        /* 计算一次数据长度 */
        Tcp_ClientInformationList.at(clientIndex)->tcpDataLen =
                qbyteArrayToUint(Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,4));/* 得到数据长度 */

        while(Tcp_ClientInformationList.at(clientIndex)->recTcpData.size() >
                    Tcp_ClientInformationList.at(clientIndex)->tcpDataLen - 1)/* 剩余长度大于数据长度，可以做处理 */
        {
            Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,4);/* 删除携带的数据长度字段 */
            Tcp_ClientInformationList.at(clientIndex)->cmd =
                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.at(0);/* 得到当前数据帧的功能 */
            Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,1);/* 删除cmd字段 */
            /* 剩下数据帧的内容全部为数据 */

            switch(Tcp_ClientInformationList.at(clientIndex)->cmd)/* 根据不同的功能进行处理 */
            {
                case HERAT_BEAT_PACK:/* 心跳包 */
                {
                    Tcp_ClientInformationList.at(clientIndex)->isActive = true;
                    qDebug() << "心跳包";
                    break;
                }
                case TAKE_PICTURE:/* 拍照命令 */
                {
                    emit appLogMessage_signal("接收到拍照命令");
                    for(int i=0;i<Tcp_ClientInformationList.size();i++)
                    {
                        if(Tcp_ClientInformationList.at(i)->cameraDevice == true)
                        {
                            /* 像摄像头设备发送拍照命令 */
                            Tcp_ClientInformationList.at(i)->socket->write(setCmdFrameFormat(1,(unsigned char)CAMERA_TAKE_PICTURE));
                        }
                    }
                    QTimer *takePictureOutTimer = new QTimer;
                    takePictureOutTimer->start(10*1000);/* 定时10s */
                    isTakePictureError  = false;

                    connect(takePictureOutTimer,&QTimer::timeout,this,&TCP_ServerWorker::on_takePictureError);
                    qDebug() << "拍照";
                    break;
                }
                case CAMERA_TAKE_PICTURE_DONE:/* 拍照完成命令 */
                {
                    emit appLogMessage_signal("设备已经完成了拍照");
                    for(int i=0;i<Tcp_ClientInformationList.size();i++)
                    {
                        if(Tcp_ClientInformationList.at(i)->cameraDevice != true)
                        {
                            /* 像摄像头设备发送拍照命令 */
                            Tcp_ClientInformationList.at(i)->socket->write(setCmdFrameFormat(1,(unsigned char)CAMERA_TAKE_PICTURE_DONE));
                        }
                    }
                    break;
                }
                case PICTURE_DATA:/* 设备发送的图片信息 */
                {
                    emit appLogMessage_signal("收到设备发送的图片");
                    /* 保存图像 */
                    QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
                    QDir *videoDir;
                    videoDir = new QDir(picName);
                    if(!videoDir->exists())/* 查找是否存在视频文件 */
                    {
                        /* 不存在就创建 */
                        bool ismkdir = videoDir->mkdir(picName);
                        if(!ismkdir)
                            qDebug() << "创建文件夹失败";
                        else
                            qDebug() << "创建文件夹成功";
                    }
                    delete videoDir;
                    picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd");
                    videoDir = new QDir(picName);
                    if(!videoDir->exists())/* 查找是否存在视频文件 */
                    {
                        /* 不存在就创建 */
                        bool ismkdir = videoDir->mkdir(picName);
                        if(!ismkdir)
                            qDebug() << "创建文件夹失败";
                        else
                            qDebug() << "创建文件夹成功";
                    }
                    delete videoDir;
                    picName += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".jpeg";
                    QFile *RecFile = new QFile(picName);
                    RecFile->open(QFile::WriteOnly);
                    RecFile->write(Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,
                                        Tcp_ClientInformationList.at(clientIndex)->tcpDataLen - 1));
                    RecFile->close();
                    RecFile->deleteLater();
                    delete RecFile;

                    emit appLogMessage_signal("图片大小:" + QByteArray::number(Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,
                                                            Tcp_ClientInformationList.at(clientIndex)->tcpDataLen - 1).size()));

                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                    break;
                }
                case CLEAR_SERVER_CACHE:/* 清除服务器缓存 */
                {
                    emit appLogMessage_signal("收到清除缓存命令");
                    QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
                    QDir dir;
                    dir.setPath(picName);
                    dir.removeRecursively();
                    TempSocket->write(setCmdFrameFormat(1,(unsigned char)CLEAR_SERVER_CACHE_DONE));
                    break;
                }
                case SET_CAMERA_DEVICE_FLAG:/* 设置相机设备标志命令 */
                {
                    QByteArray tempString = "IP为" + Tcp_ClientInformationList.at(clientIndex)->ClientIP_String.toLocal8Bit() +
                             "端口为" + Tcp_ClientInformationList.at(clientIndex)->ClientPort_String.toLocal8Bit() +
                             "的远程相机设备加入";/* 临时字符串变量 */
                    emit appLogMessage_signal(tempString);
                    Tcp_ClientInformationList.at(clientIndex)->cameraDevice = true;/* 标记该设备为摄像头设备 */
                    break;
                }
                case SET_CAMERA_DEVICE_ID:/* 设置相机设备ID命令 */
                {
                    Tcp_ClientInformationList.at(clientIndex)->cameraDeviceId =
                            Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);/* 获取设备ID */
                    qDebug() << "设备ID" << Tcp_ClientInformationList.at(clientIndex)->cameraDeviceId;
                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                    break;
                }
                case DOWNLOAD_PICTURE:/* 客户端请求下载图片数据 */
                {
                    emit appLogMessage_signal("客户端请求下载图片");
                    QByteArray date = Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,
                                                        Tcp_ClientInformationList.at(clientIndex)->tcpDataLen - 1);
                    if(date.isEmpty())return;

                    qDebug() << "date:" << date;
                    /* 遍历服务器存储的所有图片文件 */
                    QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + date;
                    qDebug() << "picPath:" << picPath;
                    QDir dir(picPath);
                    QStringList filename ;
                    filename << "*.jpeg";//可叠加，可使用通配符筛选
                    QStringList fileResults;
                    fileResults = dir.entryList(filename,QDir::Files | QDir::Readable,QDir::Name);
                    if(fileResults.size() == 0)
                    {
                        Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                    Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                        TempSocket->write(setCmdFrameFormat(1,(unsigned char)EMPTY));
                        return;
                    }
                    qDebug() <<fileResults.at(0);
                    for(int i=0;i<fileResults.size();i++)
                    {
                        for(int j=0;j<Tcp_ClientInformationList.at(clientIndex)->existFileNameList.count();j++)
                        {
                            if(fileResults.at(i) == Tcp_ClientInformationList.at(clientIndex)->existFileNameList.at(j))
                            {
                                fileResults.removeAt(i);
                            }
                        }
                    }
                    Tcp_ClientInformationList.at(clientIndex)->existFileNameList.clear();
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

                        TempSocket->write(setDataFrameFormat(1 + picName.size(),/* 数据总长 */
                                           (unsigned char)PICTURE_TO_CLIENT_NAME, /* 数据帧功能 */
                                           picName.toLocal8Bit()));/* 发送图片名称 */
                        TempSocket->write(setDataFrameFormat(1 + picData.size(),/* 数据总长 */
                                           (unsigned char)PICTURE_TO_CLIENT_DATA, /* 数据帧功能 */
                                           picData));/* 发送图片数据 */
                    }
                    TempSocket->write(setCmdFrameFormat(1,(unsigned char)PICTURE_TO_CLIENT_END));/* 告诉客户端图片数据发送完成 */
                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                    break;
                }
                case CLIENT_PICTURE_FILE_NAME:/* 得到客户端那边已经下载的图片名称 */
                {
                    Tcp_ClientInformationList.at(clientIndex)->existFileNameList.append(
                                Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1));

                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                    break;
                }
                case PICTURE_ERROR:/* 得到ESP发送来的图像错误信息 */
                {
                    for(int i=0;i<Tcp_ClientInformationList.size();i++)
                    {
                        if(Tcp_ClientInformationList.at(i)->cameraDevice != true)
                        {
                            /* 像摄像头设备发送拍照命令 */
                            Tcp_ClientInformationList.at(i)->socket->write(setCmdFrameFormat(1,(unsigned char)PICTURE_ERROR));
                        }
                    }
                    break;
                }
                case SET_PC_DEVICE_FLAG:/* 设置客户端标志命令 */
                {
                    QByteArray tempString = "IP为" + Tcp_ClientInformationList.at(clientIndex)->ClientIP_String.toLocal8Bit() +
                             "端口为" + Tcp_ClientInformationList.at(clientIndex)->ClientPort_String.toLocal8Bit() +
                             "的远程客户端设备加入";/* 临时字符串变量 */
                    emit appLogMessage_signal(tempString);
                    Tcp_ClientInformationList.at(clientIndex)->pcClient = true;/* 标记该设备为客户端设备 */
                    break;
                }
                case GET_ONLINE_DEVICE:/* 获取在线设备的列表命令 */
                {
                    for(int i=0;i<Tcp_ClientInformationList.count();i++)
                    {
                        if(Tcp_ClientInformationList.at(i)->cameraDevice == true)/* 如果是相机设备 */
                        {
                            TempSocket->write(setDataFrameFormat(1 + Tcp_ClientInformationList.at(i)->cameraDeviceId.size(),/* 数据总长 */
                                               (unsigned char)ONLINE_CAMERA_DEVICE_ID_TO_CLIENT, /* 数据帧功能 */
                                               Tcp_ClientInformationList.at(i)->cameraDeviceId));/* 发送设备ID */
                        }
                    }
                    TempSocket->write(setCmdFrameFormat(1,(unsigned char)ONLINE_CAMERA_DEVICE_LIST_TO_CLIENT_END));
                    break;
                }
                case CLIENT_BIND_CAMERA:/* 绑定设备命令 */
                {
                    for(int i=0;i<Tcp_ClientInformationList.count();i++)
                    {
                        if(Tcp_ClientInformationList.at(i)->cameraDeviceId ==
                                Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1))
                        {
                            Tcp_ClientInformationList.at(clientIndex)->targetIP_string = Tcp_ClientInformationList.at(i)->ClientIP_String;
                            Tcp_ClientInformationList.at(clientIndex)->targetPort_string = Tcp_ClientInformationList.at(i)->ClientPort_String;
                            break;
                        }
                    }
                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                    break;
                }
                default:/* 未知的cmd */
                    Tcp_ClientInformationList.at(clientIndex)->recTcpData.remove(0,
                                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen-1);
                    break;
            }
            if(Tcp_ClientInformationList.at(clientIndex)->recTcpData.size() < 4)/* 接收的数据小于能够计算的数据长度字节数4，即不能得到后面的数据长度 */
            {
                return;/* 退出 */
            }
            else
            {
                Tcp_ClientInformationList.at(clientIndex)->tcpDataLen =
                        qbyteArrayToUint(Tcp_ClientInformationList.at(clientIndex)->recTcpData.mid(0,4));/* 得到数据长度 */
                qDebug() << Tcp_ClientInformationList.at(clientIndex)->tcpDataLen;

                if(Tcp_ClientInformationList.at(clientIndex)->tcpDataLen >
                        Tcp_ClientInformationList.at(clientIndex)->recTcpData.size())/* 剩余的数据不够数据长度，还没接收完毕 */
                {
                    Tcp_ClientInformationList.at(clientIndex)->recNextData = true;
                    return;
                }
                /* 接下来就会进入while的条件判断 */
            }
        }
    }
}

void TCP_ServerWorker::ClientDisconnected()
{
    if(isDisConnectedAll == true)return;
    /*客户端断开连接处理槽函数*/
    QTcpSocket *TempSocket = (QTcpSocket*)sender();/*获取产生信号来源的socket*/
    qDebug() <<"有客户端断开了连接";
    qDebug() <<"ClientDisconnected" << Tcp_ClientInformationList.count();
    for(int i=0;i<Tcp_ClientInformationList.count();i++)
    {
        /*把这个socket从保存的信息中删除*/
        qDebug() <<"ClientDisconnected" << Tcp_ClientInformationList.count();
        if((TempSocket->peerAddress().toString() == Tcp_ClientInformationList.at(i)->ClientIP_String) &&
                (TempSocket->peerPort() == Tcp_ClientInformationList.at(i)->ClientPort_String.toInt()))
        {
            QString ClientIP   = Tcp_ClientInformationList[i]->ClientIP_String;
            QString ClientPort = Tcp_ClientInformationList[i]->ClientPort_String;           /*获取当前的socket的IP和端口号*/

            Tcp_ClientInformationList.removeAt(i);                                          /*从链表中删除这个socket*//*bug：和服务器主动断开的槽函数发生冲突*/
            qDebug() <<"ClientDisconnected1" << Tcp_ClientInformationList.count();
            emit TCP_ServerWorker::SocketInformationDisconnectSignal(ClientIP,ClientPort,Tcp_ClientInformationList.count());  /*向UI线程发送当前的socket的相关信息*/
            break;
        }
    }
}

void TCP_ServerWorker::ProgramOver()
{
    /*结束所有客户带端的连接*/
    isDisConnectedAll = true;

    qDebug() <<"当前连接客户端数量为"<<Tcp_ClientInformationList.count();

    /*把socket信息删除*/
    for(int i=0;i<Tcp_ClientInformationList.count();i++)
    {
        Tcp_ClientInformationList[i]->socket->disconnectFromHost();/*断开和客户端的连接*/
        Tcp_ClientInformationList[i]->socket->close();
        Tcp_ClientInformationList[i]->socket->deleteLater();
    }
    Tcp_ClientInformationList.clear();/*清除连接用户的信息*/
    qDebug() <<"当前连接客户端数量为"<<Tcp_ClientInformationList.count();

    TCP_Server->close();
    TCP_Server->deleteLater();/*删除TCP_Server，客户端不能再连接客户端，同时避免绑定同一个端口时发生意想不到的错误*/

    qDebug() <<"断开所有连接";

    emit TCP_ServerWorker::SocketInformationConnectSignal(0,0,Tcp_ClientInformationList.count());
    isDisConnectedAll = false;
}

void TCP_ServerWorker::ServerSendDataToClient(unsigned int ClientID, QByteArray ServerData)
{
    for(int i=0;i<Tcp_ClientInformationList.count();i++)
    {
        if(ClientID == Tcp_ClientInformationList[i]->ClientLocalUserID)/*找到远程客户端的ID*/
        {
            Tcp_ClientInformationList[i]->socket->write(ServerData);
            break;/*查找到指定的socket退出*/
        }
    }
}

void TCP_ServerWorker::checkTcpIsActive()
{
    qDebug() << "checkTcpIsActive";
    for(int i=0;i<Tcp_ClientInformationList.count();i++)
    {
        if(Tcp_ClientInformationList[i]->isActive == false)
        {
            Tcp_ClientInformationList.removeAt(i);
        }
        else if(Tcp_ClientInformationList[i]->isActive == true)
        {
            Tcp_ClientInformationList[i]->isActive = false;
        }
    }
    emit TCP_ServerWorker::SocketInformationConnectSignal(0,0,Tcp_ClientInformationList.count()); /*向UI线程发送当前的连接客户端的socket信息*/
}

QByteArray TCP_ServerWorker::uIntToQbyteArray(unsigned int uIntData)
{
    QByteArray dataBuffer;
    dataBuffer.resize(4);
    for(unsigned char i=0;i<4;i++)
    {
        dataBuffer[i] = (uIntData>>i*8);
    }
    return dataBuffer;
}

unsigned int TCP_ServerWorker::qbyteArrayToUint(QByteArray dataBuffer)
{
    unsigned int value = 0;
    for(unsigned char i=0;i<4;i++)
    {
        value |= ((unsigned char)dataBuffer.at(i)<< (i*8));
    }
    return value;
}

QByteArray TCP_ServerWorker::setCmdFrameFormat(unsigned int dataLen, unsigned char cmd)
{
    QByteArray tempData;
    tempData = uIntToQbyteArray(dataLen);/* 填充后面的数据长度 */
    tempData.append(cmd);/* cmd */
    return tempData;
}

QByteArray TCP_ServerWorker::setDataFrameFormat(unsigned int dataLen, unsigned char cmd, QByteArray data)
{
    QByteArray tempData;
    tempData = uIntToQbyteArray(dataLen);/* 填充后面的数据长度 */
    tempData.append(cmd);/* cmd */
    tempData += data;
    return tempData;
}
/* 解决拍照发生错误的槽函数 */
void TCP_ServerWorker::on_takePictureError()
{
    qDebug() << "拍照错误处理函数！";
    QTimer *tempTimer = (QTimer*)sender();/*获取产生信号来源的socket*/

    for(int i=0;i<Tcp_ClientInformationList.size();i++)
    {
        if(Tcp_ClientInformationList.at(i)->cameraDevice != true)
        {
            /* 像摄像头设备发送拍照命令 */
            emit appLogMessage_signal("设备接收图片数超时");
            Tcp_ClientInformationList.at(i)->socket->write(setCmdFrameFormat(1,(unsigned char)PICTURE_ERROR));
        }
    }
    tempTimer->stop();
    tempTimer->deleteLater();
}


