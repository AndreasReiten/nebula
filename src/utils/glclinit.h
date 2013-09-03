#ifndef GLCLINIT_H
#define GLCLINIT_H

/* Useful C++ libs */
#include <iostream>

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

#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef __linux__
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

private:
    void setMessageString(QString str);

    // OpenGL Related
    int init_gl();
    bool isGLIntitialized;

    // OpenCL Related
    int init_cl_device(int verbose);
    int init_cl();
    cl_device * device; // Might have to declare with new
    cl_context * context2;
    cl_command_queue * queue;
    cl_uint num_devices;
    cl_uint num_platforms;
    cl_int err;
};

#endif
