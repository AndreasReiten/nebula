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
#include <QDebug>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QMutex>

#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "openglwindow.h"

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

signals:
    void changedMessageString(QString str);

public slots:
    void setImageWidth(int value);
    void setImageHeight(int value);
    void aquireSharedBuffers();
    void releaseSharedBuffers();


protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);


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
    bool isGLIntitialized;
    int initResourcesGL();
    void initializeProgramsGL();
    void setTexturesVBO();
    void setTexturePositions();

    GLuint image_tex[5];

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
