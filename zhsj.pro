QT += core gui widgets network

CONFIG += c++11

# 可执行文件名称
TARGET = zhsj
TEMPLATE = app

# 源文件列表
SOURCES += main.cpp\
           mainwindow.cpp

# 头文件列表
HEADERS  += mainwindow.h

# UI文件列表
FORMS    += mainwindow.ui

# 部署规则（可选）
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
