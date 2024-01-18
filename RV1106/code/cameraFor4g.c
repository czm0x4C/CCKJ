#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <math.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>

#include "cameraFor4g.h"
#include "ringBuffer.h"

static void tcpDataAnalysis(_RingBuffer *ringbuffer);               /* 分析TCP数据 */
static void frameDataAnalysis(uint8_t *data,uint32_t dataLen);      /* 解析帧 */
static void pcToCameraDeal(uint8_t *data);                          
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data);/* TCP数据封帧同时发送 */

static uint8_t cameraInit(char *cameraDevicePath);
static uint8_t cameraTakePicture(void);
static uint8_t closeCamera(void);

#define BUFFER_SIZE 1024

int clientSocket;                   /* 全局的sock对象 */    
char buffer[BUFFER_SIZE];
int cameraFd;                       /* 定义一个设备描述符 */
_deviceInfo deviceAttributeInfo;    /* 保存当前的设备信息 */

char getTackPictureFlag = 0;        /* 拍照标志 */
char openMotoFlag = 0;              /* 打开电机标志 */      

// 信号处理函数
void sigint_handler(int sig) {
    printf("Received SIGINT signal. Terminating...\n");
    pthread_cancel(pthread_self());
}

// 线程函数
void* tcpReceiveThreadFunc(void* arg) 
{
    uint32_t recTcpDataLen = 0;
    while (1) 
    {
        // 从服务器接收数据,接收为阻塞接受
        memset(buffer, 0, sizeof(buffer));
        recTcpDataLen = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (recTcpDataLen < 0) 
        {
            perror("Failed to receive message");
            exit(1);
        }

        WriteBytes(&TCP_RxRingBuffer,buffer,recTcpDataLen);     /* 写入数据到缓冲区中 */
        tcpDataAnalysis(&TCP_RxRingBuffer);                     /* 解析数据 */
    }

    pthread_exit(NULL);
}

// 线程函数
void* tcpSendThreadFunc(void* arg) 
{
    while (1) 
    {
/*************************************************************************发送TCP心跳包**********************************************************************************************/
		static uint8_t heartBeatTimeCnt = 0;
		if(heartBeatTimeCnt == 100)
		{
            packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,HERAT_BEAT_PACK,0,0);
            heartBeatTimeCnt = 0;
            printf("send heart beat pack!\n");
		}
		heartBeatTimeCnt++;

        if(1 == getTackPictureFlag)
        {
            cameraTakePicture();
            printf("done!\n");
            getTackPictureFlag = 0;
        }

        usleep(1000 * 10);/* 10ms */
    }

    pthread_exit(NULL);
}

void printHexArray(const unsigned char* array, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02X ", array[i]);
        if ((i + 1) % 10 == 0) {
            printf("\n");
        }
    }
}

static void tcpDataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *FrameData = NULL;                                        /* 存储解析帧的空间 */
	unsigned short allFrameDataLen = ringbuffer->Lenght;               		/* 获取缓冲区中的存储的数据长度 */                                                     
	uint32_t index = 0;														/* 数组中的偏移位置 */

    struct sysinfo sysInfo;
    if (sysinfo(&sysInfo) == 0) 
    {
        long long freeMemory = sysInfo.freeram * sysInfo.mem_unit;
        if(freeMemory >= allFrameDataLen)                                  /* 判断剩余内存是否大于申请内存 */
        {
            FrameData = (uint8_t *)malloc(allFrameDataLen);	               /* 为读取缓冲区中的所有数据申请内存 */
            if (FrameData == NULL) 
            {
                printf("Failed to allocate memory\n");
                return ;
            }
            if(ReadBytes(ringbuffer,FrameData,allFrameDataLen) == 1)		/* 读取缓冲区中的所有数据 */
            {
                for(uint32_t i=0;i<allFrameDataLen;i++)						/*  */		
                {
                    if((0xAA == FrameData[i]) && (allFrameDataLen - i >= 8))/* 确认数据帧头同时能得到数据长度 */
                    {
                        uint32_t dataLen = FrameData[i+4] | FrameData[i+5]<<8 | FrameData[i+6]<<16 | FrameData[i+7]<<24;
                        if(allFrameDataLen - i >= 8+dataLen+1)              /* 能都得到整个帧的数据长度 */
                        {
                            if(0xBB == FrameData[i+8+dataLen])              /* 确认数据帧尾 */
                            {
                                frameDataAnalysis(&FrameData[i],9+dataLen); /* 分析帧的数据 */
                            }
                            i = i + dataLen + 8;                            /* 偏移 */
                            index = i;
                            break;
                        }
                    }
                }
                WriteBytes(ringbuffer,&FrameData[index],allFrameDataLen - index);
            }
            free(FrameData);  //释放内存
        }
    } 
    else 
    {
        printf("Failed to get memory information\n");
    }
}
static void frameDataAnalysis(uint8_t *data,uint32_t dataLen)
{
	switch(data[2])
	{
			case SERVER_TO_CLIENT:                      /* 服务器到pc */
			{
				break;
			}
			case PC_TO_CAMER:                           /* pc到camera */
			{
				pcToCameraDeal(data);
				break;
			}
			default:
				break;
	}
}
static void pcToCameraDeal(uint8_t *data)
{
	switch(data[3])
	{
		case TAKE_PICTURE:                           /* pc到camera */
		{
			getTackPictureFlag = 1;
            printf("take picture!\n");
			break;
		}
		case SET_RECORD_TIME:                           /* pc到camera */
		{
			uint32_t dataLen = data[4] | data[5]<<8 | data[6]<<16 | data[7]<<24;
			if(deviceAttributeInfo.recordTimeIndex > 50)
			{
					break;
			}
			if(deviceAttributeInfo.isSetRecordTime == 1)
			{
				for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
				{
						memset(deviceAttributeInfo.recordTime[i],0,6);
				}
				deviceAttributeInfo.isSetRecordTime = 0;
			}
			memset((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex],0,6);
			memcpy((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex], &data[8],dataLen);
			deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex][5] = '\0';
			deviceAttributeInfo.recordTimeIndex++;
			deviceAttributeInfo.scheduledDeletion = 0;
			break;
		}
		case SET_RECORD_TIME_END:                           /* pc到camera */
		{
			deviceAttributeInfo.recordTimeIndex = 0;
			deviceAttributeInfo.isSetRecordTime = 1;
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,SET_RECORD_TIME_SUCCESS,0,0);
			// WriteFlashData(ADDR_FLASH_SECTOR_7,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
			break;
		}
		case SET_SCHEDULED_TIME:                           /* pc到camera */
		{
			deviceAttributeInfo.scheduledDeletion = data[8] | data[9] <<8;
			for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
			{
					memset(deviceAttributeInfo.recordTime[i],0,6);
			}
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,SET_SCHEDULED_TIME_CMD_SUCCESS,0,0);
			// WriteFlashData(ADDR_FLASH_SECTOR_7,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
			break;
		}
		case OPEN_MOTO_CMD:
		{
			// openMotoFlag = 1;
            printf("open moto!\n");
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,OPEN_MOTO_SUCCESS_CMD,0,0);
			break;
		}
		default:
			break;
	}
}
/* 给发送的TCP数据封帧，同时将数据推送出去 */
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data)
{
	/* 因为TCP数据包的数据量大，同时又因为单片机的内存岌岌可危，采用使用不同内存段来实现发送 */
	/* 0xAA byte1 byte2 byte3  byte4 byte5 byte6 byte7   byteN   0xBB */
	/* 帧头  角色   目标   作用       长度（四字节）          数据载荷   帧尾 */
	uint8_t tempData[20];
	uint32_t cnt = 0;
	tempData[cnt++] = 0xAA;                 /* 帧头 */
	tempData[cnt++] = charactar;      		/* 角色 */
	tempData[cnt++] = target;               /* 目标 */
	tempData[cnt++] = cmd;                  /* 作用 */
	tempData[cnt++] = dataLen;              /* 长度 */
	tempData[cnt++] = dataLen>>8;
	tempData[cnt++] = dataLen>>16;
	tempData[cnt++] = dataLen>>24;

    // 发送数据到服务器
    if (send(clientSocket,tempData, 8, 0) < 0)          /* 发送帧头 */
    {
        perror("Failed to send message");
        exit(1);
    }
    if (send(clientSocket,data,dataLen, 0) < 0)         /* 发送数据 */
    {
        perror("Failed to send message");
        exit(1);
    }
    tempData[0] = 0xBB; 
    if (send(clientSocket,tempData,1, 0) < 0)           /* 发送帧尾 */
    {
        perror("Failed to send message");
        exit(1);
    }
}

void *frm_base[3];  //映射后的用户空间的首地址
unsigned int frm_size[3];
struct v4l2_buffer buf;
/* 摄像头初始化 */
static uint8_t cameraInit(char *cameraDevicePath)
{
    printf("********************************start***************************************\n");
    cameraFd = open(cameraDevicePath, O_RDWR);
    if(cameraFd < 0){
        perror("video设备打开失败\n");
        return -1;
    }
    else{
        printf("video设备打开成功\n");
    }
	printf("********************************1***************************************\n");
	struct v4l2_capability vcap;
	ioctl(cameraFd, VIDIOC_QUERYCAP, &vcap);
    if (!(V4L2_CAP_VIDEO_CAPTURE & vcap.capabilities)) {
        perror("Error: No capture video device!\n");
        return -1;
    }
	printf("********************************2***************************************\n");
	struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    printf("摄像头支持所有格式如下:\n");
    while(ioctl(cameraFd,VIDIOC_ENUM_FMT,&fmtdesc) == 0){
        printf("v4l2_format%d:%s\n",fmtdesc.index,fmtdesc.description);
        fmtdesc.index++;
    }
	printf("********************************3***************************************\n");
	struct v4l2_frmsizeenum frmsize;
	frmsize.index = 0;
	frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("V4L2_PIX_FMT_MJPEG格式支持所有分辨率如下:\n");
	frmsize.pixel_format = V4L2_PIX_FMT_MJPEG;// V4L2_PIX_FMT_YUYV
	while(ioctl(cameraFd,VIDIOC_ENUM_FRAMESIZES,&frmsize) == 0){
		printf("frame_size<%d*%d>\n",frmsize.discrete.width,frmsize.discrete.height);
		frmsize.index++;
	}
	printf("********************************4***************************************\n");
	struct v4l2_frmivalenum frmival;
    frmival.index = 0;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.pixel_format = V4L2_PIX_FMT_MJPEG;
    frmival.width = 2592;
    frmival.height = 1944;      
    while(ioctl(cameraFd,VIDIOC_ENUM_FRAMEINTERVALS,&frmival) == 0){
        printf("frame_interval under frame_size <%d*%d> support %dfps\n",frmival.width,frmival.height,frmival.discrete.denominator / frmival.discrete.numerator);
        frmival.index++;
    }
	printf("********************************5***************************************\n");
	struct v4l2_format vfmt;
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vfmt.fmt.pix.width = 2592;
    vfmt.fmt.pix.height = 1944;
    vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    if(ioctl(cameraFd,VIDIOC_S_FMT,&vfmt) < 0){
        perror("设置格式失败\n");
        return -1;
    }
    // 检查设置参数是否生效
    if(ioctl(cameraFd,VIDIOC_G_FMT,&vfmt) < 0){
        perror("获取设置格式失败\n");
        return -1;
    }
    else if(vfmt.fmt.pix.width == 2592 && vfmt.fmt.pix.height == 1944 && vfmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG){
        printf("设置格式生效,实际分辨率大小<%d * %d>,图像格式:Motion-JPEG\n",vfmt.fmt.pix.width,vfmt.fmt.pix.height);
    }
    else{
        printf("设置格式未生效\n");
    }

	printf("********************************6***************************************\n");
	struct v4l2_requestbuffers reqbuf;
    reqbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.count = 1;   //1个帧缓冲
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if(ioctl(cameraFd,VIDIOC_REQBUFS,&reqbuf) < 0)
    {
        perror("申请缓冲区失败\n");
        return -1;
    }
	printf("********************************7***************************************\n");
	// 将帧缓冲映射到进程地址空间
    
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    // 将每一帧对应的缓冲区的起始地址保存在frm_base数组中，读取采集数据时，只需直接读取映射区即可
    for(buf.index=0;buf.index<1;buf.index++)
    {
        ioctl(cameraFd, VIDIOC_QUERYBUF, &buf);
        frm_base[buf.index] = mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,cameraFd,buf.m.offset);
        frm_size[buf.index] = buf.length;

        if(frm_base[buf.index] == MAP_FAILED)
        {
            perror("mmap failed\n");
            return -1;
        }

        // 入队操作
        if(ioctl(cameraFd,VIDIOC_QBUF,&buf) < 0)
        {
            perror("入队失败\n");
            return -1;
        }
    }
    printf("********************************8***************************************\n");
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cameraFd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("开始采集失败\n");
        return -1;
    }
}

static uint8_t cameraTakePicture(void)
{
    uint8_t* startPic = NULL;
	uint8_t* endPic = NULL;
	uint32_t picLen = 0;

	printf("********************************9***************************************\n");
	struct v4l2_buffer  readbuffer;
    memset(&readbuffer, 0, sizeof(readbuffer));
    readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    readbuffer.memory = V4L2_MEMORY_MMAP;
    if(ioctl(cameraFd, VIDIOC_DQBUF, &readbuffer) < 0)
    {
        perror("读取帧失败\n");
    }
    
    if(ioctl(cameraFd, VIDIOC_QBUF, &readbuffer) < 0)
    {
        perror("读取帧失败\n");
    }

    if(ioctl(cameraFd, VIDIOC_DQBUF, &readbuffer) < 0)
    {
        perror("读取帧失败\n");
    }
    
    // 保存这一帧，格式为jpg
    FILE *file = fopen("v4l2_cap.yuv","w+");
    fwrite(frm_base[readbuffer.index],buf.length,1,file);
    fclose(file);
    
    uint8_t *p = (uint8_t *)frm_base[readbuffer.index];
    while(p[0] != 0xff || p[1] != 0xd8)p++;
    startPic = p;
    p = (uint8_t *)frm_base[readbuffer.index];
    while(p[0] != 0xff || p[1] != 0xd9)p++;
    endPic   = p;
    picLen   = endPic - startPic;

    if(0 != picLen && picLen < buf.length)
    {
        packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,TAKE_PICTURE_END,picLen+2,startPic);
    }

    startPic = NULL;
    endPic = NULL;

    if(ioctl(cameraFd, VIDIOC_QBUF, &readbuffer) < 0)
    {
        perror("读取帧失败\n");
    }
    return 0;
}

static uint8_t closeCamera(void)
{
    printf("********************************10***************************************\n");
	// 停止采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cameraFd, VIDIOC_STREAMOFF, &type) < 0){
        perror("停止采集失败\n");
        return -1;
    }
    // 释放映射
    for(int i=0;i<3;i++){
        munmap(frm_base[i],frm_size[i]);
    }
    
	close(cameraFd);
}

int main(int argc, char *argv[]) 
{
    int opt;
    char *tcpServerIp = NULL;
    int tcpServerPort = 0;

    char *cameraPath = NULL;

    // 使用循环获取每个选项和对应的值
    while ((opt = getopt(argc, argv, "i:p:c:")) != -1) 
    {
        switch (opt) 
        {
            case 'i':
                tcpServerIp = optarg;
                break;
            case 'p':
                tcpServerPort = atoi(optarg);
                break;
            case 'c':
                cameraPath = optarg;
                break;
            default:
                fprintf(stderr, "未知选项: %c\n", opt);
                break;
        }
    }

    /* 检查输入参数是否有效 */
    if(NULL == tcpServerIp || 0 == tcpServerPort)
    {
        printf("请检查输入参数时候有效!\n");
        return 1;
    }

    if(NULL == cameraPath)
    {
        printf("请检查摄像头路径!!\n");
        return 1;
    }

    /* 先执行摄像头的初始化 */
    cameraInit(cameraPath);

    // 打印解析结果
    printf("IP地址: %s\n", tcpServerIp);
    printf("端口号: %d\n", tcpServerPort);
    
    memset(&deviceAttributeInfo, 0, sizeof(deviceAttributeInfo));
    memset(&TCP_RxRingBuffer, 0, sizeof(TCP_RxRingBuffer));

    /**********************************************开始创建TCP的连接***********************************************************/
    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    // 设置服务器地址信息
    struct sockaddr_in serverAddr;  
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(tcpServerPort);
    if (inet_pton(AF_INET, tcpServerIp, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(1);
    }

    // 连接到服务器
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to the server.\n");
    
    sprintf(deviceAttributeInfo.deviceID,"czm");
    packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,SET_CAMERA_DEVICE_FLAG,0,0);    /* 向服务器设置自身为camera标签 */
    packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,SET_CAMERA_DEVICE_ID,strlen(deviceAttributeInfo.deviceID),(uint8_t *)deviceAttributeInfo.deviceID);
    /**********************************************开始创建执行的任务***********************************************************/
    // 注册信号处理函数
    signal(SIGINT, sigint_handler);

    // 创建多个线程
    pthread_t tcpReceiveThread, tcpSendThread;
    pthread_create(&tcpReceiveThread, NULL, tcpReceiveThreadFunc, NULL);
    pthread_create(&tcpSendThread, NULL, tcpSendThreadFunc, NULL);

    // 等待线程退出
    pthread_join(tcpReceiveThread, NULL);
    pthread_join(tcpSendThread, NULL);

    printf("All threads have exited. Program terminated.\n");

    // 关闭套接字
    close(clientSocket);

    return 0;
}
