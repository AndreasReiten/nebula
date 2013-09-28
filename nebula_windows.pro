# QMAKE PROJECT FILE
unix {
    LIBS += -lm -lGLEW -lGL -lOpenCL -lfreetype
}
else:win32 {
    LIBS += -lm -lglew32 -lOpenCL -lfreetype -Lwin32_lib
    INCLUDEPATH += win32_include
}
HEADERS       = src/mainwindow.h \
                src/utils/worker.h \
                src/utils/miniarray.h \
                src/utils/texthighlighter.h \
                src/utils/contextgl.h \
                src/utils/volumerender.h \
                src/utils/imagerender.h \
                src/utils/tools.h \
                src/utils/matrix.h \
                src/utils/searchnode.h \
                src/utils/bricknode.h \
                src/utils/fileformat.h \
                src/utils/atlas.h \
                src/utils/sparsevoxelocttree.h
SOURCES       = src/main.cpp \
                src/mainwindow.cpp \
                src/utils/worker.cpp \
                src/utils/miniarray.cpp \
                src/utils/texthighlighter.cpp \
                src/utils/contextgl.cpp \
                src/utils/volumerender.cpp \
                src/utils/imagerender.cpp \
                src/utils/tools.cpp \
                src/utils/matrix.cpp \
                src/utils/searchnode.cpp \
                src/utils/bricknode.cpp \
                src/utils/fileformat.cpp \
                src/utils/atlas.cpp \
                src/utils/sparsevoxelocttree.cpp
RESOURCES     = nebula.qrc

unix {
    QMAKE_CXXFLAGS += -std=c++0x -I/usr/include/freetype2
    QT += script opengl
    CONFIG       += debug
    TARGET = nebula
    QMAKE_MAKEFILE = Makefile_linux

    DESTDIR = build/linux/debug
    OBJECTS_DIR = build/linux/debug/.obj
    MOC_DIR = build/linux/debug/.moc
    RCC_DIR = build/linux/debug/.rcc
    UI_DIR = build/linux/debug/.ui
}
else:win32 {
    QMAKE_LFLAGS += -static
    QMAKE_CXXFLAGS += -std=c++0x
    QT += script opengl
    CONFIG       += debug \
                    staticlib
    TARGET = nebula
    QMAKE_MAKEFILE = Makefile_windows

    Release:DESTDIR = build/windows/release
    Release:OBJECTS_DIR = build/windows/release/obj
    Release:MOC_DIR = build/windows/release/moc
    Release:RCC_DIR = build/windows/release/rcc
    Release:UI_DIR = build/windows/release/ui

    Debug:DESTDIR = build/windows/debug
    Debug:OBJECTS_DIR = build/windows/debug/obj
    Debug:MOC_DIR = build/windows/debug/moc
    Debug:RCC_DIR = build/windows/debug/rcc
    Debug:UI_DIR = build/windows/debug/ui
}
