#-------------------------------------------------
#
# Project created by QtCreator 2020-06-25T15:33:07
#
#-------------------------------------------------

QT       += core gui opengl multimedia multimediawidgets serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = demo_for_qt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# Keep warnings/critical messages and qInfo diagnostics, but suppress verbose
# qDebug output in Release builds. This does not affect ViewLink SDK VLKLog.
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32 {
INCLUDEPATH += $$PWD/../demo_for_windows/ffmpeg/include
}
INCLUDEPATH += $$PWD/../../inc

win32{
contains(QT_ARCH, x86_64){
    LIBS += -L$$PWD/../../lib/win64 -lViewLink \
            -L$$PWD/../demo_for_windows/ffmpeg/lib/win64 -lavformat -lavutil -lavcodec -lswresample -lswscale
    DESTDIR = $$PWD/bin/win64

    SDKDll = $$PWD/../../bin/win64/*.dll
    SDKDll = $$replace(SDKDll, /, \\)
    FfmpegDll = $$PWD/../demo_for_windows/ffmpeg/bin/win64/*.dll
    FfmpegDll = $$replace(FfmpegDll, /, \\)
    DllDst = $$replace(DESTDIR, /, \\)

    !contains(CONFIG, ci) {
        QMAKE_PRE_LINK += $$QMAKE_COPY $$SDKDll $$DllDst && \
                          $$QMAKE_COPY $$FfmpegDll $$DllDst
    }
}else{
    LIBS += -L$$PWD/../../lib/win32 -lViewLink \
            -L$$PWD/../demo_for_windows/ffmpeg/lib/win32 -lavformat -lavutil -lavcodec -lswresample -lswscale
    DESTDIR =$$PWD/bin/win32

    SDKDll = $$PWD/../../bin/win32/*.dll
    SDKDll = $$replace(SDKDll, /, \\)
    FfmpegDll = $$PWD/../demo_for_windows/ffmpeg/bin/win32/*.dll
    FfmpegDll = $$replace(FfmpegDll, /, \\)
    DllDst = $$replace(DESTDIR, /, \\)

    !contains(CONFIG, ci) {
        QMAKE_PRE_LINK += $$QMAKE_COPY $$SDKDll $$DllDst && \
                          $$QMAKE_COPY $$FfmpegDll $$DllDst
    }
}
}
linux{
contains(QT_ARCH, x86_64){
    DESTDIR =$$PWD/bin/linux-x86_64
    LIBS += -L$$DESTDIR -lViewLink -lavformat -lavcodec -lavutil -lswscale -lswresample
    QMAKE_PRE_LINK += $$QMAKE_COPY $$PWD/../../bin/linux-x86_64/*.so $$DESTDIR
}
contains(QT_ARCH, aarch64){
    DESTDIR = $$PWD/bin/linux-aarch64
    LIBS += -L$$DESTDIR -lViewLink -lavformat -lavcodec -lavutil -lswscale -lswresample
    QMAKE_PRE_LINK += $$QMAKE_COPY $$PWD/../../bin/linux-aarch64/*.so $$DESTDIR
}
}

SOURCES += \
        main.cpp \
        widget.cpp \
        VideoObjNetwork.cpp \
        VLKVideoWidget.cpp \
        VideoObjUSBCamera.cpp \
        QtCameraCapture.cpp \
        FFVideoFormatConvert.cpp

HEADERS += \
        widget.h \
        VideoObjNetwork.h \
        ShaderSourceCode.h \
        VLKVideoWidget.h \
        VideoObjUSBCamera.h \
        QtCameraCapture.h \
        FFVideoFormatConvert.h

FORMS += \
        widget.ui

RESOURCES += \
    res.qrc
