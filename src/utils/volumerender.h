#ifndef VOLUMERENDER_H
#define VOLUMERENDER_H

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>


#include <QtGlobal>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QResizeEvent>


#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "sparsevoxelocttree.h"
#include "openglwindow.h"
#include "sharedcontext.h"
#include "transferfunction.h"

#include <QMatrix4x4>

#include <QScreen>
#include <QPainter>

#include <QtCore/qmath.h>


class VolumeRenderWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    VolumeRenderWindow();
    ~VolumeRenderWindow();

    void setSharedWindow(SharedContextWindow * window);

protected:
    void initialize();
    void render(QPainter *painter);
    void mouseMoveEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);

private:
    SharedContextWindow * shared_window;

    // Boolean checks
    bool isInitialized;
    bool isDSViewForced;
    bool isDSActive;
    bool isOrthonormal;
    bool isLogarithmic;

    // Ray texture
    Matrix<int> ray_tex_dim;
    Matrix<size_t> ray_glb_ws;
    Matrix<size_t> ray_loc_ws;
    float ray_tex_resolution;
    cl_mem ray_tex_cl;
    GLuint ray_tex_gl;
    bool isRayTexInitialized;
    void setRayTexture();
    void raytrace(cl_kernel kernel, cl_kernel workload);
    void setQuality(double value);
    double work, work_time, quality_factor;

    // Drawing functions
    void drawRayTex();
    void drawScalebars();
    void drawOverlay(QPainter *painter);
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);

    // Core set functions
    void setDataExtent();
    void setViewMatrix();
    void setTsfParameters();
    void setMiscArrays();

    // Scalebars
    size_t setScaleBars();
    size_t scalebar_coord_count;
    GLuint scalebar_vbo;
    Matrix<float> scalebar_ticks;
    int n_scalebar_ticks;
    double scalebar_multiplier;

    // Transfer function texture
    void setTsfTexture();
    cl_mem tsf_tex_cl;
    cl_sampler tsf_tex_sampler;
    GLuint tsf_tex_gl;
    GLuint tsf_tex_gl_thumb;
    bool isTsfTexInitialized;
    TransferFunction tsf;

    // Ray texture timing
    bool isBadCall;
    bool isRefreshRequired;
    float callTimeMax;
    float timeLastActionMin;
    float fps_required;
    QElapsedTimer *timerLastAction;
    QElapsedTimer *callTimer;
    QElapsedTimer ray_kernel_timer;

    // Mouse
    int last_mouse_pos_x;
    int last_mouse_pos_y;

    // View matrices
    Matrix<double> view_matrix;
    CameraToClipMatrix<double> ctc_matrix;
    RotationMatrix<double> rotation;
    Matrix<double> data_translation;
    Matrix<double> data_scaling;
    Matrix<double> bbox_scaling;
    Matrix<double> bbox_translation;
    Matrix<double> normalization_scaling;
    Matrix<double> scalebar_view_matrix;
    RotationMatrix<double> scalebar_rotation;
    Matrix<double> projection_scaling;

    // Other matrices
    Matrix<double> data_extent;
    Matrix<double> data_view_extent;
    Matrix<double> tsf_parameters;
    Matrix<int> misc_ints;
    Matrix<double> svo_misc_floats;
    Matrix<double> model_misc_floats;

    // OpenGL
    void initResourcesGL();

    // OpenCL
    cl_int err;
    cl_program program;
    cl_kernel cl_svo_raytrace;
    cl_kernel cl_model_raytrace;
    cl_kernel cl_svo_workload;
    cl_kernel cl_model_workload;

    cl_mem cl_glb_work;
    cl_mem cl_view_matrix_inverse;
    cl_mem cl_data_extent;
    cl_mem cl_data_view_extent;
    cl_mem cl_tsf_parameters;
    cl_mem cl_misc_ints;
    cl_mem cl_svo_misc_floats;
    cl_mem cl_model_misc_floats;

    void initResourcesCL();

    // Colors
    Matrix<GLfloat> white;
    Matrix<GLfloat> black;
    Matrix<GLfloat> clear_color;
    Matrix<GLfloat> clear_color_inverse;

    // Pens
    void initializePaintTools();
    QPen * normal_pen;
    QPen * border_pen;
    QBrush * fill_brush;
    QBrush * normal_brush;
    QBrush * dark_fill_brush;
    QFont * normal_font;
    QFont * emph_font;
    QFontMetrics * normal_fontmetric;
    QFontMetrics * emph_fontmetric;
};
#endif
