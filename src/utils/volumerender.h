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

    // Ray texture
    Matrix<int> ray_tex_dim;
    Matrix<size_t> ray_glb_ws;
    Matrix<size_t> ray_loc_ws;
    float ray_tex_resolution;
    cl_mem ray_tex_cl;
    GLuint ray_tex_gl;
    bool isRayTexInitialized;
    void setRayTexture();
    void raytrace(cl_kernel kernel);

    // Core set functions
    void setDataExtent();
    void setViewMatrix();
    void setTsfParameters();
    void setMiscArrays();

    // Scalebars
    size_t setScaleBars();
    Matrix<GLfloat> scalebar_coords;
    size_t scalebar_coord_count;

    // Transfer function texture
    void setTsfTexture();
    cl_mem tsf_tex_cl;
    cl_sampler tsf_tex_sampler;
    GLuint tsf_tex_gl;
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

    // OpenCL
    cl_int err;
    cl_program program;
    cl_kernel cl_svo_raytrace;
    cl_kernel cl_model_raytrace;

    cl_mem cl_view_matrix_inverse;
    cl_mem cl_data_extent;
    cl_mem cl_data_view_extent;
    cl_mem cl_tsf_parameters;
    cl_mem cl_misc_ints;
    cl_mem cl_svo_misc_floats;
    cl_mem cl_model_misc_floats;

    void initResourcesCL();

    // Colors
    Matrix<float> clear_color;
    Matrix<float> clear_color_inverse;
    Matrix<float> white;
    Matrix<float> black;
};
#endif
