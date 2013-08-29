# QMAKE PROJECT FILE
unix {
    LIBS += -lm -lGLEW -lGL -lOpenCL -lhdf5 -lfreetype
}
else:win32 {
    LIBS += -lm -lhdf5 -lglew32_static -lOpenCL -Llib_mingw32 -lfreetype
    INCLUDEPATH += inc_mingw32
}
HEADERS       = src/mainwindow.h \
                src/utils/miniarray.h \
                src/utils/text_highlighter.h \
                src/utils/vol_data_set.h \
                src/utils/glclinit.h \
                src/utils/volumerender.h \
                src/utils/imagerender.h \
                src/utils/tools.h \
                src/utils/matrix.h \
                src/utils/octnode.h \
                src/utils/node.h \
                src/utils/file_formats.h \
                src/utils/atlas.h
SOURCES       = src/main.cpp \
                src/mainwindow.cpp \
                src/utils/miniarray.cpp \
                src/utils/text_highlighter.cpp \
                src/utils/vol_data_set.cpp \
                src/utils/glclinit.cpp \
                src/utils/volumerender.cpp \
                src/utils/imagerender.cpp \
                src/utils/tools.cpp \
                src/utils/matrix.cpp \
                src/utils/octnode.cpp \
                src/utils/node.cpp \
                src/utils/file_formats.cpp \
                src/utils/atlas.cpp 
RESOURCES     = riv.qrc 

unix {
    QMAKE_CXXFLAGS += -std=c++0x -I/usr/include/freetype2
    QT += script opengl
    CONFIG       += debug
    TARGET = riv
    QMAKE_MAKEFILE = Makefile_unix
    
    DESTDIR = build/unix
    OBJECTS_DIR = build/unix/.obj
    MOC_DIR = build/unix/.moc
    RCC_DIR = build/unix/.rcc
    UI_DIR = build/unix/.ui
}
else:win32 {
    QMAKE_CXXFLAGS += -std=c++0x -I/usr/include/freetype2
    QT += script opengl
    #CONFIG       += release
    TARGET = riv
    QMAKE_MAKEFILE = Makefile_win32
    
    Release:DESTDIR = build/win32/release
    Release:OBJECTS_DIR = build/win32/release/obj
    Release:MOC_DIR = build/win32/release/moc
    Release:RCC_DIR = build/win32/release/rcc
    Release:UI_DIR = build/win32/release/ui

    Debug:DESTDIR = build/win32/debug
    Debug:OBJECTS_DIR = build/win32/debug/obj
    Debug:MOC_DIR = build/win32/debug/moc
    Debug:RCC_DIR = build/win32/debug/rcc
    Debug:UI_DIR = build/win32/debug/ui
}
