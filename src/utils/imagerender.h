#ifndef IMAGERENDER_H
#define IMAGERENDER_H

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <string>

#include <QtGlobal>

/* GL and CL*/
#ifdef Q_OS_WIN
    #define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

/* QT */
//#include <QMouseEvent>
#include <QTimer>
#include <QElapsedTimer>
//#include <QDebug>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QGLWidget>

//#ifdef Q_OS_WIN
//    #include <windows.h>
//#elif defined Q_OS_LINUX
//    #include <GL/glx.h>
//#endif

#include <ft2build.h>
#include FT_FREETYPE_H



#include "tools.h"
//~ #include "fileformats.h"
#include "miniarray.h"
#include "matrix.h"
#include "atlas.h"

class ImageRenderGLWidget : public QGLWidget
{
    Q_OBJECT

public:
    explicit ImageRenderGLWidget(cl_device * device, cl_context * context, cl_command_queue * queue, const QGLFormat & format, QWidget *parent = 0, const QGLWidget * shareWidget = 0);
    ~ImageRenderGLWidget();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    cl_mem * getTsfImgCLGL();
    cl_mem * getAlphaImgCLGL();
    cl_mem * getGammaImgCLGL();
    cl_mem * getBetaImgCLGL();
    void setImageSize(int w, int h);

    //~void runFilterKernel(cl_kernel * kernel, size_t * loc_ws, size_t * glb_ws);

signals:
    void changedMessageString(QString str);

public slots:
    void setImageWidth(int value);
    void setImageHeight(int value);
    void aquireSharedBuffers();
    void releaseSharedBuffers();
    //~void setThreadFlag(bool value);


protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    //~void resizeEvent(QResizeEvent * event);
    //~void paintEvent(QPaintEvent * event);

private:
    void initFreetype();
    void setMessageString(QString str);
    void writeLog(QString str);

    QTimer * timer;
    QElapsedTimer * time;
    int image_w, image_h;
    int WIDTH, HEIGHT;
    int verbosity;

    TsfMatrix<double> transferFunction;

    MiniArray<float> white;
    MiniArray<float> transparent;
    MiniArray<float> black;
    MiniArray<float> blue;
    MiniArray<float> red;
    MiniArray<float> green;
    MiniArray<float> clear;
    MiniArray<float> clearInv;

    // OpenGL Related
    //~bool isInMainThread;
    bool isGLIntitialized;
    int initResourcesGL();
    void initializeProgramsGL();
    void setTexturesVBO();
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



    // OpenCL Related
    cl_mem alpha_img_clgl, beta_img_clgl, gamma_img_clgl, tsf_img_clgl;
    cl_device * device;
    cl_program program;
    cl_context * context;
    cl_command_queue * queue;
    cl_int err;

    int setTarget();
    void setTsfTexture(TsfMatrix<double> * tsf);

    size_t glb_ws[2];
    size_t loc_ws[2];

    bool isAlphaImgInitialized, isBetaImgInitialized, isGammaImgInitialized, isTsfImgInitialized;
};

#endif
