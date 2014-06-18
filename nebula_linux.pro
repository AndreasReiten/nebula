# QMAKE PROJECT FILE
LIBS += -lOpenCL

RESOURCES     = nebula.qrc

QMAKE_CXXFLAGS += -std=c++0x # C++11 
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
    src/kernels/image_preview.cl \
    src/kernels/parallel_reduction.cl \
    src/kernels/box_sampler.cl \
    src/kernels/models.cl

HEADERS += \
    lib/qxlib/qxfile/utils/fileformat.h \
    lib/qxlib/qxfile/utils/filetreeview.h \
    lib/qxlib/qxfile/qxfilelib.h \
    lib/qxlib/qximage/utils/imagepreview.h \
    lib/qxlib/qximage/qximagelib.h \
    lib/qxlib/qxmath/utils/ccmatrix.h \
    lib/qxlib/qxmath/utils/colormatrix.h \
    lib/qxlib/qxmath/utils/matrix.h \
    lib/qxlib/qxmath/utils/rotationmatrix.h \
    lib/qxlib/qxmath/utils/ubmatrix.h \
    lib/qxlib/qxmath/qxmathlib.h \
    lib/qxlib/qxopencl/utils/contextcl.h \
    lib/qxlib/qxopencl/utils/devicecl.h \
    lib/qxlib/qxopencl/qxopencllib.h \
    lib/qxlib/qxopengl/utils/openglwindow.h \
    lib/qxlib/qxopengl/utils/sharedcontext.h \
    lib/qxlib/qxopengl/utils/transferfunction.h \
    lib/qxlib/qxopengl/qxopengllib.h \
    lib/qxlib/qxsvo/qxsvolib.h \
    lib/qxlib/qxlib.h \
    lib/qxlib/qxsvo/utils/bricknode.h \
    lib/qxlib/qxsvo/utils/searchnode.h \
    lib/qxlib/qxsvo/utils/sparsevoxelocttree.h \
    src/utils/marker.h \
    src/utils/texthighlighter.h \
    src/utils/volumerender.h \
    src/utils/worker.h \
    src/mainwindow.h

SOURCES += \
    lib/qxlib/qxfile/utils/fileformat.cpp \
    lib/qxlib/qxfile/utils/filetreeview.cpp \
    lib/qxlib/qxfile/qxfilelib.cpp \
    lib/qxlib/qximage/utils/imagepreview.cpp \
    lib/qxlib/qximage/qximagelib.cpp \
    lib/qxlib/qxmath/qxmathlib.cpp \
    lib/qxlib/qxopencl/utils/contextcl.cpp \
    lib/qxlib/qxopencl/utils/devicecl.cpp \
    lib/qxlib/qxopencl/qxopencllib.cpp \
    lib/qxlib/qxopengl/utils/openglwindow.cpp \
    lib/qxlib/qxopengl/utils/sharedcontext.cpp \
    lib/qxlib/qxopengl/utils/transferfunction.cpp \
    lib/qxlib/qxopengl/qxopengllib.cpp \
    lib/qxlib/qxsvo/qxsvolib.cpp \
    lib/qxlib/qxsvo/utils/bricknode.cpp \
    lib/qxlib/qxsvo/utils/searchnode.cpp \
    lib/qxlib/qxsvo/utils/sparsevoxelocttree.cpp \
    src/utils/marker.cpp \
    src/utils/texthighlighter.cpp \
    src/utils/volumerender.cpp \
    src/utils/worker.cpp \
    src/main.cpp \
    src/mainwindow.cpp
