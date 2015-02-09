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
    src/utils/opengl/glsl/std_2d_col.f.glsl \
    src/utils/opengl/glsl/std_2d_col.v.glsl \
    src/utils/opengl/glsl/std_2d_tex.f.glsl \
    src/utils/opengl/glsl/std_2d_tex.v.glsl \
    src/utils/opengl/glsl/std_3d_col.f.glsl \
    src/utils/opengl/glsl/std_3d_col.v.glsl \
    src/utils/opengl/glsl/std_blend.f.glsl \
    src/utils/opengl/glsl/std_blend.v.glsl \
    src/utils/opengl/glsl/unitcell.f.glsl \
    src/utils/opengl/glsl/unitcell.v.glsl \
    src/utils/opengl/glsl/rect_hl_2d_tex.f.glsl \
    src/utils/opengl/glsl/rect_hl_2d_tex.v.glsl \
    src/utils/opencl/cl/image_preview.cl \
    src/utils/opencl/cl/mem_functions.cl \
    src/utils/opencl/cl/parallel_reduce.cl \
    src/utils/opencl/cl/background_filter.cl

HEADERS += \
    src/utils/file/utils/fileformat.h \
    src/utils/file/utils/filetreeview.h \
    src/utils/file/qxfilelib.h \
    src/utils/image/utils/imagepreview.h \
    src/utils/image/qximagelib.h \
    src/utils/math/utils/ccmatrix.h \
    src/utils/math/utils/colormatrix.h \
    src/utils/math/utils/matrix.h \
    src/utils/math/utils/rotationmatrix.h \
    src/utils/math/utils/ubmatrix.h \
    src/utils/math/qxmathlib.h \
    src/utils/opencl/utils/contextcl.h \
    src/utils/opencl/qxopencllib.h \
#    src/utils/opengl/utils/openglwindow.h \
#    src/utils/opengl/utils/sharedcontext.h \
    src/utils/opengl/utils/transferfunction.h \
    src/utils/opengl/qxopengllib.h \
    src/utils/svo/qxsvolib.h \
    src/utils/lib.h \
    src/utils/svo/utils/bricknode.h \
    src/utils/svo/utils/searchnode.h \
    src/utils/svo/utils/sparsevoxelocttree.h \
    src/utils/marker.h \
    src/utils/texthighlighter.h \
    src/utils/volumerender.h \
    src/utils/worker.h \
    src/mainwindow.h \
    src/utils/file/utils/framecontainer.h \
    src/utils/file/utils/selection.h

SOURCES += \
    src/utils/file/utils/fileformat.cpp \
    src/utils/file/utils/filetreeview.cpp \
    src/utils/file/qxfilelib.cpp \
    src/utils/image/utils/imagepreview.cpp \
    src/utils/image/qximagelib.cpp \
    src/utils/math/qxmathlib.cpp \
    src/utils/opencl/utils/contextcl.cpp \
    src/utils/opencl/qxopencllib.cpp \
#    src/utils/opengl/utils/openglwindow.cpp \
#    src/utils/opengl/utils/sharedcontext.cpp \
    src/utils/opengl/utils/transferfunction.cpp \
    src/utils/opengl/qxopengllib.cpp \
    src/utils/svo/qxsvolib.cpp \
    src/utils/svo/utils/bricknode.cpp \
    src/utils/svo/utils/searchnode.cpp \
    src/utils/svo/utils/sparsevoxelocttree.cpp \
    src/utils/marker.cpp \
    src/utils/texthighlighter.cpp \
    src/utils/volumerender.cpp \
    src/utils/worker.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/utils/file/utils/framecontainer.cpp \
    src/utils/file/utils/selection.cpp
