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

#include "Public/TCP_System.h"
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
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

    // 创建服务器实例 - 自动加载历史用户数据
    TCPUserSystemServer server(port);
    
    // 启动服务器 - 进入监听状态
    if (server.startServer()) {
        std::cout << "服务器启动成功!" << std::endl;
        // 注意: startServer()会阻塞在这里直到服务器停止
    } else {
        std::cout << "服务器启动失败!" << std::endl;
        return 1;
    }
    
    return 0;
}