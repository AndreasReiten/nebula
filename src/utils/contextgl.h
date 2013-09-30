#ifndef GLCLINIT_H
#define GLCLINIT_H

#include <QtGlobal>

/* Useful C++ libs */
#include <iostream>
#include <sstream>


/* GL and CL*/
#ifdef Q_OS_WIN
    #define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

/* QT */
#include <QGLWidget>
#include <QMouseEvent>

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined Q_OS_LINUX
    #include <GL/glx.h>
#endif

#include "miniarray.h"
#include "tools.h"



class ContextGLWidget : public QGLWidget
{
    Q_OBJECT

public:
    explicit ContextGLWidget(const QGLFormat & format, QWidget * parent = 0);
    ~ContextGLWidget();

    cl_command_queue * getCLCommandQueue();
    cl_device * getCLDevice();
    cl_context * getCLContext();

signals:
    void changedMessageString(QString str);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    void writeLog(QString str);
    //~void paintOverlayGL();
    //~void resizeOverlayGL(int w, int h);
    //~void initializeOverlayGL();

private:
    void setMessageString(QString str);
    int verbosity;

    // OpenGL Related
    int initResourcesGL();
    bool isGLIntitialized;

    // OpenCL Related
    int initDeviceCL(int verbosity);
    int initResourcesCL();
    cl_device * device; // Might have to declare with new
    cl_context * context;
    cl_command_queue * queue;
    cl_uint num_devices;
    cl_uint num_platforms;
    cl_int err;
};

#endif
