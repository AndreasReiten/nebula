# QMAKE PROJECT FILE
win32 {
    INCLUDEPATH = src/utils/opencl/khronos/
}

unix {
    INCLUDEPATH = src/utils/opencl/khronos/
    QMAKE_CXXFLAGS += -std=c++0x # C++11
}

QT += widgets

TARGET = nebula
QMAKE_MAKEFILE = Makefile
RESOURCES  = nebula.qrc

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
    src/kernels/box_sampler.cl \
    src/kernels/models.cl \
    src/kernels/image_preview.cl \
    src/kernels/mem_functions.cl \
    src/kernels/parallel_reduce.cl \
    src/kernels/background_filter.cl \
    src/kernels/line_integral.cl \
    src/shaders/std_2d_col.f.glsl \
    src/shaders/std_2d_col.v.glsl \
    src/shaders/std_2d_tex.f.glsl \
    src/shaders/std_2d_tex.v.glsl \
    src/shaders/std_3d_col.f.glsl \
    src/shaders/std_3d_col.v.glsl \
    src/shaders/std_blend.f.glsl \
    src/shaders/std_blend.v.glsl \
    src/shaders/unitcell.f.glsl \
    src/shaders/unitcell.v.glsl \
    src/shaders/rect_hl_2d_tex.f.glsl \
    src/shaders/rect_hl_2d_tex.v.glsl

HEADERS += \
    src/utils/file/utils/fileformat.h \
    src/utils/file/utils/filetreeview.h \
    src/utils/file/qxfilelib.h \
    src/utils/image/imagepreview.h \
    src/utils/math/utils/ccmatrix.h \
    src/utils/math/utils/colormatrix.h \
    src/utils/math/utils/matrix.h \
    src/utils/math/utils/rotationmatrix.h \
    src/utils/math/utils/ubmatrix.h \
    src/utils/math/qxmathlib.h \
    src/utils/opencl/utils/contextcl.h \
    src/utils/opencl/qxopencllib.h \
    src/utils/misc/transferfunction.h \
    src/utils/svo/qxsvolib.h \
    src/utils/lib.h \
    src/utils/svo/utils/bricknode.h \
    src/utils/svo/utils/searchnode.h \
    src/utils/svo/utils/sparsevoxeloctree.h \
    src/utils/misc/texthighlighter.h \
    src/utils/volume/volumerender.h \
    src/utils/worker/worker.h \
    src/mainwindow.h \
    src/utils/file/utils/framecontainer.h \
    src/utils/file/utils/selection.h \
    src/utils/misc/line.h \
    src/utils/misc/marker.h \
    src/utils/misc/linemodel.h
#    src/utils/glsl/glslfunctions.h

SOURCES += \
    src/utils/file/utils/fileformat.cpp \
    src/utils/file/utils/filetreeview.cpp \
    src/utils/file/qxfilelib.cpp \
    src/utils/image/imagepreview.cpp \
    src/utils/math/qxmathlib.cpp \
    src/utils/opencl/utils/contextcl.cpp \
    src/utils/opencl/qxopencllib.cpp \
    src/utils/misc/transferfunction.cpp \
    src/utils/svo/qxsvolib.cpp \
    src/utils/svo/utils/bricknode.cpp \
    src/utils/svo/utils/searchnode.cpp \
    src/utils/svo/utils/sparsevoxeloctree.cpp \
    src/utils/misc/texthighlighter.cpp \
    src/utils/volume/volumerender.cpp \
    src/utils/worker/worker.cpp \
    src/utils/file/utils/framecontainer.cpp \
    src/utils/file/utils/selection.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/utils/misc/line.cpp \
    src/utils/misc/marker.cpp \
    src/utils/misc/linemodel.cpp
#    src/utils/glsl/glslfunctions.cpp

