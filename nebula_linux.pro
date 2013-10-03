# QMAKE PROJECT FILE
LIBS += -lm -lGLEW -lGL -lOpenCL -lfreetype

HEADERS       = src/mainwindow.h \
                src/utils/contextgl.h \
#                src/utils/miniarray.h \
#                src/utils/texthighlighter.h \
#                src/utils/volumerender.h \
#                src/utils/imagerender.h \
                src/utils/tools.h \
                src/utils/worker.h \
#                src/utils/matrix.h \
#                src/utils/searchnode.h \
#                src/utils/bricknode.h \
                src/utils/fileformat.h
#                src/utils/atlas.h \
#                src/utils/sparsevoxelocttree.h
SOURCES       = src/main.cpp \
                src/mainwindow.cpp \
                src/utils/contextgl.cpp \
#                src/utils/miniarray.cpp \
#                src/utils/texthighlighter.cpp \
#                src/utils/volumerender.cpp \
#                src/utils/imagerender.cpp \
                src/utils/tools.cpp \
                src/utils/worker.cpp \
#                src/utils/matrix.cpp \
#                src/utils/searchnode.cpp \
#                src/utils/bricknode.cpp \
                src/utils/fileformat.cpp
#                src/utils/atlas.cpp \
#                src/utils/sparsevoxelocttree.cpp
RESOURCES     = nebula.qrc

QMAKE_CXXFLAGS += -std=c++0x -I/usr/include/freetype2
QT += core opengl widgets
#script
#QT += opengl script
TARGET = nebula
QMAKE_MAKEFILE = Makefile

OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui
