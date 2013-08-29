#ifndef VOLUMERENDER_H
#define VOLUMERENDER_H

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>

/* GL and CL*/
#ifdef _WIN32
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

/* QT */
#include <QMouseEvent>
#include <QGLWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QString>
#include <QByteArray>
#include <QDateTime>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <GL/glx.h>
#endif

#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "atlas.h"

class VolumeRenderGLWidget : public QGLWidget
{
    Q_OBJECT
    
public:
    explicit VolumeRenderGLWidget(cl_device * device, cl_context * context2, cl_command_queue * queue, const QGLFormat & format, QWidget *parent = 0, const QGLWidget * shareWidget = 0);
    ~VolumeRenderGLWidget();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setOCT_INDEX(MiniArray<unsigned int> * OCT_INDEX, size_t n_levels, float * extent);
    void setOCT_BRICK(MiniArray<unsigned int> * OCT_BRICK, size_t pool_power);
    void setBRICKS(MiniArray<float> * BRICKS, size_t n_bricks, size_t dim_bricks);
    void setMeta(MiniArray<double> * HIST_NORM, MiniArray<double> * HIST_LOG, MiniArray<double> * HIST_MINMAX, MiniArray<char> * SVO_COMMENT);
    void setMatrixU(float * buf);
    void setMatrixB(float * buf);
    
    
public slots:
    void takeScreenshot();
    void setResolutionf(double value);
    void setResolutioni(int value);
    void togglePerspective();
    void toggleBackground();
    //~ void toggleFastMove(bool value);
    void setTsf(int value);
    void setTsfMin(double value);
    void setTsfMax(double value);
    void setTsfAlpha(double value);
    void setTsfBrightness(double value);
    void toggleLog();
    void toggleDataStructure();
    void toggleFunctionView();
    void setFuncParamA(double value);
    void setFuncParamB(double value);
    void setFuncParamC(double value);
    void setFuncParamD(double value);
    void toggleUnitcellView();
    void setHklFocus(const QString str);
    void setTsfAlphaStyle(int value);
    
signals:
    void changedMessageString(QString str);
    void changedResolutioni(int value);
    void changedResolutionf(double value);
    void changedAlphaValue(double value);
    void changedBrightnessValue(double value);
    void changedDataMinValue(double value);
    void changedDataMaxValue(double value);
    void changedFuncParamA(double value);
    void changedFuncParamB(double value);
    void changedFuncParamC(double value);
    void changedFuncParamD(double value);
    
protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    
private:
    Atlas * fontSmall;
    Atlas * fontMedium;
    Atlas * fontLarge;

    void setEmit();
    void getScreenPosition(float * screen_pos, float * space_pos, float * transform);
    void std_text_draw(const char *text, Atlas *a, float * color, float * xy, float scale, int w, int h);
    void init_freetype();
    void setMinLevel(float level);
    int getUnitcellVBO(GLuint * xyz_coords, int * hkl_low, int * hkl_high, float * a, float * b, float * c);
    void getUnitcellBasis(float * buf, int * hkl_offset, float * a, float * b, float * c);
    void setProjection(double F, double N, double fov, bool isPerspectiveRequired);
    void pixPos(GLuint * vbo, float x, float y, float w, float h);
    void backDropTexPos(GLuint * vbo, int border_pixel_offset);
    void histTexPos(int log, float hist_min, float hist_max, float data_min, float data_max);
    void screen_buffer_refresh();
    void vbo_buffers_refresh();
    void view_matrix_reset();
    void view_matrix_refresh();
    void data_extent_refresh();
    void setTsfParameters();
    void setMiscArrays();
    void std_2d_tex_draw(GLuint * elements, int num_elements, int active_tex, GLuint texture, GLuint * xy_coords, GLuint * tex_coords);
    void std_3d_color_draw(GLuint * elements, int num_elements, GLfloat * color, GLuint * xyz_vbo, float * M1, float * M2, float * bbox_min, float * bbox_max );
    void gen_tsf_tex(MiniArray<float> * tsf_buf);
    void auto_rotate(int time, int treshold);
    void init_gl_programs();
    void setMessageString(QString str);
    void brick_to_tex(float * buf_in, float * buf_out, size_t id, size_t brick_dim, size_t pool_power);
    void ray_tex_refresh(cl_kernel kernel);
    void std_2d_color_draw(GLuint * elements, int num_elements, GLfloat * color, GLuint * xy_coords);
    //~ void paint_texture_2d(GLuint texture, double * coordinates, double * vertices);
    
    void getHistogramTexture(GLuint * tex, MiniArray<double> * buf, size_t height, float * color);
    
    GLint msaa_hdr_attribute_position , msaa_hdr_attribute_texpos , msaa_hdr_uniform_samples , msaa_hdr_uniform_method , msaa_hdr_uniform_exposure , msaa_hdr_uniform_texture , msaa_hdr_uniform_weight;
    GLuint hist_tex_norm, hist_tex_log;
    GLint std_text_attribute_position, std_text_uniform_color, std_text_uniform_tex, std_text_attribute_texpos;
    GLuint MSAA_FBO, STD_FBO, SMALL_FBO;
    GLuint std_3d_tex, std_2d_tex, msaa_tex, msaa_depth_tex, glow_tex, blend_tex, small_storage_tex, storage_tex, mini_uc_tex, msaa_intermediate_storage_tex;
    GLuint tex_coord_vbo[10], position_vbo[10], lab_reference_vbo[5], data_extent_vbo[5], data_view_extent_vbo[5], text_position_vbo, text_texpos_vbo;
    GLuint unitcell_vbo[5], screen_vbo[5];
    GLuint sampleWeightBuf;
    GLint std_3d_attribute_position, std_3d_attribute_texpos, std_3d_uniform_transform, std_3d_uniform_color, std_3d_uniform_texture, std_3d_uniform_time, std_3d_uniform_bbox_min, std_3d_uniform_bbox_max, std_3d_uniform_u;
    GLint pp_glow_attribute_position, pp_glow_attribute_texpos, pp_glow_uniform_color, pp_glow_uniform_texture, pp_glow_uniform_time, pp_glow_uniform_pixel_size, pp_glow_uniform_orientation, pp_glow_uniform_scale, pp_glow_uniform_deviation, pp_glow_uniform_samples;
    GLint blend_attribute_position, blend_attribute_texpos, blend_uniform_top_tex, blend_uniform_bot_tex;
    GLint msaa_attribute_position, msaa_attribute_texpos, msaa_uniform_texture, msaa_uniform_samples, msaa_uniform_weight;
    GLint std_2d_tex_attribute_position, std_2d_tex_attribute_texpos, std_2d_tex_uniform_color, std_2d_tex_uniform_texture, std_2d_tex_uniform_time, std_2d_tex_uniform_pixel_size;
    GLint std_2d_color_attribute_position, std_2d_color_uniform_color;
    GLuint std_2d_tex_program, std_2d_color_program, std_3d_program, pp_glow_program, blend_program, msaa_program, msaa_hdr_program, std_text_program;
    GLuint ray_tex, tsf_tex, hist_tex, static_texture[5];
    
    cl_image_format bricks_format;
    cl_sampler bricks_sampler, tsf_tex_sampler;
    cl_mem ray_tex_cl, tsf_tex_cl;
    cl_mem view_matrix_inv_cl;
    cl_mem bbox_extent_cl, data_extent_cl, data_view_extent_cl, tsf_parameters_cl, misc_float_cl, misc_int_cl, misc_float_k_raytrace_cl;
    cl_mem bricks_cl, oct_index_cl, oct_brick_cl;
    cl_kernel K_SVO_RAYTRACE, K_FUNCTION_RAYTRACE;
    cl_device * device;
    cl_program program;
    cl_context * context2;
    cl_command_queue * queue;
    cl_int err;
    
    int pp_samples;
    float pp_deviation;
    float pp_scale;
    
    RotationMatrix<float> X_ROTATION;
    RotationMatrix<float> Y_ROTATION;
    RotationMatrix<float> Z_ROTATION;
    RotationMatrix<float> ROTATION;
    RotationMatrix<float> ROLL_ROTATION;
    RotationMatrix<float> AUTO_ROTATION;
    
    CameraToClipMatrix<float> CTC_MATRIX;
    
    Matrix<float> origo;
    Matrix<float> pixel_size;
    //~ Matrix<float> std_texcoords;
    Matrix<float> MISC_FLOAT_K_FUNCTION;
    Matrix<float> MISC_FLOAT_K_RAYTRACE;
    Matrix<int> MISC_INT;
    Matrix<float> TSF_PARAMETERS;
    Matrix<float> BBOX_SCALING;
    Matrix<float> BBOX_EXTENT;
    Matrix<float> BBOX_TRANSLATION;
    Matrix<float> BBOX_VIEW_MATRIX;
    Matrix<float> DATA_EXTENT;
    Matrix<float> DATA_SCALING;
    Matrix<float> DATA_TRANSLATION;
    Matrix<float> NORM_SCALING;
    Matrix<float> DATA_VIEW_EXTENT;
    Matrix<float> DATA_VIEW_MATRIX;
    Matrix<float> CELL_VIEW_MATRIX;
    Matrix<float> MINI_CELL_VIEW_MATRIX;
    Matrix<float> PROJECTION_SCALING;
    Matrix<float> U;
    Matrix<float> U3x3;
    Matrix<float> I;
    Matrix<float> a;
    Matrix<float> b;
    Matrix<float> c;
    
    MiniArray<GLuint> hkl_indices;
    MiniArray<float> color;
    MiniArray<float> * BRICKS;
    MiniArray<unsigned int> * OCT_INDEX;
    MiniArray<unsigned int> * OCT_BRICK;
    MiniArray<float> tsf_buf;
    MiniArray<double> HIST_MINMAX;
    MiniArray<double> * HIST_NORM;
    MiniArray<double> * HIST_LOG;
    Matrix<float> hkl_text_pos;
    Matrix<float> hkl_text_index;
    
    QTimer *timer;
    QElapsedTimer *time;
    QElapsedTimer *rotationTimer;
    QElapsedTimer *timerLastAction;
    QElapsedTimer *callTimer;
    
    float LINEWIDTH;
    bool isUnitcellActive;
    bool isFunctionActive;
    bool isBadCall;
    bool isRefreshRequired;
    bool isPerspectiveRequired;
    bool isUnitcellValid;
    
    int isLog;
    int callTimeMax;
    int timeLastActionMin;
    int auto_rotation_delay; 
    int MSAA_SAMPLES, MSAA_METHOD;
    float MSAA_EXPOSURE;
    int WIDTH, HEIGHT, SMALL_WIDTH, SMALL_HEIGHT;
    int ray_tex_dim[2];
    int gen_ray_tex();
    int init_gl();
    int init_cl();
    
    size_t ray_glb_ws[2];
    size_t ray_loc_ws[2];
    size_t LEVELS, BPP, N_BRICKS, DIM_BRICKS;
    
    float ray_res;
    
    double N, F, fov;
    double zeta, eta;
    double X_rotation, Y_rotation;
    double lastPos_x, lastPos_y;


    char tsf_alpha_style;
    char tsf_style;
    
    MiniArray<float> white;
    MiniArray<float> transparent;
    MiniArray<float> black;
    MiniArray<float> blue;
    MiniArray<float> red;
    MiniArray<float> green;
    MiniArray<float> clear;
    MiniArray<float> clearInv;
    MiniArray<float> colorBackDropA;
    MiniArray<float> colorBackDropB;
    MiniArray<float> colorBackDropC;

    bool isGLIntitialized;
};

#endif
