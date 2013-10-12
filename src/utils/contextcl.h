#ifndef CONTEXTCL_H
#define CONTEXTCL_H

#include <CL/opencl.h>

#include "devicecl.h"
#include "tools.h"
#include "miniarray.h"

#include <QString>
#include <QList>

#include <iostream>
#include <sstream>
#include <string>

class ContextCL
{
public:
    ContextCL();
    void initialize();

private:
    MiniArray<cl_platform_id> platforms;
    MiniArray<cl_device_id> devices;

    QList<DeviceCL> device_list;

    cl_command_queue queue;
    cl_context context;
    cl_int err;
};

#endif // CONTEXTCL_H
