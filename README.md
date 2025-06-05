# TCP用户系统

一个基于TCP协议的多线程用户管理系统，支持用户注册、登录、数据管理等功能，具备完整的轻量化客户端-服务器架构。

具体的学习思路建议查阅根目录下的Learn More.md

注意：本项目支持使用wsl开发Linux环境，在目录文件夹启动wsl后运行make指令即可，如果是win环境则在目录文件夹下需要指定mingw编译器，指令为mingw32-make的格式
编译完成后使用cd bin进入应用文件夹，然后通过./tcp_server命令启动对应的程序即可

如果你刚接触项目编译可以直接启动build脚本，会自动帮你编译这个项目，仅限Windows系统，里面会自动帮你检测是否有g++环境，根据提示配置环境就好啦

## 📋 项目概述

TCP用户系统是一个采用C++开发的网络应用程序，实现了完整的用户账户管理功能。系统采用自定义TCP协议进行通信，支持多客户端并发连接，提供用户注册、登录、密码修改、数据存储等核心功能。

### 🚀 主要特性

- **多线程服务器架构** - 每个客户端连接独立线程处理，支持高并发
- **用户账户管理** - 注册、登录、注销、密码修改等完整功能
- **登录冲突处理** - 支持用户挤占下线机制
- **数据持久化** - CSV格式文件存储，自动保存用户数据
- **跨平台支持** - Windows/Linux/macOS 三平台兼容
- **安全会话管理** - 唯一会话ID，防止会话冲突
- **实时操作日志** - 完整的服务器操作记录和日志文件管理

## 🏗️ 系统架构

```
┌─────────────────┐    TCP连接     ┌─────────────────┐
│   客户端程序    │ ◄──────────► │   服务器程序    │
│  (tcp_client)   │              │  (tcp_server)   │
└─────────────────┘              └─────────────────┘
         │                               │
         │                               │
    ┌─────────┐                   ┌─────────────┐
    │ 用户界面 │                   │ 多线程处理   │
    │ 交互系统 │                   │ 用户请求     │
    └─────────┘                   └─────────────┘
                                          │
                                   ┌─────────────┐
                                   │ 数据持久化   │
                                   │ & 日志管理   │
                                   └─────────────┘
```

## 📁 项目结构

```
Server-System/
├── Source/
│   ├── Public/
│   │   └── TCP_System.h      # 核心头文件，类定义和平台兼容性
│   └── Private/
│       ├── TCP_System.cpp    # 服务器核心实现
│       └── Client.cpp        # 客户端实现
├── bin/                      # 编译输出和运行目录
│   ├── tcp_server            # 服务器可执行文件 (Unix/Linux/macOS/win 取决于你的编译方式)
│   ├── tcp_client            # 客户端可执行文件 (Unix/Linux/macOS/win 取决于你的编译方式)
│   ├── tcp_server_linux      # Linux专用服务器
│   ├── tcp_client_linux      # Linux专用客户端
│   ├── log/                  # 服务器日志目录 (运行时创建)
│   │   └── server.log        # 服务器运行日志
│   └── users/                # 用户数据目录 (运行时创建)
│       └── users.txt         # 用户数据文件
├── main.cpp                  # 服务器主程序入口
├── Makefile                  # 跨平台Make编译配置
├── build.bat                 # Windows批处理编译脚本
├── .gitignore                # Git忽略文件配置
├── LICENSE                   # MIT许可证
└── README.md                 # 项目说明文档
```

**注意**:

- `log/` 和 `users/` 目录在程序首次运行时会自动创建在 `bin/` 目录下
- 可执行文件根据编译平台自动生成对应版本

## 🔧 编译环境要求

### macOS环境 (开发推荐)

- **编译器**: Clang++ (Xcode Command Line Tools) 或 GCC 7.0+
- **系统**: macOS 10.14+
- **网络库**: 系统自带POSIX socket库

### Linux环境 (生产部署)

- **编译器**: GCC 7.0+ (支持C++11)
- **系统**: Ubuntu 16.04+ / CentOS 7+ / 其他主流发行版
- **网络库**: 系统自带socket库

### Windows环境

- **编译器**: MinGW64 (g++ 7.0+) 或 Visual Studio 2017+
- **系统**: Windows 7/8/10/11
- **网络库**: WinSock2 (系统自带)

## 🛠️ 编译和运行

### 方法一：使用Make (推荐开发者)

#### macOS/Linux环境

```bash
# 查看所有可用命令
make help

# Mac开发环境测试 (推荐)
make dev-test

# 编译所有目标
make all

# 单独编译
make server          # 编译服务器
make client          # 编译客户端

# 清理生成文件
make clean
```

#### Windows环境

```bash
# 使用MinGW Make
mingw32-make help
mingw32-make dev-test

# 或使用预设命令
mingw32-make all
```

### 方法二：使用批处理脚本 (Windows用户)

```batch
# 双击运行或在根目录执行
build.bat
```

### 跨平台编译选项

```bash
# 专用于Linux服务器的版本
make server-linux

# 专用于Linux客户端的版本  
make client-linux

# 跨平台测试版本
make test-linux      # Linux服务器 + 客户端测试

# 生产环境版本
make production      # Linux服务器 + Windows客户端
```

## 🚀 启动程序

### 启动服务器

#### macOS/Linux环境

```bash
cd bin
./tcp_server        # 或 ./tcp_server_linux
```

#### Windows环境

```batch
cd bin
tcp_server.exe
```

服务器启动后会显示：

```
=== TCP 用户系统服务器 ===
请输入服务器端口 (默认 8080): 
TCP 用户系统服务器启动成功，端口: 8080
等待客户端连接...
```

### 启动客户端

#### macOS/Linux环境

```bash
cd bin
./tcp_client        # 或 ./tcp_client_linux
```

#### Windows环境

```batch
cd bin
tcp_client.exe
```

客户端启动后会显示：

```
=== TCP 用户系统客户端 ===
请输入服务器地址 (默认 127.0.0.1): 
请输入服务器端口 (默认 8080): 
```

## 🎮 功能使用指南

### 登录前界面

```
=== TCP 用户系统 ===
1. 用户登录
2. 用户注册  
0. 退出系统
请选择操作:
```

### 登录后界面

```
=== 用户操作界面 ===
1. 查看用户字符串
2. 修改用户字符串
3. 修改密码
4. 注销账户
5. 登出
0. 退出系统
请选择操作:
```

### 主要功能说明

1. **用户注册** - 创建新的用户账户
2. **用户登录** - 验证身份并进入系统
3. **挤占登录** - 当用户已在其他地方登录时，可选择强制登录
4. **查看/修改用户字符串** - 管理用户自定义数据
5. **修改密码** - 需要验证原密码
6. **注销账户** - 永久删除用户数据
7. **用户登出** - 退出当前登录状态

### 界面体验优化

- **自动清屏** - 每次菜单显示前自动清理控制台
- **操作提示** - 每次操作后提示"按回车键继续"
- **实时反馈** - 及时显示服务器响应信息
- **踢下线通知** - 被其他会话挤占时显示明确提示

## 💡 开发工作流

### Mac开发环境 (推荐)

```bash
# 1. Mac上编译和测试
make dev-test
cd bin
./tcp_server        # 第一个终端
./tcp_client        # 第二个终端

# 2. 编译Linux版本用于生产部署
make server-linux client-linux
```

### 跨平台测试流程

```bash
# 1. 本地开发测试
make dev-test       # Mac环境编译测试

# 2. Linux服务器测试
make test-linux     # 编译Linux服务器+客户端进行网络测试

# 3. 生产环境准备
make production     # Linux服务器 + Windows客户端
```

## 🔒 安全特性

- **会话管理** - 每个连接分配唯一会话ID
- **登录冲突检测** - 防止同一用户多地同时登录
- **密码验证** - 修改密码需要验证原密码
- **操作确认** - 危险操作(如注销账户)需要用户确认
- **连接超时** - 30秒无响应自动断开连接

## 📡 通信协议

系统使用自定义的文本协议，消息格式为：`COMMAND|param1|param2|...`

### 主要命令

| 命令            | 参数                     | 说明           |
| --------------- | ------------------------ | -------------- |
| REGISTER        | userId, password         | 用户注册       |
| LOGIN           | userId, password         | 用户登录       |
| FORCE_LOGIN     | userId, password, choice | 强制登录       |
| LOGOUT          | 无                       | 用户登出       |
| DELETE          | userId, password         | 注销账户       |
| CHANGE_PASSWORD | oldPwd, newPwd           | 修改密码       |
| SET_STRING      | string                   | 设置用户字符串 |
| GET_STRING      | 无                       | 获取用户字符串 |
| QUIT            | 无                       | 客户端退出     |

### 响应格式

| 响应类型 | 格式              | 说明           |
| -------- | ----------------- | -------------- |
| 成功     | SUCCESS\|message  | 操作成功       |
| 错误     | ERROR\|message    | 操作失败       |
| 冲突     | CONFLICT\|message | 登录冲突       |
| 踢下线   | KICKED\|message   | 被其他会话挤占 |

## 💾 数据存储

### 存储位置

程序运行后会在 `bin/` 目录下自动创建：

- `users/` 目录 - 存储用户数据
- `log/` 目录 - 存储服务器日志

**注意**: 必须从 `bin/` 目录启动程序，确保目录在正确位置创建。

### 用户数据存储

用户数据以CSV格式存储在 `bin/users/users.txt` 文件中：

```
userId,password,userString
1,1,hello world
admin,123456,Hello World
user1,password,My Data
```

### 日志文件管理

服务器日志自动记录在 `bin/log/server.log` 文件中：

```
[2025-06-05 13:13:00] [SERVER] 服务器日志系统初始化
[2025-06-05 13:13:00] [SERVER] TCP用户系统服务器初始化，端口: 8080
[2025-06-05 13:13:00] [INFO] 数据文件路径: users/users.txt
[2025-06-05 13:13:00] [INFO] 用户数据加载完成，当前用户数量: 1
[2025-06-05 13:13:00] [SERVER] TCP用户系统服务器启动成功，端口: 8080
[2025-06-05 13:16:40] [INFO] 新客户端连接: 127.0.0.1:65523
```

系统会自动：

- 启动时创建必要的目录结构
- 启动时加载历史数据
- 操作时实时保存数据和记录日志
- 关闭时确保数据完整性

## 🧵 技术实现亮点

### 自定义同步原语

为保证兼容性，项目实现了自定义的线程同步机制：

- `SimpleAtomicBool` - 原子布尔操作
- `SimpleMutex` - 跨平台互斥锁
- `SimpleLockGuard` - RAII锁管理
- `SimpleSharedPtr` - 智能指针实现

### 多线程架构

- 主线程负责接受连接
- 每个客户端连接分配独立处理线程
- 线程安全的用户数据管理
- 优雅的服务器关闭处理

### 跨平台兼容性

- **编译器适配**: 自动检测并使用 clang++ (macOS) / g++ (Linux) / MinGW (Windows)
- **网络库统一**: 自动选择 POSIX Socket (Unix) / WinSock2 (Windows)
- **构建系统**: 智能Make配置支持多平台编译

### 日志管理系统

- 分级日志记录 (INFO/WARN/ERROR/USER/SERVER)
- 同时输出到控制台和文件
- 线程安全的日志写入
- 自动时间戳和格式化

### 网络编程

- 跨平台Socket封装
- 可靠的消息发送接收
- 超时处理和错误恢复
- 非阻塞消息检查

### 用户体验优化

- 跨平台清屏功能
- 智能输入验证
- 友好的错误提示
- 统一的界面风格

## 🐛 故障排除

### 常见问题

1. **编译错误：找不到编译器**

   ```bash
   # macOS: 安装Xcode Command Line Tools
   xcode-select --install

   # Ubuntu/Debian: 安装build-essential
   sudo apt-get install build-essential

   # Windows: 确保MinGW64已正确安装
   ```
2. **连接失败：无法连接到服务器**

   - 检查服务器是否已启动
   - 确认端口号是否正确 (默认8080)
   - 检查防火墙设置
   - 确保服务器和客户端在同一网络
3. **目录创建问题**

   ```bash
   # 确保从bin目录启动程序
   cd bin
   ./tcp_server

   # 检查工作目录权限
   ls -la
   chmod 755 .
   ```
4. **数据文件无法访问**

   - 检查 `bin/` 目录的写权限
   - 确保 `bin/users/users.txt` 文件未被其他程序占用
   - 确保 `bin/log/` 目录具有写权限

### 目录结构问题

如果程序运行后没有在正确位置创建目录：

1. **确保从 `bin/` 目录启动程序**

   ```bash
   cd /path/to/Server-System/bin
   ./tcp_server
   ```
2. **检查当前工作目录**

   ```bash
   pwd  # 应该显示 .../Server-System/bin
   ```
3. **验证程序权限**

   ```bash
   chmod +x tcp_server tcp_client
   ```

### 调试模式

服务器提供详细的操作日志，包括：

```
[2025-06-05 13:13:00] [SERVER] 服务器启动成功，端口: 8080
[2025-06-05 13:13:00] [INFO] 数据文件路径: users/users.txt
[2025-06-05 13:16:40] [INFO] 新客户端连接: 127.0.0.1:65523
[2025-06-05 13:17:15] [USER] 会话[12345678] 用户[admin] 操作[LOGIN] 结果[成功]
```

### Make编译问题

```bash
# 查看Make帮助
make help

# 清理后重新编译
make clean && make dev-test

# 检查编译器可用性
which clang++  # macOS
which g++      # Linux
```

## 📈 扩展建议

1. **数据库支持** - 替换文件存储为SQLite/MySQL
2. **加密传输** - 添加SSL/TLS支持
3. **密码哈希** - 使用bcrypt等安全哈希算法
4. **配置文件** - 支持外部配置文件
5. **GUI界面** - 开发图形化客户端
6. **REST API** - 提供HTTP接口支持
7. **日志轮转** - 实现日志文件分割和清理
8. **统计功能** - 添加用户在线统计和操作分析
9. **Docker支持** - 容器化部署
10. **性能监控** - 添加服务器性能指标

## 📄 许可证

本项目采用MIT许可证，详情请参阅LICENSE文件。

## 👥 贡献指南

欢迎提交Issue和Pull Request来改进项目！

### 开发环境设置

```bash
# 1. 克隆项目
git clone <repository-url>
cd Server-System

# 2. 安装依赖
# macOS: xcode-select --install
# Linux: sudo apt-get install build-essential

# 3. 编译测试
make dev-test

# 4. 运行测试
cd bin && ./tcp_server &
cd bin && ./tcp_client
```

---

**开发环境**: C++11, TCP Socket Programming, Multi-threading
**测试环境**: macOS Sonnet, Linux Ubuntu 20.04+, Windows 10/11
**构建系统**: Make, MinGW, Clang++/GCC
**当前版本**: v1.0 - 基础用户管理系统
