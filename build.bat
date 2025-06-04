REM TCP用户系统 - 编译脚本
REM 
REM 功能说明:
REM 1. 设置UTF-8编码确保中文正确显示
REM 2. 创建必要的目录结构
REM 3. 编译服务器和客户端程序
REM 4. 错误检查和编译状态报告
REM 5. 生成可执行文件: bin/tcp_server.exe, bin/tcp_client.exe

@echo off
chcp 65001 >nul 2>&1

REM 创建项目目录结构
if not exist bin (
    mkdir bin
    echo 创建 bin 目录...
)


REM 检查g++是否可用
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: 找不到g++编译器!
    echo 请确保MinGW64已正确安装并添加到PATH环境变量中
    pause
    exit /b 1
)

echo 正在编译TCP用户系统...
echo 使用编译器: 
g++ --version | findstr "g++"

echo.
echo 编译服务器...
g++ -std=c++11 -I. -o bin/tcp_server.exe main.cpp Source/Private/TCP_System.cpp -lws2_32
if %errorlevel% neq 0 (
    echo 服务器编译失败!
    pause
    exit /b 1
)
echo 服务器编译成功!

echo.
echo 编译客户端...
g++ -std=c++11 -I. -o bin/tcp_client.exe Source/Private/Client.cpp Source/Private/TCP_System.cpp -lws2_32
if %errorlevel% neq 0 (
    echo 客户端编译失败!
    pause
    exit /b 1
)
echo 客户端编译成功!

echo.
echo 编译完成!
echo 运行服务器: bin/tcp_server.exe
echo 运行客户端: bin/tcp_client.exe
echo.
pause
