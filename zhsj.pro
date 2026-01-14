# zhsj.pro
QT       += core gui widgets
CONFIG += c++17
TARGET = TaskManager
TEMPLATE = app

# 源文件
SOURCES += main.cpp \
           mainwindow.cpp \
           taskmodel.cpp \
           reminderthread.cpp

# 头文件
HEADERS  += mainwindow.h \
            taskmodel.h \
            reminderthread.h

# UI文件
FORMS    += mainwindow.ui

# 中文编码支持
DEFINES += QT_DEPRECATED_WARNINGS

# 添加控制台输出（Windows）
CONFIG += console

# 设置输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/temp/.obj
MOC_DIR = $$PWD/temp/.moc
RCC_DIR = $$PWD/temp/.rcc
UI_DIR = $$PWD/temp/.ui
