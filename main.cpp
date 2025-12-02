/**
 * @file main.cpp
 * @brief Qt节点编辑器主程序入口文件
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#include "MainWindow.h"     // 主窗口类头文件
#include <QApplication>     // Qt应用程序类

/**
 * @brief 程序主入口函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 应用程序执行结果码
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序基本信息
    app.setApplicationName("Qt节点编辑器");        // 应用程序名称
    app.setApplicationVersion("1.0.0");           // 应用程序版本
    app.setOrganizationName("MyCompany");        // 组织名称
    app.setWindowIcon(QIcon(":/icons/node.png"));  // 应用程序图标
    
    // 创建主窗口并显示
    MainWindow window;
    window.show();
    
    // 启动Qt事件循环
    return app.exec();
}