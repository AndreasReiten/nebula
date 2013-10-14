#include "contextcl.h"

ContextCL::ContextCL()
{
}

cl_command_queue * ContextCL::getCommanQueue()
{
    return &queue;
}

cl_context * ContextCL::getContext()
{
    return &context;
}

QList<DeviceCL> * ContextCL::getDeviceList()
{
    return &device_list;
}

void ContextCL::initDevices()
{
    // Platforms and Devices
    cl_uint max_platforms = 10; // Max number of platforms
    cl_uint num_platforms = 0;
    platforms.reserve (max_platforms);

    // Get platforms
    err = clGetPlatformIDs(max_platforms, platforms.data(), &num_platforms);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_uint max_devices = 10; // Max number of devices per platform
    cl_uint nupaint_device_gls = 0;
    cl_uint nupaint_device_gls_total = 0;
    devices.reserve (max_devices);

    // Get devices for platforms
    for (size_t i = 0; i < num_platforms; i++)
    {
        err = clGetDeviceIDs( platforms[i], CL_DEVICE_TYPE_ALL, max_devices, devices.data(), &nupaint_device_gls);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        for (size_t j = 0; j < nupaint_device_gls; j++)
        {
            device_list.append (DeviceCL(platforms[i], devices[j]));
            qDebug() << device_list.last().getDeviceInfoString().c_str();
            nupaint_device_gls_total++;
        }
    }

    // Find suitable devices
    for (int i = 0; i < device_list.size(); i++)
    {
        if (device_list[i].getDeviceType() != CL_DEVICE_TYPE_GPU)
        {
            device_list.removeAt(i);
            i--;
        }
    }

    // Re-populate devices with only suitable devices
    devices.reserve(device_list.size());
    for (int i = 0; i < device_list.size(); i++)
    {
        devices[i] = device_list[i].getDeviceId();
    }
}

void ContextCL::initSharedContext()
{
    // Context with GL interopability

    #ifdef Q_OS_LINUX
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) device_list[0].getPlatformId(),
        0};

//        std::cout << glXGetCurrentContext() << std::endl;
//        std::cout << glXGetCurrentDisplay() << std::endl;

    #elif defined Q_OS_WIN
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) device_list[0].getPlatformId(),
        0};
    #endif

    context = clCreateContext(properties, devices.size(), devices.data(), NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void ContextCL::initCommandQueue()
{
    // Command queue
    queue = clCreateCommandQueue(context, device_list[0].getDeviceId(), 0, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}


