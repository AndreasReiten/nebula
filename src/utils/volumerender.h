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
#include <QDebug>

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

#include <QScreen>
#include <QPainter>



class VolumeRenderWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit VolumeRenderWorker(QObject *parent = 0);
    ~VolumeRenderWorker();

    void setSharedWindow(SharedContextWindow * window);
    void setSvo(SparseVoxelOcttree * svo);

public slots:
    void setQuality(int value);
    void setScalebar();
    void setProjection();
    void setBackground();
    void setLogarithmic();
    void setDataStructure();
    void setTsfColor(int value);
    void setTsfAlpha(int value);
    void setDataMin(double value);
    void setDataMax(double Value);
    void setAlpha(double value);
    void setBrightness(double value);
    void setUnitcell();
    void setModel();
    void setModelParam0(double value);
    void setModelParam1(double value);
    void setModelParam2(double value);
    void setModelParam3(double value);
    void setModelParam4(double value);
    void setModelParam5(double value);
    void mouseMoveEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);

protected:
    void initialize();
    void render(QPainter *painter);

private:
    SharedContextWindow * shared_window;

    // Boolean checks
    bool isInitialized;
    bool isDSActive;
    bool isOrthonormal;
    bool isLogarithmic;
    bool isModelActive;
    bool isUnitcellActive;
    bool isSvoInitialized;
    bool isScalebarActive;

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
    double work, work_time, quality_factor;

    // Drawing functions
    void drawRayTex();
    void drawScalebars();
    void drawOverlay(QPainter *painter);
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);
    int fps_string_width_prev;

    // Core set functions
    void setDataExtent();
    void setViewMatrix();
    void resetViewMatrix();
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
    int tsf_color_scheme;
    int tsf_alpha_scheme;

    // Ray texture timing
    float fps_required;
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
    Matrix<double> tsf_parameters_model;
    Matrix<double> tsf_parameters_svo;
    Matrix<int> misc_ints;
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
    cl_mem cl_scalebar_matrix;
    cl_mem cl_data_extent;
    cl_mem cl_data_view_extent;
    cl_mem cl_tsf_parameters_model;
    cl_mem cl_tsf_parameters_svo;
    cl_mem cl_misc_ints;
    cl_mem cl_model_misc_floats;

    void initResourcesCL();

    // Svo
    cl_mem cl_svo_pool;
    cl_mem cl_svo_index;
    cl_mem cl_svo_brick;
    cl_sampler cl_svo_pool_sampler;

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

class VolumeRenderWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    VolumeRenderWindow();
    ~VolumeRenderWindow();

    void setSharedWindow(SharedContextWindow * window);
    VolumeRenderWorker *getWorker();

    void initializeWorker();

public slots:
    void renderNow();

private:
    bool isInitialized;

    SharedContextWindow * shared_window;
    VolumeRenderWorker * gl_worker;
};

#endif
