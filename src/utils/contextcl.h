#ifndef CONTEXTCL_H
#define CONTEXTCL_H

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
#include "miniarray.h"

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined Q_OS_LINUX
    #include <GL/glx.h>
#endif

class ContextCL
{
public:
    ContextCL();
    void initDevices();
    void initSharedContext();
    void initCommandQueue();
    cl_command_queue * getCommanQueue();
    cl_context * getContext();
    QList<DeviceCL> * getDeviceList();

private:
    MiniArray<cl_platform_id> platforms;
    MiniArray<cl_device_id> devices;

    QList<DeviceCL> device_list;

    cl_command_queue queue;
    cl_context context;
    cl_int err;
};

#endif // CONTEXTCL_H
