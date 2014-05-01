#ifndef CONTEXTCL_H
#define CONTEXTCL_H

/*
 * This class initializes an OpenCL context.
 * */

#include <iostream>
#include <sstream>
#include <string>

#include <CL/opencl.h>
#include <CL/cl_gl.h>

#include <QString>
#include <QList>
#include <QOpenGLContext>
#include <QDebug>

#include "devicecl.h"
#include "tools.h"

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined Q_OS_LINUX
    #include <GL/glx.h>
#endif

class OpenCLContext
{
public:
    OpenCLContext();
    void initDevices();
    void initSharedContext();
    void initCommandQueue();
    const cl_command_queue *getCommandQueue();
    cl_context * getContext();
    QList<DeviceCL> * getDeviceList();
    DeviceCL * getMainDevice();

    cl_program createProgram(Matrix<const char *> * paths, cl_int * error);
    void buildProgram(cl_program * program, const char * options);

private:
    Matrix<cl_platform_id> platforms;
    Matrix<cl_device_id> devices;

    DeviceCL *main_device;
    QList<DeviceCL> device_list;

    cl_command_queue queue;
    cl_context context;
    cl_int err;
};

#endif // CONTEXTCL_H
