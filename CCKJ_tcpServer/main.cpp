#include "tcp_serverwidget.h"

#include <QApplication>
/*
修改日志
2022/8/13  完成TCP客户端数据的转发
2022/8/17  修复连接用户数量显示不正确的BUG
2022/8/17  BUG:若某个客户端异常下线，则不会触发diaconnected     (未修复)
2022/8/24  修复了已知bug，问题描述：子线程和窗口的主线程线程地址一致，线程无法结束，线程启动错误。（已修复）
2022/8/24  修复了已知bug，问题描述：结束监听后客户端还能连接到客户端。（已修复）
2022/9/28  修复了已知bug，问题描述：完善TCP的地址和端口绑定，修复了TCP断开连接后崩溃的问题
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    TCP_ServerWidget w;
    w.show();

    return a.exec();
}
