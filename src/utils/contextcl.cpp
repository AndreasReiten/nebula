#include "contextcl.h"

OpenCLContext::OpenCLContext()
{
}

const cl_command_queue * OpenCLContext::getCommandQueue()
{
    return &queue;
}

cl_context * OpenCLContext::getContext()
{
    return &context;
}

QList<DeviceCL> * OpenCLContext::getDeviceList()
{
    return &device_list;
}

DeviceCL * OpenCLContext::getMainDevice()
{
    return main_device;
}

cl_program OpenCLContext::createProgram(Matrix<const char *> * paths, cl_int * error)
{
    // Program
    Matrix<size_t> lengths(1, paths->size());
    Matrix<const char *> sources(1, paths->size());
    Matrix<QByteArray> qsources(1, paths->size());

    for (size_t i = 0; i < paths->size(); i++)
    {
        qsources[i] = openFile(paths->at(i));
        sources[i] = qsources[i].data();
        lengths[i] = strlen(sources[i]);
    }
    return clCreateProgramWithSource(context, paths->size(), sources.data(), lengths.data(), error);
}

void OpenCLContext::buildProgram(cl_program * program, const char * options)
{
    // Compile kernel
    cl_device_id tmp = main_device->getDeviceId();
    err = clBuildProgram(*program, 1, &tmp, options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // Compile log
        qDebug() << "Error compiling kernel: "+QString(cl_error_cstring(err));
        std::stringstream ss;

        char* build_log;
        size_t log_size;

        clGetProgramBuildInfo(*program, main_device->getDeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        build_log = new char[log_size+1];

        clGetProgramBuildInfo(*program, main_device->getDeviceId(), CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
        build_log[log_size] = '\0';

        ss << "___ START KERNEL COMPILE LOG ___" << std::endl;
        ss << build_log << std::endl;
        ss << "___  END KERNEL COMPILE LOG  ___" << std::endl;
        delete[] build_log;

        qDebug(ss.str().c_str());
    }
}


void OpenCLContext::initDevices()
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

    // Pick a preferred device
    main_device = &device_list[0];
}

void OpenCLContext::initSharedContext()
{
    // Context with GL interopability

    #ifdef Q_OS_LINUX
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) main_device->getPlatformId(),
        0};

    #elif defined Q_OS_WIN
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) main_device->getPlatformId(),
        0};
    #endif

    context = clCreateContext(properties, devices.size(), devices.data(), NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void OpenCLContext::initCommandQueue()
{
    // Command queue
    queue = clCreateCommandQueue(context, main_device->getDeviceId(), 0, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}


