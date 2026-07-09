TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

HEADERS += cmdline.h

win32{
contains(QT_ARCH, x86_64){
    LIBS += -L$$PWD/../../lib/win64 -lViewLink
    DESTDIR = $$PWD/bin/win64

    DllSrc = $$PWD/../../bin/win64/*.dll
    DllSrc = $$replace(DllSrc, /, \\)
    DllDst = $$replace(DESTDIR, /, \\)

    QMAKE_PRE_LINK += $$QMAKE_COPY $$DllSrc $$DllDst
}else{
    LIBS += -L$$PWD/../../lib/win32 -lViewLink
    DESTDIR =$$PWD/bin/win32

    DllSrc = $$PWD/../../bin/win32/*.dll
    DllSrc = $$replace(DllSrc, /, \\)
    DllDst = $$replace(DESTDIR, /, \\)

    QMAKE_PRE_LINK += $$QMAKE_COPY $$DllSrc $$DllDst
}
}
linux{
contains(QT_ARCH, x86_64){
    DESTDIR =$$PWD/bin/linux-x86_64
    LIBS += -L$$DESTDIR -lViewLink
    QMAKE_PRE_LINK += $$QMAKE_COPY $$PWD/../../bin/linux-x86_64/*.so $$DESTDIR
}
contains(QT_ARCH, aarch64){
    DESTDIR =$$PWD/bin/linux-aarch64
    LIBS += -L$$DESTDIR -lViewLink
    QMAKE_PRE_LINK += $$QMAKE_COPY $$PWD/../../bin/linux-aarch64/*.so $$DESTDIR
}
}


