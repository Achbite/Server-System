/*
 * TCP用户系统 - 服务器核心实现
 * 
 * 文件结构:
 * 1. 协议消息处理 - 实现自定义通信协议的解析和序列化
 * 2. 服务器生命周期管理 - 网络初始化、启动监听、资源清理
 * 3. 多线程客户端处理 - 为每个连接创建独立线程处理
 * 4. 用户管理业务逻辑 - 注册、登录、密码修改等核心功能
 * 5. 数据持久化 - CSV格式文件读写，实现用户数据的持久存储
 * 6. 网络通信 - 可靠的消息发送接收机制，支持超时处理
 * 
 * 技术实现:
 * - 基于TCP的自定义文本协议
 * - 多线程并发处理客户端连接
 * - 线程安全的用户数据管理
 * - 实时的操作日志记录
 * - 优雅的服务器关闭处理
 */

#include "../Public/TCP_System.h"
#include <ctime>
#include <cstdlib>
#include <sys/stat.h> // mkdir
#include <sstream>   // stringstream

// 创建目录的辅助函数
bool createDirectory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// 日志管理类实现
ServerLogger::ServerLogger(const std::string& filename, bool consoleOutput) 
    : logFile(filename), enableConsoleOutput(consoleOutput) {
    // 从文件路径中提取目录并创建
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string logDir = filename.substr(0, lastSlash);
        createDirectory(logDir);
    }
    logServerEvent("服务器日志系统初始化");
}

ServerLogger::~ServerLogger() {
    logServerEvent("服务器日志系统关闭");
}

// 获取当前时间字符串
std::string ServerLogger::getCurrentTime() {
    time_t now = time(0);
    char buffer[80];
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

// 写入日志的核心方法
void ServerLogger::writeLog(const std::string& level, const std::string& message) {
    SimpleLockGuard lock(logMutex);
    
    std::string logEntry = "[" + getCurrentTime() + "] [" + level + "] " + message;
    
    // 输出到控制台
    if (enableConsoleOutput) {
        std::cout << logEntry << std::endl;
    }
    
    // 写入日志文件
    std::ofstream file(logFile.c_str(), std::ios::app);
    if (file.is_open()) {
        file << logEntry << std::endl;
        file.close();
    }
}

// 信息级别日志
void ServerLogger::logInfo(const std::string& message) {
    writeLog("INFO", message);
}

// 警告级别日志
void ServerLogger::logWarning(const std::string& message) {
    writeLog("WARN", message);
}

// 错误级别日志
void ServerLogger::logError(const std::string& message) {
    writeLog("ERROR", message);
}

// 用户操作日志
void ServerLogger::logUserOperation(const std::string& sessionId, const std::string& userId, const std::string& operation, const std::string& result) {
    std::string message = "会话[" + sessionId.substr(0, 8) + "] 用户[" + userId + "] 操作[" + operation + "] 结果[" + result + "]";
    writeLog("USER", message);
}

// 服务器事件日志
void ServerLogger::logServerEvent(const std::string& event) {
    writeLog("SERVER", event);
}

// 协议消息解析实现 - 解析"COMMAND|param1|param2"格式的消息
ProtocolMessage ProtocolMessage::parse(const std::string& message) {
    ProtocolMessage msg;
    std::istringstream iss(message);
    std::string part;
    
    if (std::getline(iss, msg.command, '|')) {
        while (std::getline(iss, part, '|')) {
            msg.parameters.push_back(part);
        }
    }
    
    return msg;
}

// 协议消息序列化 - 将消息对象转换为传输格式
std::string ProtocolMessage::serialize() const {
    std::string result = command;
    for (size_t i = 0; i < parameters.size(); ++i) {
        result += "|" + parameters[i];
    }
    return result;
}

// 服务器构造函数 - 初始化服务器状态并加载历史数据
TCPUserSystemServer::TCPUserSystemServer(int serverPort, const std::string& filename) 
    : serverSocket(INVALID_SOCKET), running(false), port(serverPort), dataFile(filename) {
    // 确保当前目录下的log和users目录存在
    createDirectory("log");
    createDirectory("users");
    
    // 设置用户数据文件路径
    dataFile = "users/" + filename;
    
    // 初始化日志系统，日志文件存放在当前目录的log目录
    logger = new ServerLogger("log/server.log", true);
    
    std::stringstream ss;
    ss << serverPort;
    logger->logServerEvent("TCP用户系统服务器初始化，端口: " + ss.str());
    logger->logInfo("数据文件路径: " + dataFile);
    
    loadFromFile();  // 启动时加载用户数据
    
    std::stringstream userCount;
    userCount << users.size();
    logger->logInfo("用户数据加载完成，当前用户数量: " + userCount.str());
}

// 服务器析构函数 - 确保资源正确释放
TCPUserSystemServer::~TCPUserSystemServer() {
    if (logger) {
        logger->logServerEvent("服务器正在关闭...");
    }
    stopServer();       // 停止服务器
    saveToFile();       // 保存数据
    if (logger) {
        logger->logInfo("用户数据已保存");
        delete logger;
        logger = 0;
    }
    cleanupNetwork();   // 清理网络资源
}

// 网络环境初始化 - Windows需要WSAStartup
bool TCPUserSystemServer::initializeNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup 失败" << std::endl;
        return false;
    }
#endif
    return true;
}

// 网络资源清理
void TCPUserSystemServer::cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// 服务器启动 - 创建监听套接字并进入主循环
bool TCPUserSystemServer::startServer() {
    if (!initializeNetwork()) {
        logger->logError("网络初始化失败");
        return false;
    }

    // 创建TCP套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        logger->logError("创建服务器套接字失败");
        return false;
    }

    // 设置套接字选项 - 允许地址重用
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        logger->logError("设置套接字选项失败");
        closesocket(serverSocket);
        return false;
    }

    // 绑定服务器地址和端口
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    serverAddr.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::stringstream ss;
        ss << port;
        logger->logError("绑定地址失败，端口: " + ss.str());
        closesocket(serverSocket);
        return false;
    }

    // 开始监听客户端连接
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        logger->logError("监听失败");
        closesocket(serverSocket);
        return false;
    }

    running.store(true);
    std::stringstream ss;
    ss << port;
    logger->logServerEvent("TCP用户系统服务器启动成功，端口: " + ss.str());

    // 主循环 - 接受客户端连接并创建处理线程
    while (running.load()) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            if (running.load()) {
                logger->logWarning("接受客户端连接失败");
            }
            continue;
        }

        std::stringstream clientInfo;
        clientInfo << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port);
        logger->logInfo("新客户端连接: " + clientInfo.str());

        // 为客户端创建独立处理线程
        ThreadParam* param = new ThreadParam;
        param->server = this;
        param->clientSocket = clientSocket;

#ifdef _WIN32
        HANDLE thread = CreateThread(NULL, 0, clientThreadProc, param, 0, NULL);
        if (thread) {
            clientThreads.push_back(thread);
        }
#else
        pthread_t thread;
        if (pthread_create(&thread, NULL, clientThreadProc, param) == 0) {
            clientThreads.push_back(thread);
        }
#endif
    }

    return true;
}

// 客户端处理线程入口点 - 跨平台线程函数封装
#ifdef _WIN32
DWORD WINAPI TCPUserSystemServer::clientThreadProc(LPVOID param) {
#else
void* TCPUserSystemServer::clientThreadProc(void* param) {
#endif
    ThreadParam* p = static_cast<ThreadParam*>(param);
    p->server->handleClient(p->clientSocket);
    delete p;  // 清理线程参数
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// 用户登录 - 验证用户凭据并更新会话状态，支持挤占下线
std::string TCPUserSystemServer::loginUser(SimpleSharedPtr<ClientSession> session, const std::string& userId, const std::string& password) {
    SimpleLockGuard lock(usersMutex);
    
    if (session->isLoggedIn()) {
        return "ERROR|当前会话已有用户登录";
    }

    std::map<std::string, User>::iterator it = users.find(userId);
    if (it == users.end()) {
        return "ERROR|用户不存在";
    }

    if (!it->second.verifyPassword(password)) {
        return "ERROR|密码错误";
    }

    // 检查用户是否已在其他会话中登录
    std::string existingSessionId = findUserSession(userId);
    if (!existingSessionId.empty()) {
        // 用户已在其他会话中登录，询问是否挤占下线
        return "CONFLICT|用户已在其他客户端登录|" + existingSessionId + "|是否挤占下线？(Y/N)";
    }

    session->setLoggedInUser(userId);
    return "SUCCESS|登录成功";
}

// 处理挤占登录请求
std::string TCPUserSystemServer::handleLoginConflict(SimpleSharedPtr<ClientSession> session, 
                                                     const std::string& userId, 
                                                     const std::string& password,
                                                     bool forceLogin) {
    SimpleLockGuard lock(usersMutex);
    
    if (session->isLoggedIn()) {
        return "ERROR|当前会话已有用户登录";
    }

    std::map<std::string, User>::iterator it = users.find(userId);
    if (it == users.end()) {
        return "ERROR|用户不存在";
    }

    if (!it->second.verifyPassword(password)) {
        return "ERROR|密码错误";
    }

    std::string existingSessionId = findUserSession(userId);
    if (!existingSessionId.empty()) {
        if (!forceLogin) {
            return "ERROR|登录已取消";
        }
        
        // 强制下线已存在的会话
        SimpleSharedPtr<ClientSession> existingSession;
        {
            SimpleLockGuard sessionLock(sessionsMutex);
            std::map<std::string, SimpleSharedPtr<ClientSession>>::iterator sessionIt = sessions.find(existingSessionId);
            if (sessionIt != sessions.end()) {
                existingSession = sessionIt->second;
            }
        }
        
        if (existingSession) {
            // 通知被挤占的客户端
            sendMessage(existingSession->getSocket(), "KICKED|您的账号在其他地方登录，连接已断开");
            existingSession->setLoggedInUser("");  // 清除登录状态
            existingSession->setInactive();        // 标记会话为非活跃状态
            
            std::cout << "[服务器] 用户 " << userId << " 被新会话挤占下线，原会话ID: " 
                      << existingSessionId.substr(0, 8) << std::endl;
        }
    }

    session->setLoggedInUser(userId);
    return "SUCCESS|登录成功，已挤占原会话";
}

// 查找用户当前登录的会话ID
std::string TCPUserSystemServer::findUserSession(const std::string& userId) {
    SimpleLockGuard sessionLock(sessionsMutex);
    
    for (std::map<std::string, SimpleSharedPtr<ClientSession>>::iterator it = sessions.begin(); 
         it != sessions.end(); ++it) {
        if (it->second->isLoggedIn() && it->second->getLoggedInUser() == userId) {
            return it->first;  // 返回会话ID
        }
    }
    return "";  // 未找到
}

// 单个客户端连接处理 - 管理客户端会话生命周期
void TCPUserSystemServer::handleClient(SOCKET clientSocket) {
    // 创建唯一会话
    std::string sessionId = generateSessionId();
    SimpleSharedPtr<ClientSession> session(new ClientSession(clientSocket, sessionId));
    
    logger->logInfo("创建新会话: " + sessionId);
    
    // 注册会话 - 线程安全操作
    {
        SimpleLockGuard lock(sessionsMutex);
        sessions[sessionId] = session;
    }

    // 发送欢迎消息
    sendMessage(clientSocket, "WELCOME|TCP用户系统服务器|" + sessionId);

    // 消息处理循环
    while (running.load() && session->getIsActive()) {
        std::string message = receiveMessage(clientSocket);
        if (message.empty()) {
            break;  // 客户端断开连接
        }

        processClientMessage(session, message);
    }

    // 会话结束时的清理工作
    std::string loggedInUser = session->getLoggedInUser();
    if (!loggedInUser.empty()) {
        logger->logUserOperation(sessionId, loggedInUser, "SESSION_END", "自动登出");
    }

    // 清理会话
    {
        SimpleLockGuard lock(sessionsMutex);
        sessions.erase(sessionId);
    }

    closesocket(clientSocket);
    logger->logInfo("客户端会话结束: " + sessionId);
}

// 客户端消息处理 - 解析命令并调用相应业务逻辑
void TCPUserSystemServer::processClientMessage(SimpleSharedPtr<ClientSession> session, const std::string& message) {
    ProtocolMessage msg = ProtocolMessage::parse(message);
    std::string response;
    std::string sessionId = session->getSessionId();

    // 命令分发处理
    if (msg.command == "REGISTER") {
        if (msg.parameters.size() >= 2) {
            response = registerUser(msg.parameters[0], msg.parameters[1]);
            std::string result = (response.find("SUCCESS") != std::string::npos ? "成功" : "失败");
            logger->logUserOperation(sessionId, msg.parameters[0], "REGISTER", result);
        } else {
            response = "ERROR|参数不足";
            logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 注册操作参数不足");
        }
    }
    else if (msg.command == "LOGIN") {
        if (msg.parameters.size() >= 2) {
            response = loginUser(session, msg.parameters[0], msg.parameters[1]);
            std::string result = (response.find("SUCCESS") != std::string::npos ? "成功" : 
                               (response.find("CONFLICT") != std::string::npos ? "冲突" : "失败"));
            logger->logUserOperation(sessionId, msg.parameters[0], "LOGIN", result);
        } else {
            response = "ERROR|参数不足";
            logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 登录操作参数不足");
        }
    }
    else if (msg.command == "FORCE_LOGIN") {
        if (msg.parameters.size() >= 3) {
            bool forceLogin = (msg.parameters[2] == "Y" || msg.parameters[2] == "y");
            response = handleLoginConflict(session, msg.parameters[0], msg.parameters[1], forceLogin);
            std::string result = (response.find("SUCCESS") != std::string::npos ? "成功" : "失败");
            logger->logUserOperation(sessionId, msg.parameters[0], "FORCE_LOGIN", result + (forceLogin ? "(强制)" : "(取消)"));
        } else {
            response = "ERROR|参数不足";
            logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 强制登录操作参数不足");
        }
    }
    else if (msg.command == "LOGOUT") {
        std::string userId = session->getLoggedInUser();
        response = logoutUser(session);
        logger->logUserOperation(sessionId, userId, "LOGOUT", "用户登出");
    }
    else if (msg.command == "DELETE") {
        if (msg.parameters.size() >= 2) {
            response = deleteUser(session, msg.parameters[0], msg.parameters[1]);
            std::string result = (response.find("SUCCESS") != std::string::npos ? "成功" : "失败");
            logger->logUserOperation(sessionId, msg.parameters[0], "DELETE", result);
        } else {
            response = "ERROR|参数不足";
            logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 注销账户操作参数不足");
        }
    }
    else if (msg.command == "CHANGE_PASSWORD") {
        if (msg.parameters.size() >= 2) {
            std::string userId = session->getLoggedInUser();
            response = changePassword(session, msg.parameters[0], msg.parameters[1]);
            std::string result = (response.find("SUCCESS") != std::string::npos ? "成功" : "失败");
            logger->logUserOperation(sessionId, userId, "CHANGE_PASSWORD", result);
        } else {
            response = "ERROR|参数不足";
            logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 修改密码操作参数不足");
        }
    }
    else if (msg.command == "SET_STRING") {
        if (msg.parameters.size() >= 1) {
            std::string userId = session->getLoggedInUser();
            response = setUserString(session, msg.parameters[0]);
            logger->logUserOperation(sessionId, userId, "SET_STRING", "设置用户字符串");
        } else {
            response = "ERROR|参数不足";
            logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 设置字符串操作参数不足");
        }
    }
    else if (msg.command == "GET_STRING") {
        std::string userId = session->getLoggedInUser();
        response = getUserString(session);
        logger->logUserOperation(sessionId, userId, "GET_STRING", "查看用户字符串");
    }
    else if (msg.command == "QUIT") {
        std::string userId = session->getLoggedInUser();
        response = "GOODBYE|感谢使用";
        logger->logUserOperation(sessionId, userId.empty() ? "未登录" : userId, "QUIT", "客户端退出");
        sendMessage(session->getSocket(), response);
        session->setInactive();
        return;
    }
    else {
        response = "ERROR|未知命令: " + msg.command;
        logger->logWarning("会话[" + sessionId.substr(0, 8) + "] 未知命令: " + msg.command);
    }

    sendMessage(session->getSocket(), response);
}

// 用户注册 - 检查用户名唯一性并创建新用户
std::string TCPUserSystemServer::registerUser(const std::string& userId, const std::string& password) {
    SimpleLockGuard lock(usersMutex);  // 保护用户数据访问
    
    if (users.find(userId) != users.end()) {
        return "ERROR|用户ID已存在";
    }

    if (userId.empty() || password.empty()) {
        return "ERROR|用户ID和密码不能为空";
    }

    users[userId] = User(userId, password);
    saveToFile();  // 立即持久化
    return "SUCCESS|用户注册成功";
}

// 用户登出 - 清除会话中的登录状态
std::string TCPUserSystemServer::logoutUser(SimpleSharedPtr<ClientSession> session) {
    if (!session->isLoggedIn()) {
        return "ERROR|没有用户处于登录状态";
    }

    std::string userId = session->getLoggedInUser();
    session->setLoggedInUser("");
    
    std::cout << "[服务器] 用户 " << userId << " 从会话 " 
              << session->getSessionId().substr(0, 8) << " 登出" << std::endl;
              
    return "SUCCESS|登出成功";
}

// 用户注销 - 永久删除用户账户
std::string TCPUserSystemServer::deleteUser(SimpleSharedPtr<ClientSession> session, const std::string& userId, const std::string& password) {
    SimpleLockGuard lock(usersMutex);
    
    std::map<std::string, User>::iterator it = users.find(userId);
    if (it == users.end()) {
        return "ERROR|用户不存在";
    }

    if (!it->second.verifyPassword(password)) {
        return "ERROR|密码错误";
    }

    // 如果删除的是当前登录用户，先登出
    if (session->getLoggedInUser() == userId) {
        session->setLoggedInUser("");
    }

    users.erase(it);
    saveToFile();
    return "SUCCESS|用户注销成功";
}

// 设置用户字符串 - 更新用户的自定义数据
std::string TCPUserSystemServer::setUserString(SimpleSharedPtr<ClientSession> session, const std::string& str) {
    if (!session->isLoggedIn()) {
        return "ERROR|请先登录";
    }

    SimpleLockGuard lock(usersMutex);
    std::map<std::string, User>::iterator it = users.find(session->getLoggedInUser());
    if (it != users.end()) {
        it->second.setUserString(str);
        saveToFile();
        return "SUCCESS|用户字符串已更新";
    }

    return "ERROR|用户不存在";
}

// 获取用户字符串 - 返回用户的自定义数据
std::string TCPUserSystemServer::getUserString(SimpleSharedPtr<ClientSession> session) {
    if (!session->isLoggedIn()) {
        return "ERROR|请先登录";
    }

    SimpleLockGuard lock(usersMutex);
    std::map<std::string, User>::iterator it = users.find(session->getLoggedInUser());
    if (it != users.end()) {
        return "SUCCESS|" + it->second.getUserString();
    }

    return "ERROR|用户不存在";
}

// 修改密码 - 验证旧密码后更新为新密码
std::string TCPUserSystemServer::changePassword(SimpleSharedPtr<ClientSession> session, const std::string& oldPassword, const std::string& newPassword) {
    if (!session->isLoggedIn()) {
        return "ERROR|请先登录";
    }

    if (oldPassword.empty() || newPassword.empty()) {
        return "ERROR|密码不能为空";
    }

    SimpleLockGuard lock(usersMutex);
    std::map<std::string, User>::iterator it = users.find(session->getLoggedInUser());
    if (it != users.end()) {
        if (!it->second.verifyPassword(oldPassword)) {
            return "ERROR|旧密码错误";
        }
        
        it->second.setPassword(newPassword);
        saveToFile();
        return "SUCCESS|密码修改成功";
    }

    return "ERROR|用户不存在";
}

// 生成会话ID - 创建16位十六进制随机字符串
std::string TCPUserSystemServer::generateSessionId() {
    srand(static_cast<unsigned int>(time(0)));
    std::string sessionId;
    for (int i = 0; i < 16; ++i) {
        sessionId += "0123456789ABCDEF"[rand() % 16];
    }
    return sessionId;
}

// 发送消息到客户端 - 确保完整发送所有数据
bool TCPUserSystemServer::sendMessage(SOCKET socket, const std::string& message) {
    std::string fullMessage = message + "\n";  // 添加消息结束符
    int totalSent = 0;
    int messageLength = static_cast<int>(fullMessage.length());
    
    while (totalSent < messageLength) {
        int sent = send(socket, fullMessage.c_str() + totalSent, messageLength - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            return false;
        }
        totalSent += sent;
    }
    
    return true;
}

// 接收客户端消息 - 支持超时和完整消息接收
std::string TCPUserSystemServer::receiveMessage(SOCKET socket) {
    char buffer[1024];
    std::string message;
    
    // 设置接收超时
#ifdef _WIN32
    DWORD timeout = 30000;  // 30秒超时
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
    
    while (true) {
        int received = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            return "";  // 连接断开或超时
        }
        
        buffer[received] = '\0';
        message += buffer;
        
        // 检查消息完整性(以换行符结尾)
        size_t pos = message.find('\n');
        if (pos != std::string::npos) {
            return message.substr(0, pos);
        }
        
        // 防止消息过长攻击
        if (message.length() > 4096) {
            return "";
        }
    }
}

// 保存用户数据到文件 - CSV格式持久化存储
void TCPUserSystemServer::saveToFile() {
    std::ofstream file(dataFile.c_str());
    if (!file.is_open()) {
        std::cerr << "警告: 无法保存用户数据到文件: " << dataFile << std::endl;
        return;
    }

    for (std::map<std::string, User>::const_iterator it = users.begin(); 
         it != users.end(); ++it)
    {
        file << it->second.serialize() << std::endl;
    }
    
    if (file.fail()) {
        std::cerr << "警告: 保存用户数据时发生错误" << std::endl;
    }
    
    file.close();
}

// 从文件加载用户数据 - 服务器启动时恢复历史数据
void TCPUserSystemServer::loadFromFile() {
    std::ifstream file(dataFile.c_str());
    if (!file.is_open()) {
        return;  // 文件不存在是正常的(首次运行)
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            User user = User::deserialize(line);
            users[user.getUserId()] = user;
        }
    }
    file.close();
}

// 停止服务器 - 优雅关闭所有连接和线程
void TCPUserSystemServer::stopServer() {
    if (running.load()) {
        running.store(false);
        
        if (logger) {
            logger->logServerEvent("服务器正在停止...");
        }
        
        // 关闭监听套接字
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
        
        // 等待所有客户端线程结束
#ifdef _WIN32
        for (size_t i = 0; i < clientThreads.size(); ++i) {
            if (clientThreads[i] != NULL) {
                WaitForSingleObject(clientThreads[i], 5000);  // 等待5秒
                CloseHandle(clientThreads[i]);
            }
        }
#else
        for (size_t i = 0; i < clientThreads.size(); ++i) {
            pthread_join(clientThreads[i], NULL);
        }
#endif
        clientThreads.clear();
        
        if (logger) {
            logger->logServerEvent("服务器已停止");
        }
    }
}