QT       += core gui widgets sql
CONFIG += c++17
TARGET = TaskManager
TEMPLATE = app

# 源文件
SOURCES += main.cpp \
           mainwindow.cpp \
           taskmodel.cpp \
           reminderthread.cpp \
           dbmanager.cpp

# 头文件
HEADERS  += mainwindow.h \
            taskmodel.h \
            reminderthread.h \
            dbmanager.h

# UI文件
FORMS    += mainwindow.ui

# 中文编码支持
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_CXXFLAGS += -finput-charset=UTF-8

# 如果是 Qt 6，添加以下行
QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_MAJOR_VERSION = $$member(QT_VERSION, 0)
greaterThan(QT_MAJOR_VERSION, 5) {
    DEFINES += QT6_COMPAT
}
