# QMAKE PROJECT FILE
# Current issues in windows (VS10 64 bit): Cluttered text with QOpenGLPaintDevice. Can not set GL version explicitly for QSurfaceFormat.
LIBS += -lOpenCL -Llib_win64VS10
INCLUDEPATH += inc

HEADERS       = src/mainwindow.h \
                src/utils/openglwindow.h \
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
                src/utils/sparsevoxelocttree.h \
                src/utils/contextcl.h \
                src/utils/devicecl.h \
                src/utils/sharedcontext.h \
                src/utils/transferfunction.h

SOURCES       = src/main.cpp \
                src/mainwindow.cpp \
                src/utils/openglwindow.cpp \
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
                src/utils/sparsevoxelocttree.cpp \
                src/utils/contextcl.cpp \
                src/utils/devicecl.cpp \
                src/utils/sharedcontext.cpp \
                src/utils/transferfunction.cpp
RESOURCES     = nebula.qrc

QT += core gui opengl widgets script
CONFIG       += c++11
#CONFIG       += static
TARGET = nebula
QMAKE_MAKEFILE = Makefile

OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui

