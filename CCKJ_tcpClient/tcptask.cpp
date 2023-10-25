#include "tcptask.h"
#include "qthread.h"

#include <QDir>
#include <QImage>

tcpTask::tcpTask()
{
    qDebug()<<"CilentDataRead_1 threadId: "<<QThread::currentThreadId();
}

void tcpTask::startTcpConnect(QString tcpServerIp, QString tcpServerPort)
{
    TCP_Socket = new QTcpSocket(this);/*创建Socket对象*/
    TCP_Socket->connectToHost(QHostAddress(tcpServerIp),tcpServerPort.toUShort());/*连接服务器*/
    /* TCP连接发生错误 */
    connect(TCP_Socket,&QTcpSocket::errorOccurred,this,[=](){emit tcpDisconnected_signal();});
    /*连接成功TCP_Socket发出信号*/
    connect(TCP_Socket,&QTcpSocket::connected,this,[=](){emit tcpConnected_signal();});
    /*断开连接TCP_Socket发出信号*/
    connect(TCP_Socket,&QTcpSocket::disconnected,this,[=](){emit tcpDisconnected_signal();});
    /*客户端接收到服务器发送的信息*/
    connect(TCP_Socket,&QTcpSocket::readyRead,this,&tcpTask::readData);
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

void tcpTask::readData()
{
    QByteArray receiveData = TCP_Socket->readAll();

    static QByteArray allRecData;/* 保存所有的接收数据 */
    static QByteArray picName;
    static unsigned tcpDataLen = 0;
    static unsigned char cmd = 0;
    static bool recNextData = false;

    static QList<QByteArray> onlineDeviceList;
    allRecData += receiveData;/* 保存客户端发送来的数据 */
    if(recNextData == true)/* 判断是否一个数据帧的数据还没有接收完毕 */
    {
        if(allRecData.size() >tcpDataLen)/* 接收总数大于数据长度了，可以开始解析数据了 */
        {
            recNextData = false;
        }
        else
        {
            return;/* 数据不够继续接受 */
        }
    }
    if(allRecData.size() < 4)/* 接收的数据小于能够计算的数据长度字节数4，即不能得到后面的数据长度 */
    {
        return;/* 退出 */
    }
    else
    {
        /* TCP传输使命必达，此时前四个字节保存后面要发送数据长度 */
        //qDebug() << "receiveData = " << receiveData;
        /* 计算一次数据长度 */
        tcpDataLen =qbyteArrayToUint(allRecData.mid(0,4));/* 得到数据长度 */

        while(allRecData.size() >tcpDataLen - 1)/* 剩余长度大于数据长度，可以做处理 */
        {
            allRecData.remove(0,4);/* 删除携带的数据长度字段 */
            cmd =allRecData.at(0);/* 得到当前数据帧的功能 */
            allRecData.remove(0,1);/* 删除cmd字段 */
            /* 剩下数据帧的内容全部为数据 */

            switch(cmd)/* 根据不同的功能进行处理 */
            {
                case EMPTY:/* 服务器没有对应的数据 */
                    emit pictureDownload_signal(PICTURE_EMPTY);
                    break;
                case PICTURE_TO_CLIENT_END:/* 服务器没有对应的数据 */
                    emit pictureDownload_signal(OK);
                    break;
                case CAMERA_TAKE_PICTURE_DONE:/* 拍照完成命令 */
                    emit takePictureDone_signal();
                    break;
                case CLEAR_SERVER_CACHE_DONE:/* 服务器清除缓存完成 */
                    emit tcpServerCacheClearDone_signal();
                    break;
                case PICTURE_TO_CLIENT_NAME:/* 接收图片名称 */
                {
                    picName = allRecData.mid(0,tcpDataLen - 1);
                    allRecData.remove(0,tcpDataLen - 1);
                    emit appLogMessage_signal("正在接收的图片:" + picName);
                    break;
                }
                case PICTURE_TO_CLIENT_DATA:/* 接收图片数据 */
                {
                    /* 保存图像 */
                    if(!saveDateFileName.isEmpty())
                    {
                        QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片";//
                        QDir *videoDir;
                        videoDir = new QDir(picPath);
                        if(!videoDir->exists())/* 查找是否存在视频文件 */
                        {
                            /* 不存在就创建 */
                            bool ismkdir = videoDir->mkdir(picPath);
                            if(!ismkdir)
                                qDebug() << "创建文件夹失败";
                            else
                                qDebug() << "创建文件夹成功";
                        }
                        delete videoDir;
                        picPath += "/" + saveDateFileName;
                        videoDir = new QDir(picPath);
                        if(!videoDir->exists())/* 查找是否存在视频文件 */
                        {
                            /* 不存在就创建 */
                            bool ismkdir = videoDir->mkdir(picPath);
                            if(!ismkdir)
                                qDebug() << "创建文件夹失败";
                            else
                                qDebug() << "创建文件夹成功";
                        }
                        delete videoDir;
                        videoDir = NULL;

                        picPath += "/" + picName;
                        QFile *RecFile = new QFile(picPath);
                        RecFile->open(QFile::WriteOnly);
                        RecFile->write(allRecData.mid(0,tcpDataLen - 1));
                        RecFile->close();
                        RecFile->deleteLater();
                        delete RecFile;
                        RecFile = NULL;
                        emit appLogMessage_signal("接收完毕");
                        emit pictureData_signal(allRecData.mid(0,tcpDataLen - 1));/* 发送图片到主界面显示 */
                    }
                    allRecData.remove(0,tcpDataLen - 1);
                    break;
                }
                case PICTURE_ERROR:/* 设备那边遇到错误 */
                {
                    emit pictureError_signal();
                    break;
                }
                case ONLINE_CAMERA_DEVICE_ID_TO_CLIENT:
                {
                    onlineDeviceList.append(allRecData.mid(0,tcpDataLen - 1));
                    allRecData.remove(0,tcpDataLen - 1);
                    break;
                }
                case ONLINE_CAMERA_DEVICE_LIST_TO_CLIENT_END:
                {
                    emit onlineDeviceName_singal(onlineDeviceList);
                    onlineDeviceList.clear();
                    break;
                }
                default:
                    qDebug() << allRecData.mid(0,tcpDataLen - 1);
                    break;
            }
            if(allRecData.size() < 4)/* 接收的数据小于能够计算的数据长度字节数4，即不能得到后面的数据长度 */
            {
                return;/* 退出 */
            }
            else
            {
                tcpDataLen = qbyteArrayToUint(allRecData.mid(0,4));/* 得到数据长度 */
                qDebug() << tcpDataLen;

                if(tcpDataLen > allRecData.size())/* 剩余的数据不够数据长度，还没接收完毕 */
                {
                    recNextData = true;
                    return;
                }
                /* 接下来就会进入while的条件判断 */
            }
        }
    }
}
