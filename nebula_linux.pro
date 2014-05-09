# QMAKE PROJECT FILE
LIBS += -lOpenCL

HEADERS = \
    src/mainwindow.h \
    src/utils/openglwindow.h \
    src/utils/volumerender.h \
    src/utils/fileformat.h \
    src/utils/worker.h \
    src/utils/texthighlighter.h \
    src/utils/tools.h \
    src/utils/matrix.h \
    src/utils/searchnode.h \
    src/utils/bricknode.h \
    src/utils/sparsevoxelocttree.h \
    src/utils/contextcl.h \
    src/utils/devicecl.h \
    src/utils/sharedcontext.h \
    src/utils/transferfunction.h \
    src/utils/filetreeview.h \
    src/utils/imagepreview.h \
    src/utils/marker.h

SOURCES = \
    src/main.cpp \
    src/mainwindow.cpp \
    src/utils/openglwindow.cpp \
    src/utils/volumerender.cpp \
    src/utils/fileformat.cpp \
    src/utils/worker.cpp \
    src/utils/texthighlighter.cpp \
    src/utils/tools.cpp \
    src/utils/matrix.cpp \
    src/utils/searchnode.cpp \
    src/utils/bricknode.cpp \
    src/utils/sparsevoxelocttree.cpp \
    src/utils/contextcl.cpp \
    src/utils/devicecl.cpp \
    src/utils/sharedcontext.cpp \
    src/utils/transferfunction.cpp \
    src/utils/filetreeview.cpp \
    src/utils/imagepreview.cpp \
    src/utils/marker.cpp

RESOURCES     = nebula.qrc

QMAKE_CXXFLAGS += -std=c++0x # C++11 features are needed for Matrix
QT += core gui opengl widgets script
TARGET = nebula
QMAKE_MAKEFILE = Makefile

OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui

OTHER_FILES += \
    src/kernels/project.cl \
    src/kernels/voxelize.cl \
    src/kernels/render_svo.cl \
    src/kernels/render_shared.cl \
    src/kernels/render_model.cl \
    src/kernels/integrate.cl \
    src/shaders/std_2d_tex.f.glsl \
    src/shaders/std_2d_tex.v.glsl \
    src/shaders/std_blend.f.glsl \
    src/shaders/std_blend.v.glsl \
    src/shaders/std_3d_col.f.glsl \
    src/shaders/std_3d_col.v.glsl \
    src/shaders/std_2d_col.v.glsl \
    src/shaders/std_2d_col.f.glsl \ 
    src/shaders/unitcell.f.glsl \
    src/shaders/unitcell.v.glsl \
    src/kernels/image_preview.cl
