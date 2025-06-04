/*
 * TCP用户系统 - 服务器主程序
 * 
 * 功能说明:
 * 1. 服务器程序启动入口
 * 2. 控制台编码设置 - 确保Windows环境下中文正确显示
 * 3. 用户交互界面 - 端口配置和启动确认
 * 4. 服务器实例创建和生命周期管理
 * 
 * 启动流程:
 * 1. 设置控制台编码(Windows)
 * 2. 获取用户输入的端口号
 * 3. 创建服务器实例
 * 4. 启动服务器监听
 * 5. 保持运行直到手动停止
 */

#include "Source/Public/TCP_System.h"
#include <iostream>
#include <cstdlib>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <signal.h>
#endif

// 全局服务器指针，用于信号处理
TCPUserSystemServer* g_server = 0;

// 信号处理函数 - 关闭服务器
#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        std::cout << "\n收到关闭信号，正在关闭服务器..." << std::endl;
        if (g_server) {
            g_server->stopServer();
        }
        return TRUE;
    }
    return FALSE;
}
#else
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n收到关闭信号，正在关闭服务器..." << std::endl;
        if (g_server) {
            g_server->stopServer();
        }
    }
}
#endif

int main() {
#ifdef _WIN32
    // Windows控制台编码设置 - 解决中文乱码问题
    SetConsoleOutputCP(65001);  // 设置输出编码为UTF-8
    SetConsoleCP(65001);        // 设置输入编码为UTF-8
    
    // 设置标准输入输出为文本模式
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);
    
    // 恢复到普通文本模式以兼容std::cout
    _setmode(_fileno(stdout), _O_TEXT);
    _setmode(_fileno(stdin), _O_TEXT);
    
    // 设置Windows控制台信号处理
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    // 设置Linux信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#endif

    std::cout << "=== TCP 用户系统服务器 ===" << std::endl;
    
    // 端口配置 - 允许用户自定义监听端口
    int port = 8080;
    std::cout << "请输入服务器端口 (默认 8080): ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) {
        port = atoi(input.c_str());  // 使用atoi兼容老版本编译器
        if (port <= 0 || port > 65535) {
            std::cout << "端口号无效，使用默认端口 8080" << std::endl;
            port = 8080;
        }
    }

    // 创建服务器实例 - 只传递文件名，路径处理由服务器内部完成
    TCPUserSystemServer server(port, "users.txt");
    g_server = &server;  // 设置全局指针用于信号处理
    
    // 启动服务器 - 进入监听状态
    if (server.startServer()) {
        std::cout << "服务器启动成功!" << std::endl;
        // 注意: startServer()会阻塞在这里直到服务器停止
    } else {
        std::cout << "服务器启动失败!" << std::endl;
        return 1;
    }
    
    g_server = 0;  // 清除全局指针
    return 0;
}