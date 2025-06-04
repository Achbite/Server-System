# TCP用户系统 - Makefile
# 使用方法:
#   make           - 编译所有目标
#   make server    - 编译服务器端
#   make client    - 编译客户端
#   make clean     - 清理生成文件

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -I. -Wall -Wextra
LDFLAGS = -lws2_32

# 输出目录
BINDIR = bin

# 目标文件
TARGET_SERVER = $(BINDIR)/tcp_server.exe
TARGET_CLIENT = $(BINDIR)/tcp_client.exe

# 源文件
SRCDIR = Source/Private
SERVER_SOURCES = main.cpp $(SRCDIR)/TCP_System.cpp
CLIENT_SOURCES = $(SRCDIR)/Client.cpp $(SRCDIR)/TCP_System.cpp

# 对象文件
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# 默认目标
.PHONY: all clean server client help

all: $(BINDIR) $(TARGET_SERVER) $(TARGET_CLIENT)
	@chcp 65001 >nul 2>&1
	@echo.
	@echo 编译完成!
	@echo 运行服务器: $(TARGET_SERVER)
	@echo 运行客户端: $(TARGET_CLIENT)
	@echo 清理中间文件...
	-del /Q *.o 2>nul
	-del /Q Source\Private\*.o 2>nul

# 创建输出目录
$(BINDIR):
	@if not exist $(BINDIR) mkdir $(BINDIR)

# 编译服务器端
server: $(BINDIR) $(TARGET_SERVER)
	@chcp 65001 >nul 2>&1
	@echo 清理中间文件...
	-del /Q *.o 2>nul
	-del /Q Source\Private\*.o 2>nul

$(TARGET_SERVER): $(SERVER_OBJECTS)
	@chcp 65001 >nul 2>&1
	@echo 链接服务器...
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo 服务器编译成功!

# 编译客户端
client: $(BINDIR) $(TARGET_CLIENT)
	@chcp 65001 >nul 2>&1
	@echo 清理中间文件...
	-del /Q *.o 2>nul
	-del /Q Source\Private\*.o 2>nul

$(TARGET_CLIENT): $(CLIENT_OBJECTS)
	@chcp 65001 >nul 2>&1
	@echo 链接客户端...
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo 客户端编译成功!

# 编译规则
%.o: %.cpp
	@chcp 65001 >nul 2>&1
	@echo 编译: $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成文件
clean:
	@chcp 65001 >nul 2>&1
	@echo 清理生成文件...
	-del /Q *.o 2>nul
	-del /Q Source\Private\*.o 2>nul
	-del /Q $(BINDIR)\*.exe 2>nul
	-rmdir /Q /S $(BINDIR) 2>nul
	@echo 清理完成!

# 帮助信息
help:
	@chcp 65001 >nul 2>&1
	@echo TCP用户系统 - 构建帮助
	@echo.
	@echo 可用命令:
	@echo   mingw32-make           - 编译所有目标
	@echo   mingw32-make server    - 编译服务器端
	@echo   mingw32-make client    - 编译客户端
	@echo   mingw32-make clean     - 清理生成文件
	@echo   mingw32-make help      - 显示此帮助信息