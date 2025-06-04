/*
 * TCP用户系统 - 客户端实现
 * 
 * 文件结构:
 * 1. 网络连接管理 - TCP客户端连接建立、断开和错误处理
 * 2. 用户界面系统 - 分层界面设计(登录前/登录后)
 * 3. 消息通信 - 与服务器的可靠消息收发机制
 * 4. 业务流程控制 - 登录验证、用户操作流程管理
 * 5. 跨平台编码处理 - Windows中文显示支持
 * 
 * 界面设计:
 * - 登录前界面: 登录、注册、退出
 * - 登录后界面: 查看/修改字符串、修改密码、注销账户、登出
 * 
 * 通信协议:
 * - 基于TCP的文本协议
 * - 消息格式: "COMMAND|param1|param2"
 * - 自动重连和错误恢复机制
 */

#include "../Public/TCP_System.h"
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

// 跨平台清屏函数
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// TCP客户端类 - 管理与服务器的连接和通信
class TCPUserClient {
private:
    SOCKET clientSocket;       // 客户端套接字
    std::string serverAddress; // 服务器地址
    int serverPort;           // 服务器端口
    bool connected;           // 连接状态标志

public:
    TCPUserClient(const std::string& addr = "127.0.0.1", int port = 8080) 
        : clientSocket(INVALID_SOCKET), serverAddress(addr), serverPort(port), connected(false) {}

    ~TCPUserClient() {
        disconnect();
    }

    // 连接到服务器 - 建立TCP连接并初始化通信
    bool connect() {
#ifdef _WIN32
        // Windows网络初始化
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup 失败" << std::endl;
            return false;
        }
#endif

        // 创建客户端套接字
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "创建客户端套接字失败" << std::endl;
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        // 配置服务器地址结构
        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<unsigned short>(serverPort));
        
#ifdef _WIN32
        // Windows地址转换
        unsigned long addr = inet_addr(serverAddress.c_str());
        if (addr == INADDR_NONE) {
            std::cerr << "无效的服务器地址" << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }
        serverAddr.sin_addr.s_addr = addr;
#else
        // Linux地址转换
        if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "无效的服务器地址" << std::endl;
            closesocket(clientSocket);
            return false;
        }
#endif

        // 连接到服务器
        if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "连接服务器失败" << std::endl;
            closesocket(clientSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        connected = true;
        
        // 接收服务器欢迎消息
        std::string welcome = receiveMessage();
        std::cout << "服务器消息: " << welcome << std::endl;
        
        return true;
    }

    // 断开连接 - 优雅关闭连接并清理资源
    void disconnect() {
        if (connected && clientSocket != INVALID_SOCKET) {
            sendMessage("QUIT");  // 通知服务器客户端退出
            connected = false;
        }
        
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }
        
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // 发送消息到服务器 - 确保消息完整发送
    bool sendMessage(const std::string& message) {
        if (!connected || clientSocket == INVALID_SOCKET) return false;
        
        std::string fullMessage = message + "\n";  // 添加消息结束符
        int totalSent = 0;
        int messageLength = static_cast<int>(fullMessage.length());
        
        while (totalSent < messageLength) {
            int sent = send(clientSocket, fullMessage.c_str() + totalSent, messageLength - totalSent, 0);
            if (sent == SOCKET_ERROR) {
                connected = false;
                return false;
            }
            totalSent += sent;
        }
        
        return true;
    }

    // 接收服务器消息 - 处理网络延迟和数据分片
    std::string receiveMessage() {
        if (!connected || clientSocket == INVALID_SOCKET) return "";
        
        char buffer[1024];
        std::string message;
        
        while (true) {
            int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) {
                connected = false;
                return "";
            }
            
            buffer[received] = '\0';
            message += buffer;
            
            // 检查消息完整性
            size_t pos = message.find('\n');
            if (pos != std::string::npos) {
                return message.substr(0, pos);
            }
            
            // 防止消息过长
            if (message.length() > 4096) {
                return "";
            }
        }
    }

    // 非阻塞接收消息 - 检查是否有待处理的消息
    std::string receiveMessageNonBlocking() {
        if (!connected || clientSocket == INVALID_SOCKET) return "";
        
        // 设置非阻塞模式
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(clientSocket, FIONBIO, &mode);
#else
        int flags = fcntl(clientSocket, F_GETFL, 0);
        fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
#endif
        
        char buffer[1024];
        int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        // 恢复阻塞模式
#ifdef _WIN32
        mode = 0;
        ioctlsocket(clientSocket, FIONBIO, &mode);
#else
        int flags2 = fcntl(clientSocket, F_GETFL, 0);
        fcntl(clientSocket, F_SETFL, flags2 & ~O_NONBLOCK);
#endif
        
        if (received > 0) {
            buffer[received] = '\0';
            std::string message(buffer);
            size_t pos = message.find('\n');
            if (pos != std::string::npos) {
                return message.substr(0, pos);
            }
            return message;
        }
        
        return "";
    }

    // 检查是否被踢下线
    bool checkKicked() {
        std::string message = receiveMessageNonBlocking();
        if (!message.empty() && message.find("KICKED") != std::string::npos) {
            std::cout << "\n=== 系统通知 ===" << std::endl;
            std::cout << "您的账号在其他地方登录，连接已断开!" << std::endl;
            std::cout << "即将返回登录界面..." << std::endl;
            return true;
        }
        return false;
    }

    // 显示登录前菜单
    void printLoginMenu() {
        clearScreen();
        std::cout << "\n=== TCP 用户系统 ===" << std::endl;
        std::cout << "1. 用户登录" << std::endl;
        std::cout << "2. 用户注册" << std::endl;
        std::cout << "0. 退出系统" << std::endl;
        std::cout << "请选择操作: ";
    }

    // 显示登录后用户操作菜单
    void printUserMenu() {
        clearScreen();
        std::cout << "\n=== 用户操作界面 ===" << std::endl;
        std::cout << "1. 查看用户字符串" << std::endl;
        std::cout << "2. 修改用户字符串" << std::endl;
        std::cout << "3. 修改密码" << std::endl;
        std::cout << "4. 注销账户" << std::endl;
        std::cout << "5. 登出" << std::endl;
        std::cout << "0. 退出系统" << std::endl;
        std::cout << "请选择操作: ";
    }

    // 登录阶段处理 - 处理用户登录和注册操作
    bool loginPhase() {
        int choice;
        std::string userId, password;

        while (connected) {
            printLoginMenu();
            
            // 输入验证
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "输入无效，请输入数字!" << std::endl;
                std::cout << "按回车键继续...";
                std::cin.get();
                continue;
            }
            std::cin.ignore();

            switch (choice) {
                case 1: // 用户登录
                    std::cout << "请输入用户ID: ";
                    std::getline(std::cin, userId);
                    std::cout << "请输入密码: ";
                    std::getline(std::cin, password);
                    if (sendMessage("LOGIN|" + userId + "|" + password)) {
                        std::string response = receiveMessage();
                        std::cout << "服务器响应: " << response << std::endl;
                        
                        if (response.find("SUCCESS") != std::string::npos) {
                            std::cout << "登录成功! 欢迎 " << userId << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return true;  // 进入用户操作界面
                        }
                        else if (response.find("CONFLICT") != std::string::npos) {
                            // 处理登录冲突 - 用户已在其他地方登录
                            std::cout << "检测到该用户已在其他客户端登录!" << std::endl;
                            std::cout << "是否强制登录并挤占原会话? (Y/N): ";
                            std::string choice;
                            std::getline(std::cin, choice);
                            
                            if (sendMessage("FORCE_LOGIN|" + userId + "|" + password + "|" + choice)) {
                                std::string forceResponse = receiveMessage();
                                std::cout << "服务器响应: " << forceResponse << std::endl;
                                
                                if (forceResponse.find("SUCCESS") != std::string::npos) {
                                    std::cout << "强制登录成功! 欢迎 " << userId << std::endl;
                                    std::cout << "按回车键继续...";
                                    std::cin.get();
                                    return true;  // 进入用户操作界面
                                }
                            }
                        }
                    }
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;

                case 2: // 用户注册
                    std::cout << "请输入用户ID: ";
                    std::getline(std::cin, userId);
                    std::cout << "请输入密码: ";
                    std::getline(std::cin, password);
                    if (sendMessage("REGISTER|" + userId + "|" + password)) {
                        std::string response = receiveMessage();
                        std::cout << "服务器响应: " << response << std::endl;
                        if (response.find("SUCCESS") != std::string::npos) {
                            std::cout << "注册成功! 请使用新账户登录。" << std::endl;
                        }
                    }
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;

                case 0: // 退出系统
                    std::cout << "感谢使用!" << std::endl;
                    return false;

                default:
                    std::cout << "无效选择!" << std::endl;
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;
            }
        }
        return false;
    }

    // 用户操作阶段 - 处理登录后的用户功能
    bool userPhase() {
        int choice;
        std::string userString, oldPassword, newPassword, confirmPassword;

        while (connected) {
            // 检查是否被踢下线
            if (checkKicked()) {
                std::cout << "按回车键返回登录界面...";
                std::cin.get();
                return false;  // 返回登录界面
            }

            printUserMenu();
            
            // 输入验证
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "输入无效，请输入数字!" << std::endl;
                std::cout << "按回车键继续...";
                std::cin.get();
                continue;
            }
            std::cin.ignore();

            switch (choice) {
                case 1: // 查看用户字符串
                    if (sendMessage("GET_STRING")) {
                        std::string response = receiveMessage();
                        if (response.find("KICKED") != std::string::npos) {
                            std::cout << "\n=== 系统通知 ===" << std::endl;
                            std::cout << "您的账号在其他地方登录，连接已断开!" << std::endl;
                            std::cout << "即将返回登录界面..." << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return false;  // 返回登录界面
                        }
                        if (response.find("SUCCESS") != std::string::npos) {
                            std::cout << "您的字符串: " << response.substr(8) << std::endl; // 去掉"SUCCESS|"前缀
                        } else {
                            std::cout << "服务器响应: " << response << std::endl;
                        }
                    }
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;

                case 2: // 修改用户字符串
                    std::cout << "请输入新的字符串: ";
                    std::getline(std::cin, userString);
                    if (sendMessage("SET_STRING|" + userString)) {
                        std::string response = receiveMessage();
                        if (response.find("KICKED") != std::string::npos) {
                            std::cout << "\n=== 系统通知 ===" << std::endl;
                            std::cout << "您的账号在其他地方登录，连接已断开!" << std::endl;
                            std::cout << "即将返回登录界面..." << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return false;  // 返回登录界面
                        }
                        std::cout << "服务器响应: " << response << std::endl;
                    }
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;

                case 3: // 修改密码 - 双重验证确保安全
                    std::cout << "请输入当前密码: ";
                    std::getline(std::cin, oldPassword);
                    std::cout << "请输入新密码: ";
                    std::getline(std::cin, newPassword);
                    std::cout << "请确认新密码: ";
                    std::getline(std::cin, confirmPassword);
                    
                    if (newPassword != confirmPassword) {
                        std::cout << "两次输入的密码不一致!" << std::endl;
                        std::cout << "按回车键继续...";
                        std::cin.get();
                        break;
                    }
                    
                    if (sendMessage("CHANGE_PASSWORD|" + oldPassword + "|" + newPassword)) {
                        std::string response = receiveMessage();
                        if (response.find("KICKED") != std::string::npos) {
                            std::cout << "\n=== 系统通知 ===" << std::endl;
                            std::cout << "您的账号在其他地方登录，连接已断开!" << std::endl;
                            std::cout << "即将返回登录界面..." << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return false;  // 返回登录界面
                        }
                        std::cout << "服务器响应: " << response << std::endl;
                    }
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;

                case 4: // 注销账户 - 需要用户确认的危险操作
                    std::cout << "警告: 此操作将永久删除您的账户!" << std::endl;
                    std::cout << "请输入您的用户ID确认: ";
                    std::getline(std::cin, userString);
                    std::cout << "请输入密码确认: ";
                    std::getline(std::cin, confirmPassword);
                    
                    if (sendMessage("DELETE|" + userString + "|" + confirmPassword)) {
                        std::string response = receiveMessage();
                        if (response.find("KICKED") != std::string::npos) {
                            std::cout << "\n=== 系统通知 ===" << std::endl;
                            std::cout << "您的账号在其他地方登录，连接已断开!" << std::endl;
                            std::cout << "即将返回登录界面..." << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return false;  // 返回登录界面
                        }
                        std::cout << "服务器响应: " << response << std::endl;
                        if (response.find("SUCCESS") != std::string::npos) {
                            std::cout << "账户已注销，即将返回登录界面..." << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return false;  // 返回登录界面
                        }
                    }
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;

                case 5: // 用户登出
                    if (sendMessage("LOGOUT")) {
                        std::string response = receiveMessage();
                        if (response.find("KICKED") != std::string::npos) {
                            std::cout << "\n=== 系统通知 ===" << std::endl;
                            std::cout << "您的账号在其他地方登录，连接已断开!" << std::endl;
                            std::cout << "即将返回登录界面..." << std::endl;
                            std::cout << "按回车键继续...";
                            std::cin.get();
                            return false;  // 返回登录界面
                        }
                        std::cout << "服务器响应: " << response << std::endl;
                        std::cout << "已登出，返回登录界面..." << std::endl;
                        std::cout << "按回车键继续...";
                        std::cin.get();
                        return false;  // 返回登录界面
                    }
                    break;

                case 0: // 退出系统
                    if (sendMessage("QUIT")) {
                        std::string response = receiveMessage();
                        if (response.find("KICKED") == std::string::npos) {
                            std::cout << "服务器响应: " << response << std::endl;
                        }
                    }
                    connected = false;
                    return true;  // 结束程序
                    
                default:
                    std::cout << "无效选择!" << std::endl;
                    std::cout << "按回车键继续...";
                    std::cin.get();
                    break;
            }
            
            // 操作完成后再次检查是否被踢下线
            if (checkKicked()) {
                std::cout << "按回车键返回登录界面...";
                std::cin.get();
                return false;  // 返回登录界面
            }
        }
        return true;
    }

    // 主运行循环 - 控制程序整体流程
    void run() {
        if (!connect()) {
            std::cout << "无法连接到服务器!" << std::endl;
            return;
        }

        // 主循环: 登录阶段 -> 用户操作阶段 -> 循环或退出
        while (connected) {
            if (loginPhase()) {
                // 登录成功，进入用户操作阶段
                if (userPhase()) {
                    break;  // 用户选择退出系统
                }
                // userPhase返回false表示返回登录界面
            } else {
                break;  // 用户在登录阶段选择退出
            }
        }

        disconnect();
    }
};

// 客户端主函数 - 程序入口点，处理编码和初始化
int main() {
#ifdef _WIN32
    // Windows控制台UTF-8编码设置，确保中文正确显示
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    _setmode(_fileno(stdout), _O_TEXT);
    _setmode(_fileno(stdin), _O_TEXT);
#endif

    clearScreen();
    std::cout << "=== TCP 用户系统客户端 ===" << std::endl;
    
    // 服务器连接配置
    std::string serverAddr = "127.0.0.1";
    int serverPort = 8080;
    
    std::cout << "请输入服务器地址 (默认 127.0.0.1): ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) {
        serverAddr = input;
    }
    
    std::cout << "请输入服务器端口 (默认 8080): ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        serverPort = atoi(input.c_str());
    }

    // 创建客户端实例并启动
    TCPUserClient client(serverAddr, serverPort);
    client.run();

    return 0;
}