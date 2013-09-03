#ifndef IMAGERENDER_H
#define IMAGERENDER_H

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

#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef __linux__
    #include <GL/glx.h>
#endif

#include "tools.h"
//~ #include "file_formats.h"
#include "miniarray.h"
#include "matrix.h"
#include "atlas.h"

class ImageRenderGLWidget : public QGLWidget
{
    Q_OBJECT
    
public:
    explicit ImageRenderGLWidget(cl_device * device, cl_context * context2, cl_command_queue * queue, const QGLFormat & format, QWidget *parent = 0, const QGLWidget * shareWidget = 0);
    ~ImageRenderGLWidget();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    cl_mem * getTsfImgCLGL();
    cl_mem * getRawImgCLGL();
    cl_mem * getCorrectedImgCLGL();
    void setImageSize(int w, int h);
    void aquireSharedBuffers();
    void releaseSharedBuffers();

    
public slots:
    //~ void setRawImage(PilatusFile * file);
    //~ void setCorrectedImage(PilatusFile * file);
    
signals:
    void changedMessageString(QString str);
    
protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    
private:
    void init_freetype();
    void setMessageString(QString str);
    
    bool isGLIntitialized;
    QTimer * timer;
    QElapsedTimer * time;
    int image_w, image_h;
    int WIDTH, HEIGHT;

    TsfMatrix<double> transferFunction;

    MiniArray<float> white;
    MiniArray<float> transparent;
    MiniArray<float> black;
    MiniArray<float> blue;
    MiniArray<float> red;
    MiniArray<float> green;

    // OpenGL Related
    int init_gl();
    void init_gl_programs();
    void screen_buffer_refresh();
    void std_2d_tex_draw(GLuint * elements, int num_elements, int active_tex, GLuint texture, GLuint * xy_coords, GLuint * tex_coords);
    void std_2d_color_draw(GLuint * elements, int num_elements, GLfloat * color, GLuint * xy_coords);
    void std_text_draw(const char *text, Atlas *a, float * color, float * xy, float scale, int w, int h);
    void setTexturePositions();

    Atlas * fontSmall;
    Atlas * fontMedium;
    Atlas * fontLarge;
    
    GLuint screen_texpos_vbo[5], screen_coord_vbo[5];
    GLuint text_coord_vbo, text_texpos_vbo;
    GLuint std_2d_tex_program, std_2d_color_program, std_text_program;
    GLuint image_tex[5];
    GLint std_text_attribute_position, std_text_uniform_color, std_text_uniform_tex, std_text_attribute_texpos;
    GLint std_2d_tex_attribute_position, std_2d_tex_attribute_texpos, std_2d_tex_uniform_color, std_2d_tex_uniform_texture, std_2d_tex_uniform_time, std_2d_tex_uniform_pixel_size;
    GLint std_2d_color_attribute_position, std_2d_color_uniform_color;

    void runFilterKernel(cl_kernel * kernel, size_t * loc_ws, size_t * glb_ws);
    
    // OpenCL Related
    int init_cl();
    //~ cl_sampler source_sampler, tsf_tex_sampler;
    cl_mem raw_target_cl, corrected_target_cl, tsf_tex_cl;
    //~ cl_kernel K_FRAME_TO_IMAGE;
    cl_device * device;
    cl_program program;
    cl_context * context2;
    cl_command_queue * queue;
    cl_int err;

    int setTarget();
    void setSource();
    void setTsfTexture(TsfMatrix<double> * tsf);
    
    size_t glb_ws[2];
    size_t loc_ws[2];
};

#endif
