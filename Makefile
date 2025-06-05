# TCP用户系统 - 跨平台Makefile
# 使用方法:
#   make help           - 显示帮助信息
#   make server-linux   - 编译Linux服务器端
#   make client-linux   - 编译Linux客户端(用于测试)
#   make dev-test       - Mac开发环境测试
#   make clean          - 清理生成文件

# 检测操作系统
UNAME_S := $(shell uname -s 2>/dev/null || echo "Windows")
ifeq ($(UNAME_S),Linux)
	HOST_OS = Linux
	MKDIR = mkdir -p
	RM = rm -rf
	ECHO = echo
endif
ifeq ($(UNAME_S),Darwin)
	HOST_OS = macOS
	MKDIR = mkdir -p
	RM = rm -rf
	ECHO = echo
endif
ifneq (,$(findstring MINGW,$(UNAME_S)))
	HOST_OS = Windows
	MKDIR = if not exist
	RM = del /Q
	ECHO = echo
endif

# 基础配置
SRCDIR = Source/Private
BINDIR = bin
SERVER_SOURCES = main.cpp $(SRCDIR)/TCP_System.cpp
CLIENT_SOURCES = $(SRCDIR)/Client.cpp $(SRCDIR)/TCP_System.cpp

# Linux/Mac配置（兼容）
CXX = clang++
CXXFLAGS = -std=c++11 -I. -Wall -Wextra
LDFLAGS = -lpthread
TARGET_SERVER = $(BINDIR)/tcp_server
TARGET_CLIENT = $(BINDIR)/tcp_client

# 对象文件
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# 默认目标
.PHONY: all clean server client help dev-test clean-temp

# Mac开发测试（推荐）
dev-test: all
	@$(ECHO) ""
	@$(ECHO) "Mac开发环境测试版本就绪!"
	@$(ECHO) ""
	@$(ECHO) "测试说明:"
	@$(ECHO) "1. 第一个终端运行: $(TARGET_SERVER)"
	@$(ECHO) "2. 第二个终端运行: $(TARGET_CLIENT)"
	@$(ECHO) "3. 测试网络连接和数据传输"
	@$(ECHO) ""

# 编译所有目标（编译完成后自动清理中间文件）
all: $(BINDIR) $(TARGET_SERVER) $(TARGET_CLIENT) clean-temp
	@$(ECHO) ""
	@$(ECHO) "编译完成!"
	@$(ECHO) "服务器: $(TARGET_SERVER)"
	@$(ECHO) "客户端: $(TARGET_CLIENT)"
	@$(ECHO) ""

# 创建输出目录
$(BINDIR):
ifeq ($(HOST_OS),Windows)
	@if not exist $(BINDIR) mkdir $(BINDIR)
else
	@$(MKDIR) $(BINDIR)
endif

# 编译服务器（编译完成后自动清理中间文件）
server: $(BINDIR) $(TARGET_SERVER) clean-temp
	@$(ECHO) "服务器编译完成: $(TARGET_SERVER)"

$(TARGET_SERVER): $(SERVER_OBJECTS)
	@$(ECHO) "链接服务器..."
	$(CXX) -o $@ $^ $(LDFLAGS)

# 编译客户端（编译完成后自动清理中间文件）
client: $(BINDIR) $(TARGET_CLIENT) clean-temp
	@$(ECHO) "客户端编译完成: $(TARGET_CLIENT)"

$(TARGET_CLIENT): $(CLIENT_OBJECTS)
	@$(ECHO) "链接客户端..."
	$(CXX) -o $@ $^ $(LDFLAGS)

# 编译规则
%.o: %.cpp
	@$(ECHO) "编译: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理中间文件（只删除.o文件，保留可执行文件）
clean-temp:
	@$(ECHO) "清理中间文件..."
ifeq ($(HOST_OS),Windows)
	-del /Q *.o 2>nul
	-del /Q Source\Private\*.o 2>nul
else
	@$(RM) *.o $(SRCDIR)/*.o 2>/dev/null || true
endif

# 清理所有生成文件（包括可执行文件）
clean:
	@$(ECHO) "清理所有生成文件..."
ifeq ($(HOST_OS),Windows)
	-del /Q *.o 2>nul
	-del /Q Source\Private\*.o 2>nul
	-del /Q $(BINDIR)\*.exe 2>nul
	-del /Q $(BINDIR)\tcp_* 2>nul
	-rmdir /Q /S $(BINDIR) 2>nul
else
	@$(RM) *.o $(SRCDIR)/*.o 2>/dev/null || true
	@$(RM) $(BINDIR) 2>/dev/null || true
endif
	@$(ECHO) "清理完成!"

# 帮助信息
help:
	@$(ECHO) ""
	@$(ECHO) "TCP用户系统 - Mac开发构建工具"
	@$(ECHO) ""
	@$(ECHO) "当前环境: $(HOST_OS)"
	@$(ECHO) ""
	@$(ECHO) "可用命令:"
	@$(ECHO) "  make dev-test     Mac开发环境测试(推荐)"
	@$(ECHO) "  make all          编译所有目标"
	@$(ECHO) "  make server       编译服务器"
	@$(ECHO) "  make client       编译客户端"
	@$(ECHO) "  make clean        清理所有生成文件"
	@$(ECHO) "  make help         显示此帮助"
	@$(ECHO) ""
	@$(ECHO) "推荐使用:"
	@$(ECHO) "  make dev-test     # 编译并显示测试说明"
	@$(ECHO) ""
	@$(ECHO) "注意: 编译完成后会自动清理中间文件(.o文件)"
	@$(ECHO) ""