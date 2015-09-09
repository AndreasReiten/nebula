# QMAKE PROJECT FILE

# Modules and project configuration
QT += core gui sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

win32 {
    INCLUDEPATH = khronos/
    RC_ICONS = art/app.ico
    LIBS += -lOpenGL32
}

unix {
    INCLUDEPATH = khronos/
    QMAKE_CXXFLAGS += -std=c++0x # C++11
}

TARGET = nebula
RESOURCES  = nebula.qrc

OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui

# Files
OTHER_FILES += \
    shaders/std_2d_col.f.glsl \
    shaders/std_2d_col.v.glsl \
    shaders/std_2d_tex.f.glsl \
    shaders/std_2d_tex.v.glsl \
    shaders/std_3d_col.f.glsl \
    shaders/std_3d_col.v.glsl \
    shaders/std_blend.f.glsl \
    shaders/std_blend.v.glsl \
    shaders/std_2d_sprite.v.glsl \
    shaders/std_2d_sprite.f.glsl \
    kernels/voxelize.cl \
    kernels/models.cl \
    kernels/weightpoint.cl \
    kernels/volume_render_svo.cl \
    kernels/volume_render_shared.cl \
    kernels/volume_render_model.cl \
    kernels/parallel_reduction.cl \
    kernels/integrate_plane.cl \
    kernels/integrate_line.cl \
    kernels/integrate_image.cl \
    kernels/scattering_data_operations.cl \
    kernels/volume_sampler.cl \
    kernels/parallel_reduction.cl

HEADERS += \
    file/fileformat.h \
    file/filetreeview.h \
    image/imagepreview.h \
    math/ccmatrix.h \
    math/colormatrix.h \
    math/boxmatrix.h \
    math/matrix.h \
    math/rotationmatrix.h \
    math/ubmatrix.h \
    opencl/contextcl.h \
    misc/transferfunction.h \
    svo/bricknode.h \
    svo/searchnode.h \
    svo/sparsevoxeloctree.h \
    misc/texthighlighter.h \
    volume/volumerender.h \
    worker/worker.h \
    mainwindow.h \
    file/framecontainer.h \
    file/selection.h \
    misc/line.h \
    misc/marker.h \
    misc/linemodel.h \
    misc/plotwidget.h \
    misc/imagemarker.h \
    misc/box.h \
    reconstructionwidget.h \
    visualizationwidget.h \
    sql/customsqlquerymodel.h \
    sql/sqlqol.h

SOURCES += \
    main.cpp \
    file/fileformat.cpp \
    file/filetreeview.cpp \
    image/imagepreview.cpp \
    opencl/contextcl.cpp \
    misc/transferfunction.cpp \
    svo/bricknode.cpp \
    svo/searchnode.cpp \
    svo/sparsevoxeloctree.cpp \
    misc/texthighlighter.cpp \
    volume/volumerender.cpp \
    worker/worker.cpp \
    file/framecontainer.cpp \
    file/selection.cpp \
    mainwindow.cpp \
    misc/line.cpp \
    misc/marker.cpp \
    misc/linemodel.cpp \
    misc/plotwidget.cpp \
    misc/imagemarker.cpp \
    misc/box.cpp \
    reconstructionwidget.cpp \
    visualizationwidget.cpp \
    sql/customsqlquerymodel.cpp

FORMS += \
    mainwindow.ui \
    reconstructionwidget.ui \
    visualizationwidget.ui

DISTFILES +=
