#ifndef IMAGERENDER_H
#define IMAGERENDER_H

/*
 * A class to render data files real-time during reconstruction
 * */

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <string>

#include <QtGlobal>
#include <QDebug>
#include <QRect>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QResizeEvent>

#include "tools.h"
//#include "miniarray.h"
#include "matrix.h"
#include "openglwindow.h"
#include "transferfunction.h"
#include "sharedcontext.h"

#include <QScreen>
#include <QPainter>


class ImageRenderWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImageRenderWorker(QObject *parent = 0);
    ~ImageRenderWorker();

    void setSharedWindow(SharedContextWindow * window);

    cl_mem * getTsfImgCLGL();
    cl_mem * getAlphaImgCLGL();
    cl_mem * getGammaImgCLGL();
    cl_mem * getBetaImgCLGL();

public slots:
    void setImageSize(int w, int h);
    void aquireSharedBuffers();
    void releaseSharedBuffers();
    void mouseMoveEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void test();

protected:
    void initialize();
    void render(QPainter *painter);

private:
    SharedContextWindow * shared_window;

    QRect alpha_rect;
    QRect beta_rect;
    QRect gamma_rect;

    // Image frames
    int image_w, image_h;

    // Boolean checks
    bool isInitialized;
    bool isAlphaImgInitialized, isBetaImgInitialized, isGammaImgInitialized, isTsfImgInitialized;

    // OpenGL
    void initResourcesGL();
    GLuint image_tex[5];

    // OpenCL
    cl_mem cl_img_alpha, cl_img_beta, cl_img_gamma;
    cl_int err;
    void setTarget();

    // Drawing functions
    void drawData();
    void drawOverlay(QPainter * painter);
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);

    // Transfer function texture
    void setTsfTexture();
    cl_mem cl_tsf_tex;
    cl_sampler tsf_tex_sampler;
    GLuint tsf_tex_gl;
    GLuint tsf_tex_gl_thumb;
    bool isTsfTexInitialized;
    TransferFunction tsf;
    int tsf_color_scheme;
    int tsf_alpha_scheme;

    size_t glb_ws[2];
    size_t loc_ws[2];

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

class ImageRenderWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    ImageRenderWindow();
    ~ImageRenderWindow();

    void setSharedWindow(SharedContextWindow * window);
    ImageRenderWorker * getWorker();

    void initializeWorker();

public slots:
    void renderNow();

public slots:
    void test();

signals:
    void changedMessageString(QString str);

private:
    SharedContextWindow * shared_window;
    ImageRenderWorker * gl_worker;
    
    bool isInitialized;
};

#endif
