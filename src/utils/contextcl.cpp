#include "contextcl.h"

ContextCL::ContextCL()
{
}

void ContextCL::initialize()
{
    // DEVICE & INFO


    cl_uint max_platforms = 10; // Max number of platforms
    cl_uint num_platforms = 0;
    platforms.reserve (max_platforms);

    // Get platforms
    err = clGetPlatformIDs(max_platforms, platforms.data(), &num_platforms);
    if ( err != CL_SUCCESS) writeToLogAndPrint("Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)), "riv.log", 1);

    cl_uint max_devices = 10; // Max number of devices per platform
    cl_uint num_devices = 0;
    cl_uint num_devices_total = 0;
    devices.reserve (max_devices);

    // Get devices for platforms
    for (size_t i = 0; i < num_platforms; i++)
    {
        err = clGetDeviceIDs( platforms[i], CL_DEVICE_TYPE_ALL, max_devices, devices.data(), &num_devices);
        if ( err != CL_SUCCESS) writeToLogAndPrint("Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)), "riv.log", 1);

        for (size_t j = 0; j < num_devices; j++)
        {
            device_list.append (DeviceCL(platforms[i], devices[j]));
            writeToLogAndPrint(QString(device_list.last().getDeviceInfoString().c_str()), "riv.log", 1);
            num_devices_total++;
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
//    context = new cl_context;
//    queue = new cl_command_queue;

//    // Context with GL interopability
//    #ifdef Q_OS_LINUX
//    cl_context_properties properties[] = {
//        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
//        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
//        CL_CONTEXT_PLATFORM, (cl_context_properties) device->platform_id,
//        0};
//    #elif defined Q_OS_WIN
//    cl_context_properties properties[] = {
//        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
//        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
//        CL_CONTEXT_PLATFORM, (cl_context_properties) device->platform_id,
//        0};
//    #endif


//    *context = clCreateContext(properties, 1, &device->device_id, NULL, NULL, &err);
//    if (err != CL_SUCCESS)
//    {
//        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
//        return 0;
//    }

//    // Command queue
//    *queue = clCreateCommandQueue(*context, device->device_id, 0, &err);
//    if (err != CL_SUCCESS)
//    {
//        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
//        return 0;
//    }
//    return 1;
}
