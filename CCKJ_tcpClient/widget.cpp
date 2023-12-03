#include "widget.h"
#include "ui_widget.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStringListModel>
#include <QThread>
#include <QFileDialog>
#include <QString>
#include <QDir>
#include <QTime>

#include <windows.h>
#include <dbt.h>
#include <stdio.h>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    windowsInit();

    ui->logShowTextEdit->setReadOnly(true);/* 设置不可编辑 */
    connect(this,&Widget::appLogMessage_signal,this,&Widget::on_showLogMessage);
    connect(ui->logShowTextEdit,&QTextEdit::textChanged,this,[=]()
    {
        ui->logShowTextEdit->moveCursor(QTextCursor::End);
    });
    emit appLogMessage_signal("应用启动");

    /*调试时用的默认IP和PORT*/
    ui->TCP_Server_IP_LineEdit->setText("127.0.0.1");
    ui->TCP_Server_Port_LineEdit->setText("8000");
    openEventSetting();


    this->setWindowTitle("透明度采样软件V1.0");
    ui->CloseTCPButton->setEnabled(false);          /*未连接时，关闭连接按键为不可用状态*/

    listmodel = new QStringListModel;
    connect(ui->listView,&QListView::clicked, this, &Widget::on_listViewClicked);

    ui->selectDateEdit->setDateTime(QDateTime::currentDateTime());
    ui->recordTimeEdit->setTime(QTime::currentTime());
    ui->tcpRecordTimeEdit->setTime(QTime::currentTime());

    connect(ui->deviceComboBox,&myComboBox::Clicked_signals,this,&Widget::comBoxClick);
}

Widget::~Widget()
{

    userSetting->setValue("userData/saveTcpIp",ui->TCP_Server_IP_LineEdit->text());
    userSetting->setValue("userData/saveTcpPort",ui->TCP_Server_Port_LineEdit->text());

    delete ui;
}

void Widget::windowsInit()
{
    ui->imageSizeComboBox->addItem("QVGA(320*240)");                  /* 为图片大小添加大小选项 */
    ui->imageSizeComboBox->addItem("VGA(680*480)");
    ui->imageSizeComboBox->addItem("UXGA(1600*1200)");
    ui->imageSizeComboBox->addItem("QSXGA(2560*1920)");
    ui->imageSizeComboBox->setCurrentIndex(3);                        /* 设置当前默认选项 */

    for(int i=0;i<60;i++)
    {
        ui->imageQualityComboBox->addItem(QString::number(i));
    }
    ui->imageQualityComboBox->setCurrentIndex(10);
}
/* 窗口使能 */
void Widget::windowSetEnable()
{
    this->setEnabled(true);
}
/* 窗口失能 */
void Widget::windowSetDisable()
{
    this->setDisabled(true);
}
/* 程序注册表设置 */
void Widget::openEventSetting()
{
    /*读取配置文件*/
    userSetting = new QSettings("CCKJ","userConfig");
    if(userSetting->value("userData/savePicFilePath").isNull())/*判断键值是否存在 */
    {
        savePicFilePath = "";
        emit appLogMessage_signal("默认图片保存路径并未设置");
    }
    else
    {
        savePicFilePath = userSetting->value("userData/savePicFilePath").toString();
        emit appLogMessage_signal("默认设置的图片保存路径为:" + savePicFilePath.toLocal8Bit());
    }

    if(userSetting->value("userData/saveTcpIp").isNull())/*判断键值是否存在 */
    {
        emit appLogMessage_signal("默认服务器IP并未设置");
        userSetting->setValue("userData/saveTcpIp","127.0.0.1");
        return;
    }
    else
    {
        ui->TCP_Server_IP_LineEdit->setText(userSetting->value("userData/saveTcpIp").toString());
    }

    if(userSetting->value("userData/saveTcpPort").isNull())/*判断键值是否存在 */
    {
        emit appLogMessage_signal("默认服务器PORT并未设置");
        userSetting->setValue("userData/saveTcpPort","8000");
    }
    else
    {
        ui->TCP_Server_Port_LineEdit->setText(userSetting->value("userData/saveTcpPort").toString());
    }

}

bool Widget::checkTcpServerIsOK()
{
    if(!isTcpOpen)
    {
        emit appLogMessage_signal("请检查服务器是否连接");
        return false;
    }
    else
    {
        return true;
    }
}
/* unsigned int 转 QByteArray */
QByteArray Widget::uIntToQbyteArray(unsigned int uIntData)
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
unsigned int Widget::qbyteArrayToUint(QByteArray dataBuffer)
{
    unsigned int value = 0;
    for(unsigned char i=0;i<4;i++)
    {
        value |= ((unsigned char)dataBuffer.at(i)<< (i*8));
    }
    return value;
}
/* 发送命令的数据帧 */
QByteArray Widget::setCmdFrameFormat(unsigned int dataLen,unsigned char cmd)
{
    QByteArray tempData;
    tempData = uIntToQbyteArray(dataLen);/* 填充后面的数据长度 */
    tempData.append(cmd);/* cmd */
    return tempData;
}
/* 发送数据帧的数据帧 */
QByteArray Widget::setDataFrameFormat(unsigned int dataLen, unsigned char cmd, QByteArray data)
{
    QByteArray tempData;
    tempData = uIntToQbyteArray(dataLen);/* 填充后面的数据长度 */
    tempData.append(cmd);/* cmd */
    tempData += data;
    return tempData;
}

QByteArray Widget::setUshortFrameFormat(unsigned int dataLen, unsigned char cmd, unsigned short data)
{
    QByteArray tempData;
    tempData = uIntToQbyteArray(dataLen);/* 填充后面的数据长度 */
    tempData.append(cmd);/* cmd */
    tempData.resize(tempData.size()+2);
    tempData[5] = data;
    tempData[6] = data>>8;
    return tempData;
}
/* 设置串口发送一个字符串的帧 */
QByteArray Widget::setSerialPortStringDataFormat(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, QByteArray data)
{
    unsigned char frameDataLen = 0;

    QByteArray frameData;
    /* 发送WIFI名字 */
    frameDataLen = data.size();
    frameData.resize(frameDataLen + 4 + 2);
    frameData[0] = frameHead;                               /* 帧头 */
    frameData[1] = frameAddress;                            /* 帧地址 */
    frameData[2] = frameID;                                 /* 帧ID */
    frameData[3] = frameDataLen;                            /* 帧数据长度 */

    for(int i=0;i<frameDataLen;i++)
    {
        frameData[4+i] = data.at(i);
    }

    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;
    for(unsigned char i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    frameData[frameDataLen + 4] = sumcheck;
    frameData[frameDataLen + 5] = addcheck;
    return frameData;
}
/* 设置串口发送一个ushort的帧 */
QByteArray Widget::setSerialPortUshortDataFormat(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, unsigned short data)
{
    unsigned char frameDataLen = 0;

    QByteArray frameData;
    /* 发送WIFI名字 */
    frameDataLen = 2;
    frameData.resize(frameDataLen + 4 + 2);
    frameData[0] = frameHead;                               /* 帧头 */
    frameData[1] = frameAddress;                            /* 帧地址 */
    frameData[2] = frameID;                                 /* 帧ID */
    frameData[3] = frameDataLen;                            /* 帧数据长度 */

    frameData[4] = data;
    frameData[5] = data>>8;

    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;
    for(unsigned char i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    frameData[frameDataLen + 4] = sumcheck;
    frameData[frameDataLen + 5] = addcheck;
    return frameData;
}
/*打开TCP连接按键*/
void Widget::on_ConnectTCPtButton_clicked()
{
    if(ui->TCP_Server_IP_LineEdit->text() == "" || ui->TCP_Server_Port_LineEdit->text() == "")/*服务器IP和端口号输入为空不执行*/
     {
        QMessageBox::information(this,"错误","请确认输入的服务器和端口号！");
        return;
    }
    if(isTcpThreadAlive == true)
    {
        emit appLogMessage_signal("TCP正在连接,请勿重复点击!");
        return;
    }

    emit appLogMessage_signal("开始连接TCP服务器!");
    /*得到服务器端的IP地址，以及服务器端的端口号*/
    QString TCP_Server_IP = ui->TCP_Server_IP_LineEdit->text();/*获取文本框中输入的服务器IP地址*/
    QString TCP_Server_Port = ui->TCP_Server_Port_LineEdit->text();/*获取文本框中输入的服务器端口*/

    mTcpTask = new tcpTask;
    mTcpThread = new QThread;
    mTcpTask->moveToThread(mTcpThread); /* 创建TCP通信的子线程 */
    mTcpThread->start();
    connect(mTcpTask,&tcpTask::tcpConnected_signal,this,&Widget::on_tcpConnectedSuccessfull,Qt::UniqueConnection);   /* 子线程tcp连接成功通知主线程 */
    connect(mTcpTask,&tcpTask::tcpDisconnected_signal,this,&Widget::on_tcpDisconnected,Qt::UniqueConnection);        /* 子线程tcp断开连接通知主线程 */
    connect(mTcpTask,&tcpTask::pictureData_signal,this,&Widget::on_showPicture,Qt::UniqueConnection);                /* 子线程接收到发送到主线程显示 */
    connect(mTcpTask,&tcpTask::pictureDownload_signal,this,&Widget::on_readPictureDownLoadState,Qt::UniqueConnection);/* 子线程告知主线程图片下载状态 */
    connect(mTcpTask,&tcpTask::takePictureDone_signal,this,&Widget::on_takePicFinish,Qt::UniqueConnection);          /* 子线程告知主线程拍照完成 */
    connect(mTcpTask,&tcpTask::tcpServerCacheClearDone_signal,this,&Widget::on_tcpServerCacheClearDone,Qt::UniqueConnection);/* 子线程通知主线程服务器的缓存清除完毕 */
    connect(mTcpTask,&tcpTask::appLogMessage_signal,this,&Widget::on_showLogMessage,Qt::UniqueConnection);           /* 子线程输出log信息到主线程 */
    connect(mTcpTask,&tcpTask::pictureError_signal,this,&Widget::on_takePictureError,Qt::UniqueConnection);           /* 子线程发送拍摄图像错误信号到主线程 */
    connect(mTcpTask,&tcpTask::onlineDeviceName_singal,this,&Widget::on_showDeviceId,Qt::UniqueConnection);             /* 子线程发送的在线的摄像头列表 */
    connect(mTcpTask,&tcpTask::cameraBindOK_signal,this,&Widget::on_cameraBindOK,Qt::UniqueConnection);                 /* 摄像头绑定成功 */
    connect(mTcpTask,&tcpTask::cameraBindFail_signal,this,&Widget::on_cameraBindFail,Qt::UniqueConnection);             /* 摄像头绑定失败 */
    connect(mTcpTask,&tcpTask::cameraOpenMotoSuccess_signal,this,&Widget::on_openMotoSuccess,Qt::UniqueConnection);     /* 摄像头打开电机成功 */

    connect(this,&Widget::tcpConnectToHost_signal,mTcpTask,&tcpTask::startTcpConnect,Qt::UniqueConnection);          /* 主线程通知子线程TCP连接服务器 */
    connect(this,&Widget::sendTcpData_signal,mTcpTask,&tcpTask::sendTcpData,Qt::UniqueConnection);                   /* 主线程通知子线程TCP发送数据 */
    emit tcpConnectToHost_signal(TCP_Server_IP,TCP_Server_Port);                                                         /* 建立TCP连接 */

    tcpHeartBeatTimer = new QTimer;
    tcpHeartBeatTimer->start(500);
    connect(tcpHeartBeatTimer,&QTimer::timeout,this,&Widget::on_keepTcpHeartBeat,Qt::UniqueConnection);              /* 保持TCP心跳 */
    isTcpThreadAlive = true;
}
/*关闭TCP连接按键*/
void Widget::on_CloseTCPButton_clicked()
{
    mTcpThread->quit();
    mTcpThread->wait();
    delete mTcpTask;
    delete mTcpThread;

    tcpHeartBeatTimer->stop();
    delete tcpHeartBeatTimer;

    isTcpThreadAlive = false;
    isTcpOpen = false;

    ui->ConnectTCPtButton->setEnabled(true);        /*断开连接，打开连接按键为可用状态*/
    ui->CloseTCPButton->setEnabled(false);          /*断开连接，关闭连接按键为不可用状态*/
    emit appLogMessage_signal("已经断开TCP服务器");
}
/* 下载图片按键 */
void Widget::on_searchPictureButton_clicked()
{
    if(!checkTcpServerIsOK())return;
    if(!isTcpBackFinish)
    {
        emit appLogMessage_signal("任务正在进行，请稍后重试~");
        return;
    }
    if(isBindFinish == false)
    {
        emit appLogMessage_signal("请绑定设备!");
        return;
    }
    emit appLogMessage_signal("开始下载图片");
    sendExistPictureFileToServer();/* 向服务器发送本地已经存在的文件 */
    mTcpTask->setSaveDateFileName(ui->selectDateEdit->date().toString("yyyy-MM-dd"));/* 设置保存的文件名称，以读取到日历控件的日期命名 */
    /* 向服务器发送下载图片的命令,携带指定的日期，代表下载指定日期的图片数据 */
    emit sendTcpData_signal(
                setDataFrameFormat(1 + ui->selectDateEdit->date().toString("yyyy-MM-dd").toLocal8Bit().size(),/* 数据总长 */
                                   (unsigned char)DOWNLOAD_PICTURE, /* 数据帧功能 */
                                   ui->selectDateEdit->date().toString("yyyy-MM-dd").toLocal8Bit()));/* 数据内容 */
    ui->searchPictureButton->setDisabled(true);
    isTcpBackFinish = false;
}
/* TCP连接成功响应槽函数 */
void Widget::on_tcpConnectedSuccessfull()
{
    ui->ConnectTCPtButton->setEnabled(false);       /*连接成功，打开连接按键为不可用状态*/
    ui->CloseTCPButton->setEnabled(true);           /*连接成功，关闭连接按键为可用状态*/
    emit appLogMessage_signal("服务器连接成功");
    isTcpOpen = true;
}
/* TCP断开连接响应槽函数 */
void Widget::on_tcpDisconnected()
{
    on_CloseTCPButton_clicked();
}
/* 显示图片的槽函数 */
void Widget::on_showPicture(QByteArray picData)
{
    /* 显示图片 */
    QImage* pictureImg = new QImage;
    bool isLoadPictureData = true;
    isLoadPictureData = pictureImg->loadFromData(picData);
    if(isLoadPictureData)
    {
        /*按照pictureLabel的大小缩放显示图片*/
        ui->pictureLabel->setPixmap(QPixmap::fromImage(pictureImg->scaled(ui->pictureLabel->size(),
                                                                          Qt::KeepAspectRatio,Qt::SmoothTransformation)));
        delete pictureImg;
    }
    else
    {
        delete pictureImg;
        emit appLogMessage_signal("可能损坏的图片");
        return;
    }
}
/* 文件夹内文件名列表显示的槽函数 */
void Widget::on_listViewClicked(const QModelIndex &index)
{
    qDebug() << "选择的图片是:" << index.data().toString();
    QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraDevice + "/" +
                    ui->selectDateEdit->date().toString("yyyy-MM-dd").toLocal8Bit() + "/" + index.data().toString();
    QByteArray picData;
    QFile file(picPath);
    qDebug() << picPath;
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
    on_showPicture(picData);
}
/* 拍照按键 */
void Widget::on_takePictureButton_clicked()
{
    if(!checkTcpServerIsOK())return;
    if(!isTcpBackFinish)
    {
        emit appLogMessage_signal("任务正在进行，请稍后重试~");
        return;
    }
    if(isBindFinish == false)
    {
        emit appLogMessage_signal("请绑定设备!");
        return;
    }
    emit appLogMessage_signal("设备开始拍照");
    emit sendTcpData_signal(setCmdFrameFormat(1,(unsigned char)TAKE_PICTURE));
    ui->takePictureButton->setDisabled(true);/* 设置拍照按键不可用 */
    isTcpBackFinish = false;
}
/* TCP线程图片下载状态处理 */
void Widget::on_readPictureDownLoadState(tcpTask::picDownloadState state)
{
    switch (state)
    {
        case tcpTask::OK:
        {
            /* 将程序下的图片文件复制到用户指定的文件夹下 */
            copyDirectoryFiles(QCoreApplication::applicationDirPath() + "/" + "照片",savePicFilePath + "/" + "照片",true);
            /* 遍历存储的所有图片文件 */
            QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraDevice + "/" +
                                    ui->selectDateEdit->date().toString("yyyy-MM-dd").toLocal8Bit();
            QDir dir(picPath);
            QStringList filename ;
            filename << "*.jpeg";//可叠加，可使用通配符筛选
            QStringList fileResults;
            fileResults = dir.entryList(filename,QDir::Files | QDir::Readable,QDir::Name);
            if(fileResults.size() == 0)
            {
                QMessageBox::warning(this, "警告!", "没有找到下载的图片!");
                return;
            }

            QStringList list;
            for(int i=0;i<fileResults.size();i++)
            {
                list.append(fileResults.at(i));
            }

            listmodel->setStringList(list);
            ui->listView->setModel(listmodel);
            emit appLogMessage_signal("图片下载完成");
            ui->searchPictureButton->setDisabled(false);
            isTcpBackFinish = true;
            break;
        }
        case tcpTask::PICTURE_EMPTY:
        {
            QMessageBox::information(this,"提示","没有可下载资源");
            emit appLogMessage_signal("没有可下载资源");
            ui->searchPictureButton->setDisabled(false);
            isTcpBackFinish = true;
            break;
        }
        default:
            break;
    }
}
/* 保存路径按钮 */
void Widget::on_setSavePathButton_clicked()
{
    QString selectDir = QFileDialog::getExistingDirectory();
    qDebug() << selectDir;
    if (selectDir.isEmpty())
    {
        QMessageBox::warning(this, "警告!", "路径选择错误!");
        emit appLogMessage_signal("路径选择错误");
    }
    else
    {
        savePicFilePath = selectDir;
        userSetting->beginGroup("userData");
        userSetting->setValue("savePicFilePath",savePicFilePath);
        userSetting->endGroup();
        emit appLogMessage_signal("选择保存的路径为:" + savePicFilePath.toLocal8Bit());
    }
}
/* 发送TCP心跳包 */
void Widget::on_keepTcpHeartBeat()
{
    if(!isTcpOpen)return;
    emit sendTcpData_signal(setCmdFrameFormat(1,(unsigned char)HERAT_BEAT_PACK));
}
/* 拷贝文件夹 */
bool Widget::copyDirectoryFiles(const QString fromDir, const QString toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists())
    {    /**< 如果目标目录不存在，则进行创建 */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList)
    {
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir())
        {    /**< 当为目录时，递归的进行copy */
            if(!copyDirectoryFiles(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()),
                coverFileIfExist))
                return false;
        }
        else
        {            /**< 当允许覆盖操作时，将旧文件进行删除操作 */
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName()))
            {
                targetDir.remove(fileInfo.fileName());
            }

            /// 进行文件copy
            if(!QFile::copy(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName())))
            {
                    return false;
            }
        }
    }
    return true;
}
void Widget::SerialPortSearch()
{
    QStringList SerialPortNameList;/*保存搜索到的串口，存入列表中*/
    ui->deviceComboBox->clear();
    foreach (const QSerialPortInfo &SerialPortInfo, QSerialPortInfo::availablePorts()) /*遍历可使用的串口*/
    {
        SerialPortNameList.append(SerialPortInfo.portName());/*把搜索到的串口存入列表中*/
        //qDebug() << SerialPortInfo.portName() + " " + SerialPortInfo.description();
    }
    ui->deviceComboBox->addItems(SerialPortNameList);/*将搜索到的串口显示到UI界面上*/
}
void Widget::comBoxClick()
{
    SerialPortSearch();
    if(ui->devicePushButton->text() == "搜索设备")
    {
        ui->devicePushButton->setText("打开设备");
        return;
    }
}
/* 将本地存在的文件名称发送给服务器，避免重复下载 */
void Widget::sendExistPictureFileToServer()
{
    QString picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraDevice + "/" +
                            ui->selectDateEdit->date().toString("yyyy-MM-dd").toLocal8Bit();
    QDir dir(picPath);
    QStringList filename ;
    filename << "*.jpeg";//可叠加，可使用通配符筛选
    QStringList fileResults;
    fileResults = dir.entryList(filename,QDir::Files | QDir::Readable,QDir::Name);
    if(fileResults.size() == 0)
    {
        return;
    }
    emit appLogMessage_signal("检测到本地有存在的图片数据");
    emit appLogMessage_signal("请注意，这些图片将不会重复下载");
    QStringList list;
    for(int i=0;i<fileResults.size();i++)
    {
        list.append(fileResults.at(i));
        qDebug() <<fileResults.at(i);
        /* 将已经下载下来的图片发送给服务器，来避免重复下载图片 */
        emit sendTcpData_signal(
                    setDataFrameFormat(1 + fileResults.at(i).toLocal8Bit().size(),/* 数据总长 */
                                       (unsigned char)CLIENT_PICTURE_FILE_NAME, /* 数据帧功能 */
                                       fileResults.at(i).toLocal8Bit()));/* 数据内容 */
        emit appLogMessage_signal("本地存在的图片:" + fileResults.at(i).toLocal8Bit());
    }
}
/* 清除本地缓存按钮 */
void Widget::on_clearLocalCahePathButton_clicked()
{
    QString picName = QCoreApplication::applicationDirPath() + "/" + "照片";
    QDir dir;
    dir.setPath(picName);
    dir.removeRecursively();
    if(savePicFilePath.isEmpty())return;
    dir.setPath(savePicFilePath + "/" + "照片");
    dir.removeRecursively();
    QMessageBox::information(this, "提示", "清除成功！");
    emit appLogMessage_signal("路径为:" + savePicFilePath.toLocal8Bit() + "的图片清除完成");
}
/* 清除服务器缓存按钮 */
void Widget::on_clearServerCahePathButton_clicked()
{
    if(!checkTcpServerIsOK())return;
    if(!isTcpBackFinish)
    {
        emit appLogMessage_signal("任务正在进行，请稍后重试~");
        return;
    }
    emit appLogMessage_signal("开始清除服务器缓存");
    emit sendTcpData_signal(setCmdFrameFormat(1,(unsigned char)CLEAR_SERVER_CACHE));   
    isTcpBackFinish = false;
}
/* 拍照完成响应槽函数 */
void Widget::on_takePicFinish()
{
    ui->takePictureButton->setDisabled(false);
    emit appLogMessage_signal("拍照完成");
    isTcpBackFinish = true;
    on_searchPictureButton_clicked();/* 把拍摄的照片下载下来 */
}
/* TCP服务器缓存清除完成响应函数 */
void Widget::on_tcpServerCacheClearDone()
{
    QMessageBox::information(this, "提示", "服务器缓存清除成功！");
    emit appLogMessage_signal("服务器缓存已经清除");
    isTcpBackFinish = true;
}
/* 显示log信息到主界面UI显示槽函数 */
void Widget::on_showLogMessage(QByteArray logMessage)
{
    ui->logShowTextEdit->append(logMessage);
}
bool Widget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG* msg = reinterpret_cast<MSG*>(message);
       if(msg->message == WM_DEVICECHANGE)                // 通知应用程序设备或计算机的硬件配置发生更改。
       {
           PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
           switch (msg->wParam)
           {
           case DBT_DEVICEARRIVAL:             // 插入
           {
               if (lpdb->dbch_devicetype == DBT_DEVTYP_PORT)           // 设备类型为串口
               {
                   if(ui->devicePushButton->text() != "关闭设备")
                   {
                       emit appLogMessage_signal("检测有设备插入");
                       SerialPortSearch();/*更新串口*/
                       ui->devicePushButton->setText("打开设备");
                   }
               }
               break;
           }
           case DBT_DEVICEREMOVECOMPLETE:      // 拔出
           {
               if (lpdb->dbch_devicetype == DBT_DEVTYP_PORT)           // 设备类型为串口
               {
                   PDEV_BROADCAST_PORT lpdbv = (PDEV_BROADCAST_PORT)lpdb;
                   QString strName = QString::fromWCharArray(lpdbv->dbcp_name);  //拔出的串口名
                   if(ui->deviceComboBox->currentText() == strName && ui->devicePushButton->text() == "关闭设备")
                   {
                       serialPortTask->SerialPortSetState(false);
                       on_devicePushButton_clicked();
                       emit appLogMessage_signal("检测有设备拔出");
                   }
                   SerialPortSearch();/*更新串口*/
               }
               break;
           }
           default:
               break;
           }
       }
       return false;
}
/* 搜索设备按钮 */
void Widget::on_devicePushButton_clicked()
{
    int PortBandRate = 115200;
    QSerialPort::DataBits PortDataBits = QSerialPort::Data8;                /*数据位，默认Data8*/
    QSerialPort::Parity PortParityBits = QSerialPort::NoParity;             /*校验位，默认NoParity*/
    QSerialPort::StopBits PortStopBits = QSerialPort::OneStop;              /*停止位，默认OneStop*/
    QSerialPort::FlowControl PortFlowControl = QSerialPort::NoFlowControl;  /*串口控制，默认NoFlowControl*/

    if(ui->devicePushButton->text() == "搜索设备")
    {
        SerialPortSearch();
        emit appLogMessage_signal("搜索设备完成");
        ui->devicePushButton->setText("打开设备");
        return;
    }

    if(ui->devicePushButton->text() == "打开设备")
    {
        serialPortThread = new QThread;
        serialPortTask = new SerialPortThread;
        serialPortTask->moveToThread(serialPortThread);
        serialPortThread->start();

        connect(this,&Widget::openSerialPort_signal,serialPortTask,&SerialPortThread::OpenSerialPort,Qt::UniqueConnection);             /* 连接打开串口设备的槽函数 */
        connect(this,&Widget::closeSerialPort_signal,serialPortTask,&SerialPortThread::SerialPortClose,Qt::UniqueConnection);           /* 连接串口关闭处理槽函数 */
        connect(this,&Widget::sendSerialPortData,serialPortTask,&SerialPortThread::DataSend,Qt::UniqueConnection);                      /* 连接串口发送数据的槽函数 */
        connect(serialPortTask,&SerialPortThread::OpenSerialPortOK_signal,this,&Widget::on_openSerialPortState,Qt::UniqueConnection);   /* 连接串口设备连接状态的处理槽函数 */
        connect(serialPortTask,&SerialPortThread::appLogMessage_signal,this,&Widget::on_showLogMessage,Qt::UniqueConnection);           /* 子线程输出log信息到主线程 */

        QString portName = ui->deviceComboBox->currentText();
        if(portName.isEmpty() == true)
        {
            return;
        }

        emit openSerialPort_signal(  portName,
                                     PortBandRate,
                                     PortDataBits,
                                     PortParityBits,
                                     PortStopBits,
                                     PortFlowControl);
        return;
    }

    if(ui->devicePushButton->text() == "关闭设备")
    {
        serialPortTask->SerialPortClose();
        ui->deviceComboBox->clear();
        ui->devicePushButton->setText("搜索设备");
        ui->deviceComboBox->setDisabled(false);

        serialPortThread->exit(0);
        serialPortThread->wait();
        serialPortThread->deleteLater();

        serialPortTask->deleteLater();

        return;
    }
}
/* 判断串口设备打开是否成功 */
void Widget::on_openSerialPortState(bool state)
{
    if(state)
    {
        ui->devicePushButton->setText("关闭设备");
        ui->deviceComboBox->setDisabled(true);
        emit appLogMessage_signal("设备连接成功");
    }
    else
    {
        serialPortThread->exit(0);
        serialPortThread->wait();
        serialPortThread->deleteLater();

        serialPortTask->deleteLater();
        emit appLogMessage_signal("设备连接失败");
    }
}

void Widget::on_writeDevicePushButton_clicked()
{
    QByteArray wifiName = ui->wifiNameLineEdit->text().toLocal8Bit();
    QByteArray wifiPassWord = ui->wifiPasswordLineEdit->text().toLocal8Bit();

    QByteArray serverIp = ui->serverIpLineEdit->text().toLocal8Bit();
    QByteArray serverPort = ui->serverPortLineEdit->text().toLocal8Bit();

    QByteArray deviceId = ui->deviceIdSetLineEdit->text().toLocal8Bit();

    unsigned short imageSize = 0;
    unsigned short imageQuality = 0;
    unsigned short ledFlashBrightness = 0;
    unsigned short takePictureDelayTime = 0;

    switch (ui->imageSizeComboBox->currentIndex())
    {
        case 0:
            imageSize = SerialPortThread::FRAMESIZE_QVGA;
            break;
        case 1:
            imageSize = SerialPortThread::FRAMESIZE_VGA;
            break;
        case 2:
            imageSize = SerialPortThread::FRAMESIZE_UXGA;
            break;
        case 3:
            imageSize = SerialPortThread::FRAMESIZE_QSXGA;
            break;
        default:
            break;
    }

    imageQuality = ui->imageQualityComboBox->currentIndex();
    ledFlashBrightness = ui->ledBrightnessLineEdit->text().toUShort();
    takePictureDelayTime = ui->takePictureDelayTimeLineEdit->text().toUShort();

    if(wifiName.isEmpty()){emit appLogMessage_signal("WIFI名字为空，请检查");return;}
    if(wifiPassWord.isEmpty()){emit appLogMessage_signal("WIFI密码为空，请检查");return;}
    if(serverIp.isEmpty()){emit appLogMessage_signal("服务器地址为空，请检查");return;}
    if(serverPort.isEmpty()){emit appLogMessage_signal("服务器端口为空，请检查");return;}
    if(deviceId.isEmpty()){emit appLogMessage_signal("设备ID为空，请检查");return;}

    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CDM_WIFI_NAME,wifiName));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_WIFI_PASSWORD,wifiPassWord));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SERVER_IP,serverIp));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SERVER_PORT,serverPort));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_DEVICE_ID,deviceId));

//    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_PICTURE_SIZE,imageSize));
//    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_PICTURE_QUALITY,imageQuality));
//    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_LIED_BRIGHTNESS,ledFlashBrightness));
//    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_TAKE_PICTURE_DELAY_TIME,takePictureDelayTime));

//    if(ui->scheduledTimeCheckBox->isChecked())/* 间隔定时模式 */
//    {
//        QByteArray dateTime;
//        dateTime = ui->scheduledTimeLineEdit->text().toLocal8Bit();
//        if(dateTime.isEmpty())
//        {
//            emit appLogMessage_signal("请输入正确的间隔时间!");
//            return;
//        }
//        if(dateTime.toUShort() > 24 * 60)
//        {
//            emit appLogMessage_signal("超过最大可设定时间，请检查重新填写");
//            return;
//        }
//        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_DELAY_TIME,dateTime.toUShort()));/* 发送间隔定时时间 */
//        emit appLogMessage_signal("间隔定时的定时时间已发送!");
//    }
//    else if(ui->recordCheckBox->isChecked()) /* 固定定时 */
//    {
//        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_DELAY_TIME,0));
//        QByteArray dateTime;
//        for(int i=0;i<ui->addRecordTimeComboBox->count();i++)
//        {
//            dateTime = ui->addRecordTimeComboBox->itemText(i).toLocal8Bit();
//            emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_RECORD_TIME,dateTime));
//        }
//        emit appLogMessage_signal("固定定时的定时时间已发送!");
//    }
//    else
//    {
//        emit appLogMessage_signal("请选择定时方式!");
//    }
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_PARA_END,0));/* 配置信息发送完毕 */
}

void Widget::on_takePictureError()
{
    ui->takePictureButton->setDisabled(false);
    isTcpBackFinish = true;
    emit appLogMessage_signal("设备获取图像错误，超过最大重试次数或接收超时！");
}

/* 搜索在线设备的按键的槽函数 */
void Widget::on_searchDeviceButton_clicked()
{
    if(!checkTcpServerIsOK()){emit appLogMessage_signal("请检查服务器是否连接！");return;};

    if(ui->searchDeviceButton->text() == "搜索设备")
    {
        emit sendTcpData_signal(setCmdFrameFormat(1,(unsigned char)GET_ONLINE_DEVICE));
        ui->searchDeviceButton->setText("连接设备");
    }
    else if(ui->searchDeviceButton->text() == "连接设备")
    {
        if(ui->deviceOnlineComboBox->currentText() == "")
        {
            emit appLogMessage_signal("请检查设备ID是否正确！");
            ui->searchDeviceButton->setText("搜索设备");
            return;
        }
        emit sendTcpData_signal(setDataFrameFormat( 1+ui->deviceOnlineComboBox->currentText().toLocal8Bit().size(),
                                                    (unsigned char)CLIENT_BIND_CAMERA,
                                                    ui->deviceOnlineComboBox->currentText().toLocal8Bit()));
    }
    if(ui->searchDeviceButton->text() == "断开连接")
    {
        emit sendTcpData_signal(setDataFrameFormat( 1+ui->deviceOnlineComboBox->currentText().toLocal8Bit().size(),
                                                   (unsigned char)CLIENT_DISBIND_CAMERA,
                                                   ui->deviceOnlineComboBox->currentText().toLocal8Bit()));
        bindCameraDevice = "";
        isBindFinish = false;
        ui->searchDeviceButton->setText("搜索设备");
    }
}

void Widget::on_showDeviceId(QList<QByteArray> deviceIdList)
{
    ui->deviceOnlineComboBox->clear();
    for(int i=0;i<deviceIdList.count();i++)
    {
        ui->deviceOnlineComboBox->addItem(deviceIdList.at(i));
        qDebug()<< "设备ID" <<deviceIdList.at(i);
    }
}

void Widget::on_cameraBindOK()
{
    ui->searchDeviceButton->setText("断开连接");
    bindCameraDevice = ui->deviceOnlineComboBox->currentText().toLocal8Bit();
    mTcpTask->setBindCameraDevice(bindCameraDevice);
    isBindFinish = true;
}

void Widget::on_cameraBindFail()
{
    ui->searchDeviceButton->setText("搜索设备");
    bindCameraDevice = "";
    isBindFinish = false;
}
/* 打开水泵按键 */
void Widget::on_motoControlPushButton_clicked()
{
    if(!checkTcpServerIsOK())return;
    if(isBindFinish == false)
    {
        emit appLogMessage_signal("请绑定设备!");
        return;
    }
    emit sendTcpData_signal(setCmdFrameFormat(1,(unsigned char)OPEN_MOTO_CMD));
}

void Widget::on_openMotoSuccess()
{
    emit appLogMessage_signal("电机已经启动");
}

void Widget::on_readDeviceInfoPushButton_clicked()
{
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SHOW_DEVICE_INFO,0));
}

void Widget::on_resetDevicePushButton_clicked()
{
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_RESET_DEVICE,0));
}

void Widget::on_addRecordTimePushButton_clicked()
{
    QByteArray dateTime;
    dateTime.resize(5);
    sprintf(dateTime.data(),"%02d:%02d",ui->recordTimeEdit->time().hour(),ui->recordTimeEdit->time().minute());
    ui->addRecordTimeComboBox->addItem(QString::fromLocal8Bit(dateTime));
}


void Widget::on_deleteRecordTimePushButton_clicked()
{
    ui->addRecordTimeComboBox->removeItem(ui->addRecordTimeComboBox->currentIndex());
}

/* 通过TCP设置定时参数按钮 */
void Widget::on_tcpSetRecordPushButton_clicked()
{
    if(ui->tcpScheduledTimeCheckBox->isChecked())/* 间隔定时模式 */
    {
        QByteArray dateTime;
        dateTime = ui->tcpScheduledTimeLineEdit->text().toLocal8Bit();
        if(dateTime.isEmpty())
        {
            emit appLogMessage_signal("请输入正确的间隔时间!");
            return;
        }
        if(dateTime.toUShort() > 24 * 60)
        {
            emit appLogMessage_signal("超过最大可设定时间，请检查重新填写");
            return;
        }
        emit sendTcpData_signal(setUshortFrameFormat( 1 + 2,(unsigned char)SET_SCHEDULED_TIME_CMD,dateTime.toUShort()));/* 发送间隔定时时间 */
        emit appLogMessage_signal("间隔定时的定时时间已发送!");
    }
    else if(ui->tcpRecordCheckBox->isChecked()) /* 固定定时 */
    {
        emit sendTcpData_signal(setUshortFrameFormat( 1 + 2,(unsigned char)SET_SCHEDULED_TIME_CMD,0));/* 发送间隔定时时间，时间为0分钟 */
        QByteArray dateTime;
        for(int i=0;i<ui->tcpAddRecordTimeComboBox->count();i++)
        {
            dateTime = ui->tcpAddRecordTimeComboBox->itemText(i).toLocal8Bit();
            emit sendTcpData_signal(setDataFrameFormat( 1 + dateTime.size(),
                                                        (unsigned char)SET_RECORD_TIME_CMD,
                                                        dateTime));/* 发送定时的时间 */
        }

        emit sendTcpData_signal(setCmdFrameFormat(1,(unsigned char)SET_RECORD_TIME_DONE_CMD));/* 发送定时的时间 */
        emit appLogMessage_signal("固定定时的定时时间已发送!");
    }
}

/* 添加一组定时按键 */
void Widget::on_tcpAddRecordTimePushButton_clicked()
{
    QByteArray dateTime;
    dateTime.resize(5);
    sprintf(dateTime.data(),"%02d:%02d",ui->tcpRecordTimeEdit->time().hour(),ui->tcpRecordTimeEdit->time().minute());
    ui->tcpAddRecordTimeComboBox->addItem(QString::fromLocal8Bit(dateTime));
}

/* 删除一组定时 */
void Widget::on_tcpDeleteRecordTimePushButton_clicked()
{
    ui->tcpAddRecordTimeComboBox->removeItem(ui->tcpAddRecordTimeComboBox->currentIndex());
}

void Widget::on_tcpRecordCheckBox_clicked()
{
    if(ui->tcpScheduledTimeCheckBox->isChecked())
    {
        ui->tcpScheduledTimeCheckBox->setCheckState(Qt::Unchecked);
    }
}


void Widget::on_tcpScheduledTimeCheckBox_clicked()
{
    if(ui->tcpRecordCheckBox->isChecked())
    {
        ui->tcpRecordCheckBox->setCheckState(Qt::Unchecked);
    }
}

void Widget::on_recordCheckBox_clicked()
{
    if(ui->scheduledTimeCheckBox->isChecked())
    {
        ui->scheduledTimeCheckBox->setCheckState(Qt::Unchecked);
    }
}

void Widget::on_scheduledTimeCheckBox_clicked()
{
    if(ui->recordCheckBox->isChecked())
    {
        ui->recordCheckBox->setCheckState(Qt::Unchecked);
    }
}

