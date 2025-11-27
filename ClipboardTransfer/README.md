
# 剪贴板文件传输演示

这个项目演示了如何使用Qt和Windows剪贴板API实现模拟网络文件传输的功能。它使用VirtualFileSrcStream类创建一个虚拟文件流，并通过FileBufferManager类管理文件数据缓冲。

## 功能特点

- 模拟网络文件传输到剪贴板
- 使用定时器向中间缓存队列添加文件块
- FileStream::Read方法从队列中消费数据
- 提供开始传输和取消传输按钮
- 显示传输进度

## 项目结构

- `mainwindow.h/cpp`: Qt主窗口，提供UI界面
- `FileBufferManager.h/cpp`: 文件缓冲区管理器，负责生成和管理文件数据
- `VirtualFileSrcStream.h/cpp`: 虚拟文件流实现，用于Windows剪贴板
- `DataObject.h/cpp`: 数据对象基类
- `main.cpp`: 应用程序入口

## 使用方法

1. 打开应用程序
2. 输入要模拟传输的文件名
3. 设置文件大小（MB）
4. 点击"开始传输"按钮
5. 文件将被添加到剪贴板
6. 在目标位置粘贴文件（如桌面、文件夹等）

## 技术实现

1. **FileBufferManager**:
   - 使用QTimer定时向队列添加数据块
   - 提供线程安全的数据读取接口
   - 模拟网络传输的延迟和分块特性

2. **VirtualFileSrcStream**:
   - 实现IStream接口
   - 从FileBufferManager读取数据
   - 支持异步操作

3. **MainWindow**:
   - 提供用户界面
   - 控制传输的开始和取消
   - 显示传输进度

## 编译说明

1. 确保安装了Qt 5.x或更高版本
2. 使用Qt Creator打开ClipboardTransfer.pro项目
3. 配置编译器（建议使用MSVC）
4. 构建并运行项目

## 注意事项

- 此项目仅用于演示目的
- 实际网络传输需要实现相应的网络通信代码
- 在Windows系统上运行
