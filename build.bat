REM TCP用户系统 - 编译脚本
REM 
REM 功能说明:
REM 1. 设置UTF-8编码确保中文正确显示
REM 2. 编译服务器和客户端程序
REM 3. 错误检查和编译状态报告
REM 4. 生成可执行文件: tcp_server.exe, tcp_client.exe
REM 
REM 编译环境:
REM - 编译器: MinGW64 g++
REM - C++标准: C++11
REM - 链接库: ws2_32 (Windows Socket)

@echo off
chcp 65001 >nul 2>&1
echo 正在编译TCP用户系统...

echo.
echo 编译服务器...
"D:\Dev-Cpp\MinGW64\bin\g++.exe" -std=c++11 -I. -o tcp_server.exe main.cpp Private/TCP_System.cpp -lws2_32
if %errorlevel% neq 0 (
    echo 服务器编译失败!
    pause
    exit /b 1
)
echo 服务器编译成功!

echo.
echo 编译客户端...
"D:\Dev-Cpp\MinGW64\bin\g++.exe" -std=c++11 -I. -o tcp_client.exe Private/Client.cpp Private/TCP_System.cpp -lws2_32
if %errorlevel% neq 0 (
    echo 客户端编译失败!
    pause
    exit /b 1
)
echo 客户端编译成功!

echo.
echo 编译完成!
echo 运行服务器: tcp_server.exe
echo 运行客户端: tcp_client.exe
echo.
pause
