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
#include <QMenu>

#include <windows.h>
#include <dbt.h>
#include <stdio.h>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    windowsInit();

    connect(this,&Widget::appLogMessage_signal,this,&Widget::on_showLogMessage);
    connect(ui->logShowTextEdit,&QTextEdit::textChanged,this,[=]()
    {
        ui->logShowTextEdit->moveCursor(QTextCursor::End);
    });

    /*调试时用的默认IP和PORT*/
    ui->TCP_Server_IP_LineEdit->setText("127.0.0.1");
    ui->TCP_Server_Port_LineEdit->setText("8000");

    openEventSetting();                             /* 读取注册表保存的信息 */

    qDebug()<<"main threadId: "<<QThread::currentThreadId();
}

Widget::~Widget()
{
    /* 保存窗口上的一些信息到注册表内 */
    userSetting->setValue("userData/saveTcpIp",ui->TCP_Server_IP_LineEdit->text());
    userSetting->setValue("userData/saveTcpPort",ui->TCP_Server_Port_LineEdit->text());

    delete ui;
}

void Widget::windowsInit()
{
    this->setWindowTitle("透明度采样软件V1.0");

    hideTcpSetScheduledTimingGroup();

    hideTcpSetRecordTimingGroup();

    hideSetScheduledTimingGroup();

    hideSetRecordTimingGroup();

    ui->logShowTextEdit->setReadOnly(true);                             /* 图片显示label设置不可编辑 */

    ui->imageSizeComboBox->addItem("QVGA(320*240)");                    /* 为图片大小添加大小选项 */
    ui->imageSizeComboBox->addItem("VGA(680*480)");
    ui->imageSizeComboBox->addItem("UXGA(1600*1200)");
    ui->imageSizeComboBox->addItem("QSXGA(2592*1940)");
    ui->imageSizeComboBox->setCurrentIndex(3);                          /* 设置当前默认选项 */

    for(int i=0;i<101;i++)                                               /* 图片质量的选择窗口 */
    {
        ui->imageQualityComboBox->addItem(QString::number(i));
    }
    ui->imageQualityComboBox->setCurrentIndex(100);

    ui->CloseTCPButton->setEnabled(false);                              /* 未连接时，关闭连接按键为不可用状态 */
    ui->disConnectDeviceButton->setEnabled(false);                      /* 断开连接相机按键不可用 */
    listmodel = new QStringListModel;                                   /* 绑定文件夹显示的信号和槽 */
    connect(ui->listView,&QListView::clicked, this, &Widget::on_listViewClicked);

    connect(ui->deviceComboBox,&myComboBox::Clicked_signals,this,&Widget::comBoxClick);

    /* 设置日期选择框 */

    uint32_t currentYear = QDateTime::currentDateTime().date().year();
    uint32_t currentMonth = QDateTime::currentDateTime().date().month();
    uint32_t currentDay = QDateTime::currentDateTime().date().day();
    uint32_t currentHour = QDateTime::currentDateTime().time().hour();
    uint32_t currentMinute = QDateTime::currentDateTime().time().minute();
    qDebug() << currentYear << currentMonth <<currentDay;

    ui->tcpDownLoadYearComboBox->clear();
    for(uint32_t i=currentYear-10;i<currentYear+10;i++)
    {
        ui->tcpDownLoadYearComboBox->addItem(QString::number(i));
    }
    ui->tcpDownLoadYearComboBox->setCurrentIndex(10);

    ui->tcpDownLoadMouthComboBox->clear();
    for(uint32_t i=0;i<12;i++)
    {
        ui->tcpDownLoadMouthComboBox->addItem(QString::number(i+1));
    }
    ui->tcpDownLoadMouthComboBox->setCurrentIndex(currentMonth-1);

    ui->tcpDownLoadDayComboBox->clear();
    for(uint32_t i=0;i<31;i++)
    {
        ui->tcpDownLoadDayComboBox->addItem(QString::number(i+1));
    }
    ui->tcpDownLoadDayComboBox->setCurrentIndex(currentDay-1);

    ui->hourTimingComboBox->clear();
    for(uint32_t i=0;i<24;i++)
    {
        ui->hourTimingComboBox->addItem(QString::number(i));
    }
    ui->hourTimingComboBox->setCurrentIndex(currentHour);

    ui->minuteTimingComboBox->clear();
    for(uint32_t i=0;i<60;i++)
    {
        ui->minuteTimingComboBox->addItem(QString::number(i));
    }
    ui->minuteTimingComboBox->setCurrentIndex(currentMinute);

    ui->tcpHourTimingComboBox->clear();
    for(uint32_t i=0;i<24;i++)
    {
        ui->tcpHourTimingComboBox->addItem(QString::number(i));
    }
    ui->tcpHourTimingComboBox->setCurrentIndex(currentHour);

    ui->tcpMinuteTimingComboBox->clear();
    for(uint32_t i=0;i<60;i++)
    {
        ui->tcpMinuteTimingComboBox->addItem(QString::number(i));
    }
    ui->tcpMinuteTimingComboBox->setCurrentIndex(currentMinute);

    ui->pictureDirectionComboBox->addItem("正常");
    ui->pictureDirectionComboBox->addItem("镜像");
    ui->pictureDirectionComboBox->addItem("翻转");
    ui->pictureDirectionComboBox->addItem("镜像翻转");

    ui->binDownLoadPushButton->hide();          /* 下载固件按键隐藏 */
    ui->takePictureTestModePushButton->hide();
    ui->openMotoTestModePushButton->hide();     /* 测试模式下的相关按键隐藏 */

    //获取所有的控件，为窗口缩放做准备
    m_Widget = this->findChildren<QWidget*>(QString(), Qt::FindChildrenRecursively);
    //遍历控件获取大小和位置
    foreach(auto widget, m_Widget)
    {
        m_WidgetRect.insert(widget, QRect(widget->x(), widget->y(), widget->width(), widget->height()));
    }
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
    for(unsigned char i=0; i < (uint8_t)frameData[3] + 4 ; i++)
    {
        sumcheck += (uint8_t)frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
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
    mTcpTask->setTcpserverIpPort(QHostAddress(TCP_Server_IP),TCP_Server_Port.toUShort());                               /* 设置连接服务器的IP和port */
    mTcpTask->moveToThread(mTcpThread);                                                                                 /* 创建TCP通信的子线程 */
    mTcpThread->start();                                                                                                /* 启动线程 */
    connect(mTcpThread, &QThread::started, mTcpTask,&tcpTask::startTcpConnect);                                         /* 连接线程启动信号，建立连接 */

    connect(mTcpTask,&tcpTask::tcpConnected_signal,this,&Widget::on_tcpConnectedSuccessfull,Qt::UniqueConnection);      /* 子线程tcp连接成功通知主线程 */
    connect(mTcpTask,&tcpTask::errorOccurred_signal,this,&Widget::on_errorOccurred,Qt::UniqueConnection);               /* tcp连接发生错误 */
    connect(this,&Widget::sendTcpData_signal,mTcpTask,&tcpTask::sendTcpData,Qt::UniqueConnection);                      /* 主线程通知子线程TCP发送数据 */
    connect(mTcpTask,&tcpTask::onlineDeviceName_singal,this,&Widget::on_showDeviceId,Qt::UniqueConnection);             /* 子线程发送的在线的摄像头列表 */
    connect(mTcpTask,&tcpTask::cameraBindOK_signal,this,&Widget::on_cameraBindOK,Qt::UniqueConnection);                 /* 摄像头绑定成功 */
    connect(mTcpTask,&tcpTask::cameraDisbindOK_signal,this,&Widget::on_cameraDisbindOK,Qt::UniqueConnection);           /* 摄像头绑定失败 */
    connect(mTcpTask,&tcpTask::pictureData_signal,this,&Widget::on_showPicture,Qt::UniqueConnection);                   /* 子线程接收的图片到发送到主线程显示 */
    connect(mTcpTask,&tcpTask::appLogMessage_signal,this,&Widget::on_showLogMessage,Qt::UniqueConnection);              /* 子线程输出log信息到主线程 */
    connect(mTcpTask,&tcpTask::downLoadPictureSuccess_signal,this,&Widget::on_downLoadPictureSuccess,Qt::UniqueConnection);

    connect(mTcpThread, &QThread::finished, this,[=]()
    {
        mTcpThread->quit();
        mTcpThread->wait();
        mTcpTask->deleteLater();
        mTcpThread->deleteLater();

        isTcpThreadAlive = false;

        on_cameraDisbindOK();

        ui->ConnectTCPtButton->setEnabled(true);        /*断开连接，打开连接按键为可用状态*/
        ui->CloseTCPButton->setEnabled(false);          /*断开连接，关闭连接按键为不可用状态*/
        emit appLogMessage_signal("已经断开TCP服务器");
    });
//    connect(mTcpTask,&tcpTask::cameraOpenMotoSuccess_signal,this,&Widget::on_openMotoSuccess,Qt::UniqueConnection);     /* 摄像头打开电机成功 */
    isTcpThreadAlive = true;
}
/*关闭TCP连接按键*/
void Widget::on_CloseTCPButton_clicked()
{
    mTcpThread->quit();
    mTcpThread->wait();
    mTcpTask->deleteLater();
    mTcpThread->deleteLater();

    isTcpThreadAlive = false;

    ui->ConnectTCPtButton->setEnabled(true);        /*断开连接，打开连接按键为可用状态*/
    ui->CloseTCPButton->setEnabled(false);          /*断开连接，关闭连接按键为不可用状态*/
    emit appLogMessage_signal("已经断开TCP服务器");
}
/* 下载图片按键 */
void Widget::on_searchPictureButton_clicked()
{
    if(!checkTcpIsConnected())return;
    chooseDate = ui->tcpDownLoadYearComboBox->currentText().toLocal8Bit() + "-" +
                    ui->tcpDownLoadMouthComboBox->currentText().toLocal8Bit().rightJustified(2, '0') + "-" +
                    ui->tcpDownLoadDayComboBox->currentText().toLocal8Bit().rightJustified(2, '0');
    qDebug() << "当前选择的日期为:" << chooseDate;
    sendExistPictureFileToServer();                                                                                             /* 向服务器发送本地已经存在的文件 */
    if(bindCameraDeviceID.isEmpty() && !ui->downLoadIdLineEdit->text().isEmpty())
    {
        mTcpTask->setBindCameraDevice(ui->downLoadIdLineEdit->text().toLocal8Bit());                                              /* 当没有绑定在线摄像头时设置ID */
        emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,SET_TEMP_DEVICE_ID,ui->downLoadIdLineEdit->text().toLocal8Bit().size(),ui->downLoadIdLineEdit->text().toLocal8Bit()));
    }
    else if(bindCameraDeviceID.isEmpty() && ui->downLoadIdLineEdit->text().isEmpty())
    {
        emit appLogMessage_signal("未绑定设备也未设置下载的设备ID，请检查!");
        return;
    }
    mTcpTask->setSaveDateFileName(chooseDate);                                                                                  /* 设置保存的文件名称，以读取到日历控件的日期命名 */
                                                                                                                                /* 向服务器发送下载图片的命令,携带指定的日期，代表下载指定日期的图片数据 */
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,DOWNLOAD_PICTURE,chooseDate.size(),chooseDate));      /* 请求下载图片命令 */

//    emit appLogMessage_signal("开始下载图片");ui->searchPictureButton->setDisabled(true);
    isTcpBackFinish = false;
}
/* TCP连接成功响应槽函数 */
void Widget::on_tcpConnectedSuccessfull()
{
    ui->ConnectTCPtButton->setEnabled(false);       /*连接成功，打开连接按键为不可用状态*/
    ui->CloseTCPButton->setEnabled(true);           /*连接成功，关闭连接按键为可用状态*/
    emit appLogMessage_signal("服务器连接成功");
}
/* tcp连接错误处理槽函数 */
void Widget::on_errorOccurred()
{
    isTcpThreadAlive = false;
    emit appLogMessage_signal("发生TCP连接错误");
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
    QString picPath;
    if(bindCameraDeviceID.isEmpty())
    {
        picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + ui->downLoadIdLineEdit->text().toLocal8Bit() + "/" + chooseDate;
    }
    else
    {
        picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraDeviceID + "/" + chooseDate;
    }
    picPath += "/" + index.data().toString();
    QByteArray picData;
    QFile file(picPath);
    bool isReadOK = file.open(QIODevice::ReadOnly); //只读模式打开
    if(isReadOK)
    {
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
    if(!checkTcpIsConnected())return;
    if(isBindFinish == false)
    {
        emit appLogMessage_signal("请绑定设备!");
        return;
    }
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_CAMER,TAKE_PICTURE,0,0));
}
/* 保存路径按钮 */
void Widget::on_setSavePathButton_clicked()
{
    QString selectDir = QFileDialog::getExistingDirectory();
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
    if(chooseDate.isEmpty())
    {
        emit appLogMessage_signal("请检查选择的日期是否正确");
        return;
    }
    QString picPath;
    if(bindCameraDeviceID.isEmpty())
    {
        picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + ui->downLoadIdLineEdit->text().toLocal8Bit() + "/" + chooseDate;
    }
    else
    {
        picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraDeviceID + "/" + chooseDate;
    }
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
        emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,CLIENT_EXIST_PICTURE_FILE_NAME,
                                                 fileResults.at(i).toLocal8Bit().size(),
                                                 fileResults.at(i).toLocal8Bit()));
        emit appLogMessage_signal("本地存在的图片:" + fileResults.at(i).toLocal8Bit());
    }
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,CLIENT_EXIST_PICTURE_FILE_NAME_END,0,0));
}

QByteArray Widget::packTcpDataFrame(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, QByteArray data)
{
    /* 0xAA byte1 byte2 byte3  byte4 byte5 byte6 byte7   byteN   0xBB */
    /* 帧头  角色   目标   作用       长度（四字节）          数据载荷   帧尾 */
    QByteArray tempData;
    uint32_t cnt = 0;
    tempData.resize(4 + 4);
    tempData[cnt++] = 0xAA;                 /* 帧头 */
    tempData[cnt++] = charactar;      /* 角色 */
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

bool Widget::checkTcpIsConnected()
{
    if(!isTcpThreadAlive)
    {
        QMessageBox::information(this, "提示", "请检查TCP链接！");
        return false;
    }
    return true;
}

void Widget::hideTcpSetScheduledTimingGroup()
{
    ui->label_15->hide();
    ui->tcpScheduledTimeLineEdit->hide();
    ui->label_22->hide();
}

void Widget::showTcpSetScheduledTimingGroup()
{
    ui->label_15->show();
    ui->tcpScheduledTimeLineEdit->show();
    ui->label_22->show();
}

void Widget::hideTcpSetRecordTimingGroup()
{
    ui->label_16->hide();
    ui->tcpHourTimingComboBox->hide();
    ui->label_23->hide();
    ui->tcpMinuteTimingComboBox->hide();
    ui->label_24->hide();
    ui->tcpAddRecordTimeComboBox->hide();
    ui->tcpAddRecordTimePushButton->hide();
    ui->tcpDeleteRecordTimePushButton->hide();
}

void Widget::showTcpSetRecordTimingGroup()
{
    ui->label_16->show();
    ui->tcpHourTimingComboBox->show();
    ui->label_23->show();
    ui->tcpMinuteTimingComboBox->show();
    ui->label_24->show();
    ui->tcpAddRecordTimeComboBox->show();
    ui->tcpAddRecordTimePushButton->show();
    ui->tcpDeleteRecordTimePushButton->show();
}

void Widget::hideSetScheduledTimingGroup()
{
    ui->label_14->hide();
    ui->scheduledTimeLineEdit->hide();
    ui->label_25->hide();
}

void Widget::showSetScheduledTimingGroup()
{
    ui->label_14->show();
    ui->scheduledTimeLineEdit->show();
    ui->label_25->show();
}

void Widget::showSetRecordTimingGroup()
{
    ui->label_12->show();
    ui->hourTimingComboBox->show();
    ui->label_27->show();
    ui->minuteTimingComboBox->show();
    ui->label_26->show();
    ui->addRecordTimeComboBox->show();
    ui->addRecordTimePushButton->show();
    ui->deleteRecordTimePushButton->show();
}

void Widget::hideSetRecordTimingGroup()
{
    ui->label_12->hide();
    ui->hourTimingComboBox->hide();
    ui->label_27->hide();
    ui->minuteTimingComboBox->hide();
    ui->label_26->hide();
    ui->addRecordTimeComboBox->hide();
    ui->addRecordTimePushButton->hide();
    ui->deleteRecordTimePushButton->hide();
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
    if(!checkTcpIsConnected()){return;};
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,CLEAR_SERVER_CACHE,0,0));
}
/* 拍照完成响应槽函数 */
void Widget::on_takePicFinish()
{
    ui->takePictureButton->setDisabled(false);
    emit appLogMessage_signal("拍照完成");
    isTcpBackFinish = true;
    on_searchPictureButton_clicked();/* 把拍摄的照片下载下来 */
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
        connect(this,&Widget::sendBinFileData_signal,serialPortTask,&SerialPortThread::sendBinFile,Qt::UniqueConnection);               /* 发送bin文件 */

        connect(serialPortTask,&SerialPortThread::OpenSerialPortOK_signal,this,&Widget::on_openSerialPortState,Qt::UniqueConnection);   /* 连接串口设备连接状态的处理槽函数 */
        connect(serialPortTask,&SerialPortThread::appLogMessage_signal,this,&Widget::on_showLogMessage,Qt::UniqueConnection);           /* 子线程输出log信息到主线程 */
        connect(serialPortTask,&SerialPortThread::pictureData_signal,this,&Widget::on_showPicture,Qt::UniqueConnection);                /* 串口接收的图片显示到界面 */

        connect(serialPortTask,&SerialPortThread::deviceIapState_signal,this,[=]()                                                      /* 设备处于IAP引导状态 */
        {
            ui->binDownLoadPushButton->show();
            ui->binDownLoadPushButton->setText("固件下载");
        });
        connect(serialPortTask,&SerialPortThread::deviceAppState_signal,this,[=]()                                                      /* 设备处于APP正常运行状态 */
        {
            ui->binDownLoadPushButton->show();
            ui->binDownLoadPushButton->setText("引导模式");
        });

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

        ui->binDownLoadPushButton->hide();

        return;
    }
}
/* 判断串口设备打开是否成功 */
void Widget::on_openSerialPortState(bool state)
{
    isSerialPortOpen = state;
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
    if(!isSerialPortOpen)
    {
        emit appLogMessage_signal("请检查串口是否打开");
        return;
    }
    QByteArray wifiName = ui->wifiNameLineEdit->text().toLocal8Bit();               /* 获取WIFI名称 */
    QByteArray wifiPassWord = ui->wifiPasswordLineEdit->text().toLocal8Bit();       /* 获取WIFI密码 */

    QByteArray serverIp = ui->serverIpLineEdit->text().toLocal8Bit();               /* 获取服务器IP */
    uint16_t serverPort = ui->serverPortLineEdit->text().toUShort();                /* 获取服务器PORT */

    QByteArray deviceId = ui->deviceIdSetLineEdit->text().toLocal8Bit();            /* 获取设置的相机ID */

    unsigned short imageSize = 0;                                                   /* 图片大小 */
    unsigned short imageQuality = 0;                                                /* 图片质量 */
    unsigned short ledFlashBrightness = 0;                                          /* 闪光灯的亮度 */
    unsigned short takePictureDelayTime = 0;                                        /* 拍照延时 */

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

    imageQuality         = ui->imageQualityComboBox->currentIndex();
    ledFlashBrightness   = ui->ledBrightnessLineEdit->text().toUShort();
    takePictureDelayTime = ui->takePictureDelayTimeLineEdit->text().toUShort();

    switch (ui->pictureDirectionComboBox->currentIndex())
    {
        case 0:             /* 正常 */
        {
            emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_CAMERA_PICTURE_DIRECTION,0));
            break;
        }
        case 1:             /* 镜像 */
        {
            emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_CAMERA_PICTURE_DIRECTION,1));
            break;
        }
        case 2:             /* 翻转 */
        {
            emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_CAMERA_PICTURE_DIRECTION,2));
            break;
        }
        case 3:             /* 镜像翻转 */
        {
            emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_CAMERA_PICTURE_DIRECTION,3));
            break;
        }
        default:
            break;
    }

    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CDM_WIFI_NAME                     ,wifiName));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_WIFI_PASSWORD                 ,wifiPassWord));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SERVER_IP                     ,serverIp));
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SERVER_PORT                   ,serverPort));
    emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_DEVICE_ID                     ,deviceId));
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_PICTURE_SIZE                  ,imageSize));
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_PICTURE_QUALITY               ,imageQuality));
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_LIED_BRIGHTNESS           ,ledFlashBrightness));
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_TAKE_PICTURE_DELAY_TIME   ,takePictureDelayTime));

    if(ui->scheduledTimeCheckBox->isChecked())/* 间隔定时模式 */
    {
        QByteArray dateTime;
        dateTime = ui->scheduledTimeLineEdit->text().toLocal8Bit();
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
        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_DELAY_TIME,dateTime.toUShort()));/* 发送间隔定时时间 */
    }
    else if(ui->recordCheckBox->isChecked()) /* 固定定时 */
    {
        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_DELAY_TIME,0));
        QByteArray dateTime;
        for(int i=0;i<ui->addRecordTimeComboBox->count();i++)
        {
            dateTime = ui->addRecordTimeComboBox->itemText(i).toLocal8Bit();
            emit sendSerialPortData(setSerialPortStringDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_RECORD_TIME,dateTime));
        }
    }
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_PARA_END,0));/* 配置信息发送完毕 */
    emit appLogMessage_signal("相关参数设置已发送!");
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
    if(!checkTcpIsConnected()){return;};
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,GET_ONLINE_CAMERA_DEVICE_ID,0,0));
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
    bindCameraDeviceID = ui->deviceOnlineComboBox->currentText().toLocal8Bit();
    mTcpTask->setBindCameraDevice(bindCameraDeviceID);
    ui->disConnectDeviceButton->setEnabled(true);                   /* 断开连接设备按键可用 */
    ui->connectDeviceButton->setEnabled(false);                     /* 连接设备按键不可用 */
    isBindFinish = true;                                            /* 绑定成功标志 */
    emit appLogMessage_signal("相机绑定成功！");
}

void Widget::on_cameraDisbindOK()
{
    bindCameraDeviceID = "";
    mTcpTask->setBindCameraDevice(bindCameraDeviceID);
    ui->disConnectDeviceButton->setEnabled(false);                  /* 断开连接设备按键不可用 */
    ui->connectDeviceButton->setEnabled(true);                      /* 连接设备按键可用 */
    isBindFinish = false;                                           /* 解除绑定 */
    emit appLogMessage_signal("解除相机绑定成功！");
}
/* 打开水泵按键 */
void Widget::on_motoControlPushButton_clicked()
{
    if(isBindFinish == false)
    {
        emit appLogMessage_signal("请绑定设备!");
        return;
    }
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_CAMER,OPEN_MOTO_CMD,0,0));
}

void Widget::on_openMotoSuccess()
{
    emit appLogMessage_signal("电机已经启动");
}

void Widget::on_readDeviceInfoPushButton_clicked()
{
    if(!isSerialPortOpen)
    {
        emit appLogMessage_signal("请检查串口是否打开");
        return;
    }
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SHOW_DEVICE_INFO,0));
}

void Widget::on_resetDevicePushButton_clicked()
{
    if(!isSerialPortOpen)
    {
        emit appLogMessage_signal("请检查串口是否打开");
        return;
    }
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_RESET_DEVICE,0));
}

void Widget::on_addRecordTimePushButton_clicked()
{
    QByteArray dateTime;
    dateTime.resize(5);
    sprintf(dateTime.data(),"%02d:%02d",ui->hourTimingComboBox->currentText().toInt(),ui->minuteTimingComboBox->currentText().toInt());
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
        QByteArray shortData;
        shortData.resize(2);
        shortData[0] = dateTime.toUShort();
        shortData[1] = dateTime.toUShort()>>8;
        emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_CAMER,SET_SCHEDULED_TIME,2,shortData));
        emit appLogMessage_signal("间隔定时的定时时间已发送!");
    }
    else if(ui->tcpRecordCheckBox->isChecked()) /* 固定定时 */
    {
        QByteArray dateTime;
        for(int i=0;i<ui->tcpAddRecordTimeComboBox->count();i++)
        {
            dateTime = ui->tcpAddRecordTimeComboBox->itemText(i).toLocal8Bit();
            emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_CAMER,SET_RECORD_TIME,dateTime.size(),dateTime));
        }
        emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_CAMER,SET_RECORD_TIME_END,0,0));
        emit appLogMessage_signal("固定定时的定时时间已发送!");
    }
}

/* 添加一组定时按键 */
void Widget::on_tcpAddRecordTimePushButton_clicked()
{
    QByteArray dateTime;
    dateTime.resize(5);
    sprintf(dateTime.data(),"%02d:%02d",ui->tcpHourTimingComboBox->currentText().toInt(),ui->tcpMinuteTimingComboBox->currentText().toInt());
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

void Widget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    show();
    qApp->processEvents();
    float width = this->width() * 1./ 477;
    float height = this->height() * 1./630;
    for ( auto it= m_WidgetRect.begin(); it != m_WidgetRect.end(); it++ )
    {
        it.key()->setGeometry(it.value().x() * width, it.value().y() * height, it.value().width() * width, it.value().height() * height);
        it.key()->updateGeometry();
    }
}

void Widget::on_connectDeviceButton_clicked()
{
    if(!checkTcpIsConnected()){return;};
    if(ui->deviceOnlineComboBox->currentText() == "")
    {
        emit appLogMessage_signal("请检查设备ID是否正确！");
        return;
    }
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,CLIENT_BIND_CAMERA,
        ui->deviceOnlineComboBox->currentText().toLocal8Bit().size(),ui->deviceOnlineComboBox->currentText().toLocal8Bit()));
}

void Widget::on_disConnectDeviceButton_clicked()
{
    if(!checkTcpIsConnected()){return;};
    emit sendTcpData_signal(packTcpDataFrame(DEVICE_LABEL_PC,PC_TO_SERVER,CLIENT_DISBIND_CAMERA,
        ui->deviceOnlineComboBox->currentText().toLocal8Bit().size(),ui->deviceOnlineComboBox->currentText().toLocal8Bit()));
    bindCameraDeviceID = "";
    ui->disConnectDeviceButton->setEnabled(false);                          /* 断开连接设备按键不可用 */
}

void Widget::on_tcpScheduledTimeCheckBox_stateChanged(int arg1)
{
    if(Qt::Checked == arg1)
    {
        showTcpSetScheduledTimingGroup();
    }
    else
    {
        hideTcpSetScheduledTimingGroup();
    }
}

void Widget::on_scheduledTimeCheckBox_stateChanged(int arg1)
{
    if(Qt::Checked == arg1)
    {
        showSetScheduledTimingGroup();
    }
    else
    {
        hideSetScheduledTimingGroup();
    }
}

void Widget::on_tcpRecordCheckBox_stateChanged(int arg1)
{
    if(Qt::Checked == arg1)
    {
        showTcpSetRecordTimingGroup();
    }
    else
    {
        hideTcpSetRecordTimingGroup();
    }
}

void Widget::on_recordCheckBox_stateChanged(int arg1)
{
    if(Qt::Checked == arg1)
    {
        showSetRecordTimingGroup();
    }
    else
    {
        hideSetRecordTimingGroup();
    }
}

void Widget::on_downLoadPictureSuccess()
{
    /* 将程序下的图片文件复制到用户指定的文件夹下 */
    copyDirectoryFiles(QCoreApplication::applicationDirPath() + "/" + "照片",savePicFilePath + "/" + "照片",true);
    /* 遍历存储的所有图片文件 */
    QString picPath;
    if(bindCameraDeviceID.isEmpty())
    {
        picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + ui->downLoadIdLineEdit->text().toLocal8Bit() + "/" + chooseDate;
    }
    else
    {
        picPath = QCoreApplication::applicationDirPath() + "/" + "照片" + "/" + bindCameraDeviceID + "/" + chooseDate;
    }
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
}


void Widget::on_testModePushButton_clicked()
{
    if(!isSerialPortOpen)
    {
        emit appLogMessage_signal("请检查串口是否打开");
        return;
    }
    if(ui->testModePushButton->text() == "测试模式")
    {
        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_TEST_MODE,0));
        ui->takePictureTestModePushButton->show();
        ui->openMotoTestModePushButton->show();
        ui->testModePushButton->setText("工作模式");
    }
    else if(ui->testModePushButton->text() == "工作模式")
    {
        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_SET_WORK_MODE,0));
        ui->takePictureTestModePushButton->hide();
        ui->openMotoTestModePushButton->hide();
        ui->testModePushButton->setText("测试模式");
    }
}

void Widget::on_takePictureTestModePushButton_clicked()
{
    if(!isSerialPortOpen)
    {
        emit appLogMessage_signal("请检查串口是否打开");
        return;
    }
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_TAKE_PICTURE_TEST_MODE,0));
}


void Widget::on_openMotoTestModePushButton_clicked()
{
    if(!isSerialPortOpen)
    {
        emit appLogMessage_signal("请检查串口是否打开");
        return;
    }
    emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_OPEN_MOTO_TEST_MODE,0));
}


void Widget::on_binDownLoadPushButton_clicked()
{
    if(ui->binDownLoadPushButton->text() == "引导模式")
    {
        emit sendSerialPortData(setSerialPortUshortDataFormat(0xAA,SerialPortThread::frameAddress::PC,SerialPortThread::frameCmd::CMD_RUN_IAP,0));
    }
    else if(ui->binDownLoadPushButton->text() == "固件下载")
    {
        QString runPath = QCoreApplication::applicationDirPath();//获取项目的根路径
        QString binFilePath = QFileDialog::getOpenFileName(this,QStringLiteral("选择文件"),runPath,"Bin Files(*.bin)",nullptr,QFileDialog::DontResolveSymlinks);

        QFile file(binFilePath);

        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "无法打开文件：" << file.errorString();
            return ;
        }

        QByteArray data = file.readAll(); // 读取文件数据到 QByteArray

        emit sendBinFileData_signal(data);

        file.close();
    }
}

