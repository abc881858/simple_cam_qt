QT += core gui widgets

CONFIG += c++17
CONFIG += no_keywords

SOURCES += \
    image.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    image.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += /usr/local/include/libcamera
INCLUDEPATH += /usr/local/include/libcamera/libcamera

LIBS += /usr/local/lib/aarch64-linux-gnu/libcamera.so.0.1.0
LIBS += /usr/local/lib/aarch64-linux-gnu/libcamera-base.so.0.1.0
