# TCP用户系统 - 跨平台Makefile (增强Windows支持)
# 文件编码: UTF-8

# 检测操作系统 (简化WSL检测)
UNAME_S := $(shell uname -s 2>/dev/null || echo "Windows")

# 操作系统配置
ifeq ($(UNAME_S),Linux)
	HOST_OS = Linux
	CXX = g++
	MKDIR = mkdir -p
	RM = rm -rf
	ECHO = echo
	EXE_EXT = 
	PATH_SEP = /
	LDFLAGS = -lpthread
	
	# 检测WSL环境（用于显示提示信息）
	WSL_CHECK := $(shell grep -i microsoft /proc/version 2>/dev/null)
	ifneq ($(WSL_CHECK),)
		IS_WSL = true
		HOST_OS = Linux(WSL)
	endif
endif

ifeq ($(UNAME_S),Darwin)
	HOST_OS = macOS
	CXX = clang++
	MKDIR = mkdir -p
	RM = rm -rf
	ECHO = echo
	EXE_EXT = 
	PATH_SEP = /
endif

# Windows检测 (支持多种Windows环境)
ifneq (,$(findstring MINGW,$(UNAME_S)))
	HOST_OS = Windows
	IS_WINDOWS = true
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
	HOST_OS = Windows
	IS_WINDOWS = true
endif
ifneq (,$(findstring CYGWIN,$(UNAME_S)))
	HOST_OS = Windows
	IS_WINDOWS = true
endif
ifeq ($(OS),Windows_NT)
	HOST_OS = Windows
	IS_WINDOWS = true
endif

# Windows配置
ifdef IS_WINDOWS
	CXX = g++
	MKDIR = mkdir
	RM = del /Q /S
	RMDIR = rmdir /Q /S
	ECHO = echo
	EXE_EXT = .exe
	PATH_SEP = \\
	LDFLAGS = -lws2_32 -lpthread
	# Windows下设置UTF-8输出
	CMD_PREFIX = chcp 65001 >nul 2>&1 &&
else
	# Unix/Linux/WSL 共用配置
	LDFLAGS = -lpthread
	ECHO = echo
	CMD_PREFIX = 
endif

# 基础配置
SRCDIR = Source$(PATH_SEP)Private
BINDIR = bin
SERVER_SOURCES = main.cpp $(SRCDIR)$(PATH_SEP)TCP_System.cpp
CLIENT_SOURCES = $(SRCDIR)$(PATH_SEP)Client.cpp $(SRCDIR)$(PATH_SEP)TCP_System.cpp

# 编译选项
CXXFLAGS = -std=c++11 -I. -Wall -Wextra

# 目标文件 (自动添加.exe后缀)
TARGET_SERVER = $(BINDIR)$(PATH_SEP)tcp_server$(EXE_EXT)
TARGET_CLIENT = $(BINDIR)$(PATH_SEP)tcp_client$(EXE_EXT)

# 对象文件
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# 默认目标
.PHONY: all clean server client help dev-test clean-temp windows-test

# 创建输出目录
$(BINDIR):
ifdef IS_WINDOWS
	@if not exist $(BINDIR) $(MKDIR) $(BINDIR)
else
	@$(MKDIR) $(BINDIR)
endif

# 编译规则
$(TARGET_SERVER): $(SERVER_OBJECTS)
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) 链接服务器...
else
	@$(ECHO) "链接服务器..."
endif
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(CLIENT_OBJECTS)
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) 链接客户端...
else
	@$(ECHO) "链接客户端..."
endif
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) 编译: $<
else
	@$(ECHO) "编译: $<"
endif
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理中间文件
clean-temp:
	@$(ECHO) "清理中间文件..."
ifdef IS_WINDOWS
	-del /Q *.o 2>nul
	-del /Q Source$(PATH_SEP)Private$(PATH_SEP)*.o 2>nul
else
	@$(RM) *.o $(SRCDIR)/*.o 2>/dev/null || true
endif

# 清理所有文件
clean:
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) 清理所有生成文件...
	-del /Q *.o 2>nul
	-del /Q Source$(PATH_SEP)Private$(PATH_SEP)*.o 2>nul
	-del /Q $(BINDIR)$(PATH_SEP)*.exe 2>nul
	-del /Q $(BINDIR)$(PATH_SEP)tcp_* 2>nul
	-$(RMDIR) $(BINDIR) 2>nul
	@$(ECHO) 清理完成!
else
	@$(ECHO) "清理所有生成文件..."
	@$(RM) *.o $(SRCDIR)/*.o 2>/dev/null || true
	@$(RM) $(BINDIR) 2>/dev/null || true
	@$(ECHO) "清理完成!"
endif

# Windows测试目标
windows-test: all
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) ""
	@$(ECHO) Windows环境编译完成!
	@$(ECHO) ""
	@$(ECHO) 可执行文件:
	@$(ECHO)   服务器: $(TARGET_SERVER)
	@$(ECHO)   客户端: $(TARGET_CLIENT)
	@$(ECHO) ""
	@$(ECHO) 启动说明:
	@$(ECHO)   1. 打开第一个命令提示符: cd bin ^&^& tcp_server.exe
	@$(ECHO)   2. 打开第二个命令提示符: cd bin ^&^& tcp_client.exe
	@$(ECHO) ""
else
	@$(ECHO) ""
	@$(ECHO) "Linux环境编译完成!"
	@$(ECHO) "可执行文件:"
	@$(ECHO) "  服务器: $(TARGET_SERVER)"
	@$(ECHO) "  客户端: $(TARGET_CLIENT)"
	@$(ECHO) ""
endif

# 开发测试 (自动检测平台)
dev-test: all
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) ""
	@$(ECHO) Windows开发环境就绪!
	@$(ECHO) 启动: cd bin ^&^& tcp_server.exe
else
	@$(ECHO) ""
	@$(ECHO) "$(HOST_OS)开发环境就绪!"
	@$(ECHO) "启动: cd bin && ./tcp_server"
endif

# 编译所有目标
all: $(BINDIR) $(TARGET_SERVER) $(TARGET_CLIENT) clean-temp
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) ""
	@$(ECHO) 编译完成! ^($(HOST_OS)^)
	@$(ECHO) 服务器: $(TARGET_SERVER)
	@$(ECHO) 客户端: $(TARGET_CLIENT)
	@$(ECHO) ""
else
	@$(ECHO) ""
	@$(ECHO) "编译完成! ($(HOST_OS))"
	@$(ECHO) "服务器: $(TARGET_SERVER)"
	@$(ECHO) "客户端: $(TARGET_CLIENT)"
	@$(ECHO) ""
endif

# 简化帮助信息
help:
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) ""
	@$(ECHO) TCP用户系统 - Windows构建工具
	@$(ECHO) ""
	@$(ECHO) 当前环境: $(HOST_OS) ^(编译器: $(CXX)^)
	@$(ECHO) ""
	@$(ECHO) 推荐命令:
	@$(ECHO)   make windows-test    Windows环境测试
	@$(ECHO)   make dev-test        开发环境测试
	@$(ECHO) ""
	@$(ECHO) 编译命令:
	@$(ECHO)   make all            编译所有目标^(.exe^)
	@$(ECHO)   make server         编译服务器
	@$(ECHO)   make client         编译客户端
	@$(ECHO)   make clean          清理生成文件
	@$(ECHO) ""
	@$(ECHO) 运行方式:
	@$(ECHO)   cd bin
	@$(ECHO)   tcp_server.exe      # 启动服务器
	@$(ECHO)   tcp_client.exe      # 启动客户端
else
	@$(ECHO) ""
	@$(ECHO) "TCP用户系统 - $(HOST_OS)构建工具"
	@$(ECHO) ""
	@$(ECHO) "当前环境: $(HOST_OS)"
ifdef IS_WSL
	@$(ECHO) "WSL信息: Ubuntu $(shell grep -oP 'VERSION=\"\K[^\"]+' /etc/os-release 2>/dev/null || echo '未知版本')"
	@$(ECHO) "Windows路径: /mnt/d/CPP Project/Server-System"
endif
	@$(ECHO) "编译器: $(shell gcc --version | head -n1 2>/dev/null || echo '$(CXX)')"
	@$(ECHO) ""
	@$(ECHO) "可用命令:"
	@$(ECHO) "  make dev-test       开发环境测试"
	@$(ECHO) "  make all            编译所有目标"
	@$(ECHO) "  make server         编译服务器"
	@$(ECHO) "  make client         编译客户端"
	@$(ECHO) "  make clean          清理生成文件"
	@$(ECHO) "  make check-env      检查编译环境"
ifdef IS_WSL
	@$(ECHO) ""
	@$(ECHO) "WSL特定功能:"
	@$(ECHO) "  make wsl-setup      安装开发工具"
endif
	@$(ECHO) ""
	@$(ECHO) "运行方式:"
	@$(ECHO) "  cd bin && ./tcp_server    # 启动服务器"
	@$(ECHO) "  cd bin && ./tcp_client    # 启动客户端"
ifdef IS_WSL
	@$(ECHO) ""
	@$(ECHO) "Windows中访问:"
	@$(ECHO) "  路径: d:\\CPP Project\\Server-System\\bin\\"
	@$(ECHO) "  注意: 编译的是Linux程序，需在WSL中运行"
endif
endif
	@$(ECHO) ""

# 简化环境检测
check-env:
ifdef IS_WINDOWS
	@chcp 65001 >nul 2>&1
	@$(ECHO) ========== 编译环境信息 ==========
	@$(ECHO) 操作系统: $(UNAME_S)
	@$(ECHO) 环境标识: $(HOST_OS)
	@$(ECHO) 编译器: $(CXX)
	@$(ECHO) 目标平台: Windows可执行文件
	@$(ECHO) 链接库: $(LDFLAGS)
	@$(ECHO) =================================
else
	@$(ECHO) "========== 编译环境信息 =========="
	@$(ECHO) "操作系统: $(UNAME_S)"
	@$(ECHO) "环境标识: $(HOST_OS)"
ifdef IS_WSL
	@$(ECHO) "WSL环境: 是"
	@$(ECHO) "发行版: $(shell grep -oP 'NAME=\"\K[^\"]+' /etc/os-release 2>/dev/null)"
	@$(ECHO) "版本: $(shell grep -oP 'VERSION=\"\K[^\"]+' /etc/os-release 2>/dev/null)"
	@$(ECHO) "内核: $(shell uname -r)"
endif
	@$(ECHO) "编译器: $(CXX)"
	@$(ECHO) "GCC版本: $(shell gcc --version | head -n1 2>/dev/null || echo '未安装')"
	@$(ECHO) "Make版本: $(shell make --version | head -n1 2>/dev/null)"
	@$(ECHO) "目标平台: $(if $(EXE_EXT),Windows可执行文件,Linux可执行文件)"
	@$(ECHO) "链接库: $(LDFLAGS)"
	@$(ECHO) "================================="
endif

# 简化WSL设置（复用Linux包管理）
setup-dev:
ifeq ($(UNAME_S),Linux)
	@$(ECHO) "设置Linux/WSL开发环境..."
	@if command -v apt >/dev/null 2>&1; then \
		sudo apt update && \
		sudo apt install -y build-essential g++ make gdb valgrind; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum groupinstall -y "Development Tools" && \
		sudo yum install -y gcc-c++ make gdb valgrind; \
	else \
		$(ECHO) "请手动安装编译工具: gcc g++ make"; \
	fi
ifdef IS_WSL
	@$(ECHO) "安装中文支持..."
	@sudo apt install -y language-pack-zh-hans 2>/dev/null || true
endif
	@$(ECHO) "✓ 开发环境设置完成!"
else
	@$(ECHO) "Windows环境请安装MinGW或使用WSL"
endif

# 保持兼容性的别名
wsl-setup: setup-dev
wsl-test: dev-test

# 从Windows调用Linux编译
linux-compile:
ifdef IS_WINDOWS
	@$(ECHO) "从Windows调用WSL编译Linux版本..."
	wsl bash -c "cd /mnt/d/CPP\\ Project/Server-System && make dev-test"
else
	@$(ECHO) "当前已在Linux环境，直接编译:"
	$(MAKE) dev-test
endif

# 兼容性别名
wsl-compile: linux-compile