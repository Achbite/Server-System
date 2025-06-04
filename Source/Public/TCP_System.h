/*
 * TCP用户系统 - 核心头文件
 * 
 * 文件结构:
 * 1. 平台兼容性处理 - 跨平台网络编程支持(Windows/Linux)
 * 2. 自定义同步原语 - 替代C++11标准库实现线程安全
 *    - SimpleAtomicBool: 原子布尔操作
 *    - SimpleMutex: 互斥锁
 *    - SimpleLockGuard: RAII锁管理
 *    - SimpleSharedPtr: 智能指针实现
 * 3. 核心业务类 - 用户管理和网络通信
 *    - User: 用户数据模型，支持序列化/反序列化
 *    - ClientSession: 客户端会话管理
 *    - TCPUserSystemServer: 服务器核心类，多线程处理客户端连接
 *    - ProtocolMessage: 协议消息解析
 * 
 * 技术特点:
 * - 兼容C++11及以上版本
 * - 跨平台网络编程(WinSock2/Linux Socket)
 * - 多线程安全设计
 * - 自定义轻量级同步机制
 */

#ifndef TCP_USER_SYSTEM_H
#define TCP_USER_SYSTEM_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
#include <ctime>
#include <stdexcept>

// 平台兼容性处理 - 统一Windows和Linux的网络编程接口
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #ifndef INADDR_NONE
        #define INADDR_NONE 0xFFFFFFFF
    #endif
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <pthread.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

// 原子布尔类 - 替代std::atomic<bool>，确保多线程安全的布尔操作
class SimpleAtomicBool {
private:
    volatile bool value;
#ifdef _WIN32
    CRITICAL_SECTION cs;    // Windows临界区
#else
    pthread_mutex_t mutex;  // Linux互斥锁
#endif

public:
    SimpleAtomicBool(bool initial = false) : value(initial) {
#ifdef _WIN32
        InitializeCriticalSection(&cs);
#else
        pthread_mutex_init(&mutex, NULL);
#endif
    }
    
    ~SimpleAtomicBool() {
#ifdef _WIN32
        DeleteCriticalSection(&cs);
#else
        pthread_mutex_destroy(&mutex);
#endif
    }
    
    // 原子读取操作
    bool load() const {
#ifdef _WIN32
        EnterCriticalSection(const_cast<CRITICAL_SECTION*>(&cs));
        bool result = value;
        LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&cs));
        return result;
#else
        pthread_mutex_lock(const_cast<pthread_mutex_t*>(&mutex));
        bool result = value;
        pthread_mutex_unlock(const_cast<pthread_mutex_t*>(&mutex));
        return result;
#endif
    }
    
    // 原子写入操作
    void store(bool newValue) {
#ifdef _WIN32
        EnterCriticalSection(&cs);
        value = newValue;
        LeaveCriticalSection(&cs);
#else
        pthread_mutex_lock(&mutex);
        value = newValue;
        pthread_mutex_unlock(&mutex);
#endif
    }
};

// 简单互斥锁类 - 替代std::mutex，提供跨平台锁机制
class SimpleMutex {
private:
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t mutex;
#endif

public:
    SimpleMutex() {
#ifdef _WIN32
        InitializeCriticalSection(&cs);
#else
        pthread_mutex_init(&mutex, NULL);
#endif
    }
    
    ~SimpleMutex() {
#ifdef _WIN32
        DeleteCriticalSection(&cs);
#else
        pthread_mutex_destroy(&mutex);
#endif
    }
    
    void lock() {
#ifdef _WIN32
        EnterCriticalSection(&cs);
#else
        pthread_mutex_lock(&mutex);
#endif
    }
    
    void unlock() {
#ifdef _WIN32
        LeaveCriticalSection(&cs);
#else
        pthread_mutex_unlock(&mutex);
#endif
    }
};

// RAII锁守卫 - 替代std::lock_guard，自动管理锁的生命周期
class SimpleLockGuard {
private:
    SimpleMutex& mutex_;
public:
    SimpleLockGuard(SimpleMutex& m) : mutex_(m) {
        mutex_.lock();
    }
    
    ~SimpleLockGuard() {
        mutex_.unlock();
    }
};

// 前置声明
class ClientSession;

// 日志管理类 - 记录服务器操作和异常情况
class ServerLogger {
private:
    std::string logFile;         // 日志文件路径
    SimpleMutex logMutex;        // 日志写入保护
    bool enableConsoleOutput;    // 是否同时输出到控制台

public:
    ServerLogger(const std::string& filename = "server.log", bool consoleOutput = true);
    ~ServerLogger();
    
    // 日志记录方法
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    void logUserOperation(const std::string& sessionId, const std::string& userId, const std::string& operation, const std::string& result);
    void logServerEvent(const std::string& event);
    
private:
    void writeLog(const std::string& level, const std::string& message);
    std::string getCurrentTime();
};

// 简单智能指针 - 替代std::shared_ptr，实现引用计数管理
template<typename T>
class SimpleSharedPtr {
private:
    T* ptr;
    int* ref_count;

public:
    SimpleSharedPtr() : ptr(0), ref_count(0) {}
    
    explicit SimpleSharedPtr(T* p) : ptr(p), ref_count(new int(1)) {}
    
    // 拷贝构造 - 增加引用计数
    SimpleSharedPtr(const SimpleSharedPtr& other) : ptr(other.ptr), ref_count(other.ref_count) {
        if (ref_count) ++(*ref_count);
    }
    
    // 析构函数 - 减少引用计数，计数为0时释放资源
    ~SimpleSharedPtr() {
        if (ref_count && --(*ref_count) == 0) {
            delete ptr;
            delete ref_count;
        }
    }
    
    // 赋值操作 - 正确处理引用计数转移
    SimpleSharedPtr& operator=(const SimpleSharedPtr& other) {
        if (this != &other) {
            if (ref_count && --(*ref_count) == 0) {
                delete ptr;
                delete ref_count;
            }
            ptr = other.ptr;
            ref_count = other.ref_count;
            if (ref_count) ++(*ref_count);
        }
        return *this;
    }
    
    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }
    T* get() const { return ptr; }
    bool operator!() const { return ptr == 0; }
    operator bool() const { return ptr != 0; }
};

// 用户数据模型 - 封装用户信息，支持数据持久化
class User {
private:
    std::string userId;      // 用户唯一标识
    std::string password;    // 用户密码
    std::string userString;  // 用户自定义字符串

public:
    User() {}
    User(const std::string& id, const std::string& pwd) 
        : userId(id), password(pwd), userString("") {}

    std::string getUserId() const { return userId; }
    std::string getPassword() const { return password; }
    std::string getUserString() const { return userString; }

    void setUserString(const std::string& str) { userString = str; }
    void setPassword(const std::string& pwd) { password = pwd; }
    
    // 密码验证 - 简单明文比较(实际应用应使用哈希)
    bool verifyPassword(const std::string& pwd) const {
        return password == pwd;
    }

    // 数据序列化 - 转换为CSV格式用于文件存储
    std::string serialize() const {
        return userId + "," + password + "," + userString;
    }

    // 数据反序列化 - 从CSV格式恢复用户对象
    static User deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string id, pwd, str;
        
        std::getline(iss, id, ',');
        std::getline(iss, pwd, ',');
        std::getline(iss, str);
        
        User user(id, pwd);
        user.setUserString(str);
        return user;
    }
};

// 客户端会话管理 - 维护单个客户端连接状态
class ClientSession {
private:
    SOCKET clientSocket;         // 客户端套接字
    std::string sessionId;       // 会话唯一标识
    std::string loggedInUser;    // 当前登录用户ID
    bool isActive;              // 会话活跃状态

public:
    ClientSession(SOCKET socket, const std::string& id) 
        : clientSocket(socket), sessionId(id), loggedInUser(""), isActive(true) {}

    SOCKET getSocket() const { return clientSocket; }
    std::string getSessionId() const { return sessionId; }
    std::string getLoggedInUser() const { return loggedInUser; }
    bool getIsActive() const { return isActive; }

    void setLoggedInUser(const std::string& user) { loggedInUser = user; }
    void setInactive() { isActive = false; }
    bool isLoggedIn() const { return !loggedInUser.empty(); }
};

// TCP用户系统服务器核心类 - 多线程网络服务器实现
class TCPUserSystemServer {
private:
    // 网络相关
    SOCKET serverSocket;          // 服务器监听套接字
    SimpleAtomicBool running;     // 服务器运行状态标志
    int port;                     // 监听端口
    std::string dataFile;         // 用户数据文件路径
    
    // 日志管理
    ServerLogger* logger;         // 日志记录器
    
    // 数据管理 - 使用map确保有序性和查找效率
    std::map<std::string, User> users;                              // 用户数据存储
    std::map<std::string, SimpleSharedPtr<ClientSession> > sessions; // 活跃会话管理
    SimpleMutex usersMutex;       // 用户数据访问保护
    SimpleMutex sessionsMutex;    // 会话数据访问保护
    
    // 线程管理 - 为每个客户端连接创建独立处理线程
#ifdef _WIN32
    std::vector<HANDLE> clientThreads;      // Windows线程句柄
#else
    std::vector<pthread_t> clientThreads;   // Linux线程ID
#endif

public:
    TCPUserSystemServer(int serverPort = 8080, const std::string& filename = "users.txt");
    ~TCPUserSystemServer();

    // 服务器生命周期管理
    bool startServer();          // 启动服务器监听
    void stopServer();           // 停止服务器并清理资源
    bool isRunning() const { return running.load(); }

    // 客户端连接处理
    void handleClient(SOCKET clientSocket);        // 单个客户端处理入口
    void processClientMessage(SimpleSharedPtr<ClientSession> session, const std::string& message);

    // 用户管理功能 - 核心业务逻辑
    std::string registerUser(const std::string& userId, const std::string& password);
    std::string loginUser(SimpleSharedPtr<ClientSession> session, const std::string& userId, const std::string& password);
    std::string logoutUser(SimpleSharedPtr<ClientSession> session);
    std::string deleteUser(SimpleSharedPtr<ClientSession> session, const std::string& userId, const std::string& password);
    std::string changePassword(SimpleSharedPtr<ClientSession> session, const std::string& oldPassword, const std::string& newPassword);

    // 登录冲突处理 - 支持用户挤占下线功能
    std::string handleLoginConflict(SimpleSharedPtr<ClientSession> session, 
                                   const std::string& userId, 
                                   const std::string& password,
                                   bool forceLogin);
    std::string findUserSession(const std::string& userId);

    // 用户数据操作
    std::string setUserString(SimpleSharedPtr<ClientSession> session, const std::string& str);
    std::string getUserString(SimpleSharedPtr<ClientSession> session);

    // 工具函数
    std::string generateSessionId();              // 生成唯一会话ID
    bool sendMessage(SOCKET socket, const std::string& message);    // 发送消息到客户端
    std::string receiveMessage(SOCKET socket);   // 接收客户端消息

    // 数据持久化 - 文件读写操作
    void saveToFile();          // 保存用户数据到文件
    void loadFromFile();        // 从文件加载用户数据

    // 网络初始化
    bool initializeNetwork();   // 初始化网络环境
    void cleanupNetwork();      // 清理网络资源
    
    // 多线程处理函数
#ifdef _WIN32
    static DWORD WINAPI clientThreadProc(LPVOID param);
#else
    static void* clientThreadProc(void* param);
#endif
};

// 线程参数传递结构
struct ThreadParam {
    TCPUserSystemServer* server;
    SOCKET clientSocket;
};

// 协议消息结构 - 定义客户端与服务器通信格式
struct ProtocolMessage {
    std::string command;                    // 命令类型
    std::vector<std::string> parameters;    // 命令参数列表
    
    // 消息解析 - 从"COMMAND|param1|param2"格式解析
    static ProtocolMessage parse(const std::string& message);
    // 消息序列化 - 转换为传输格式
    std::string serialize() const;
};

#endif