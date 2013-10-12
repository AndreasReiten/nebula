# QMAKE PROJECT FILE
LIBS += -lm -lGLEW -lGL -lOpenCL -lfreetype

HEADERS       = src/mainwindow.h \
                src/utils/contextgl.h \
                src/utils/volumerender.h \
                src/utils/imagerender.h \
                src/utils/fileformat.h \
                src/utils/worker.h \
                src/utils/miniarray.h \
                src/utils/texthighlighter.h \
                src/utils/tools.h \
                src/utils/matrix.h \
                src/utils/searchnode.h \
                src/utils/bricknode.h \
                src/utils/atlas.h \
                src/utils/sparsevoxelocttree.h \
                src/utils/contextcl.h \
                src/utils/devicecl.h

SOURCES       = src/main.cpp \
                src/mainwindow.cpp \
                src/utils/contextgl.cpp \
                src/utils/volumerender.cpp \
                src/utils/imagerender.cpp \
                src/utils/fileformat.cpp \
                src/utils/worker.cpp \
                src/utils/miniarray.cpp \
                src/utils/texthighlighter.cpp \
                src/utils/tools.cpp \
                src/utils/matrix.cpp \
                src/utils/searchnode.cpp \
                src/utils/bricknode.cpp \
                src/utils/atlas.cpp \
                src/utils/sparsevoxelocttree.cpp \
                src/utils/contextcl.cpp \
                src/utils/devicecl.cpp

RESOURCES     = nebula.qrc

QMAKE_CXXFLAGS += -std=c++0x -I/usr/include/freetype2
QT += core gui opengl widgets script # Use for Qt5
#QT += opengl script # Use for Qt4
TARGET = nebula
QMAKE_MAKEFILE = Makefile

OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui
