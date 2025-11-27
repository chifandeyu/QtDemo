
QT += widgets
CONFIG += c++17

TARGET = ClipboardTransfer
TEMPLATE = app

# 指定源文件编码为UTF-8
CODECFORTR = UTF-8
CODECFORSRC = UTF-8
CONFIG(debug, debug|release) {
    # Debug 模式：EXE 输出到 ../bin/debug
    DESTDIR = $$PWD/debug
} else {
    # Release 模式：EXE 输出到 ../bin/release
    DESTDIR = $$PWD/release
}

SOURCES +=     main.cpp \
     dataproducerthread.cpp \
     mainwindow.cpp \
     VirtualFileSrcStream.cpp \
     DataObject.cpp \
     filebuffermanager.cpp

HEADERS += \
     dataproducerthread.h \
     mainwindow.h \
     VirtualFileSrcStream.h \
     DataObject.h \
     filebuffermanager.h

# Windows specific
win32 {
    LIBS += -lshell32 -lole32 -luuid -luser32
}
