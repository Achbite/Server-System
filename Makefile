# TCP用户系统 - 跨平台Makefile (增强Windows支持)

# 检测操作系统 (增强Windows检测)
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
	MKDIR = if not exist
	RM = del /Q /S
	RMDIR = rmdir /Q /S
	ECHO = echo
	EXE_EXT = .exe
	PATH_SEP = \\
	# Windows链接库
	LDFLAGS = -lws2_32 -lpthread
else
	# Unix/Linux链接库
	LDFLAGS = -lpthread
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

# Windows测试目标
windows-test: all
	@$(ECHO) ""
	@$(ECHO) "Windows环境编译完成!"
	@$(ECHO) ""
	@$(ECHO) "可执行文件:"
	@$(ECHO) "  服务器: $(TARGET_SERVER)"
	@$(ECHO) "  客户端: $(TARGET_CLIENT)"
	@$(ECHO) ""
	@$(ECHO) "启动说明:"
	@$(ECHO) "  1. 打开第一个命令提示符: cd bin && tcp_server.exe"
	@$(ECHO) "  2. 打开第二个命令提示符: cd bin && tcp_client.exe"
	@$(ECHO) ""

# 开发测试 (自动检测平台)
dev-test: all
ifdef IS_WINDOWS
	@$(ECHO) ""
	@$(ECHO) "Windows开发环境就绪!"
	@$(ECHO) "启动: cd bin && tcp_server.exe"
else
	@$(ECHO) ""
	@$(ECHO) "$(HOST_OS)开发环境就绪!"
	@$(ECHO) "启动: cd bin && ./tcp_server"
endif

# 编译所有目标
all: $(BINDIR) $(TARGET_SERVER) $(TARGET_CLIENT) clean-temp
	@$(ECHO) ""
	@$(ECHO) "编译完成! ($(HOST_OS))"
	@$(ECHO) "服务器: $(TARGET_SERVER)"
	@$(ECHO) "客户端: $(TARGET_CLIENT)"
	@$(ECHO) ""

# 创建输出目录
$(BINDIR):
ifdef IS_WINDOWS
	@if not exist $(BINDIR) $(MKDIR) $(BINDIR)
else
	@$(MKDIR) $(BINDIR)
endif

# 编译规则
$(TARGET_SERVER): $(SERVER_OBJECTS)
	@$(ECHO) "链接服务器..."
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(CLIENT_OBJECTS)
	@$(ECHO) "链接客户端..."
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@$(ECHO) "编译: $<"
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
	@$(ECHO) "清理所有生成文件..."
ifdef IS_WINDOWS
	-del /Q *.o 2>nul
	-del /Q Source$(PATH_SEP)Private$(PATH_SEP)*.o 2>nul
	-del /Q $(BINDIR)$(PATH_SEP)*.exe 2>nul
	-del /Q $(BINDIR)$(PATH_SEP)tcp_* 2>nul
	-$(RMDIR) $(BINDIR) 2>nul
else
	@$(RM) *.o $(SRCDIR)/*.o 2>/dev/null || true
	@$(RM) $(BINDIR) 2>/dev/null || true
endif
	@$(ECHO) "清理完成!"

# 帮助信息
help:
	@$(ECHO) ""
ifdef IS_WINDOWS
	@$(ECHO) "TCP用户系统 - Windows构建工具"
	@$(ECHO) ""
	@$(ECHO) "当前环境: $(HOST_OS) (编译器: $(CXX))"
	@$(ECHO) ""
	@$(ECHO) "推荐命令:"
	@$(ECHO) "  make windows-test    Windows环境测试"
	@$(ECHO) "  make dev-test        开发环境测试"
	@$(ECHO) ""
	@$(ECHO) "编译命令:"
	@$(ECHO) "  make all            编译所有目标(.exe)"
	@$(ECHO) "  make server         编译服务器"
	@$(ECHO) "  make client         编译客户端"
	@$(ECHO) "  make clean          清理生成文件"
	@$(ECHO) ""
	@$(ECHO) "运行方式:"
	@$(ECHO) "  cd bin"
	@$(ECHO) "  tcp_server.exe      # 启动服务器"
	@$(ECHO) "  tcp_client.exe      # 启动客户端"
else
	@$(ECHO) "TCP用户系统 - $(HOST_OS)构建工具"
	@$(ECHO) ""
	@$(ECHO) "当前环境: $(HOST_OS)"
	@$(ECHO) ""
	@$(ECHO) "可用命令:"
	@$(ECHO) "  make dev-test       开发环境测试"
	@$(ECHO) "  make all            编译所有目标"
	@$(ECHO) "  make server         编译服务器"
	@$(ECHO) "  make client         编译客户端"
	@$(ECHO) "  make clean          清理生成文件"
endif
	@$(ECHO) ""