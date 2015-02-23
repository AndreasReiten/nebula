#include "contextcl.h"

#include <QtGlobal>
#include <QList>
//#include <QOpenGLContext>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QByteArray>

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include <CL/cl_gl.h>

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined Q_OS_LINUX
    #include <GL/glx.h>
#endif

OpenCLFunctions::OpenCLFunctions()
{
    
}

OpenCLFunctions::~OpenCLFunctions()
{
    
}

void OpenCLFunctions::initializeOpenCLFunctions()
{
    QLibrary myLib("OpenCL");
    
    QOpenCLGetPlatformIDs = (PROTOTYPE_QOpenCLGetPlatformIDs) myLib.resolve("clGetPlatformIDs");
    if (!QOpenCLGetPlatformIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceIDs = (PROTOTYPE_QOpenCLGetDeviceIDs) myLib.resolve("clGetDeviceIDs");
    if (!QOpenCLGetDeviceIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetPlatformInfo = (PROTOTYPE_QOpenCLGetPlatformInfo) myLib.resolve("clGetPlatformInfo");
    if (!QOpenCLGetPlatformInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceInfo = (PROTOTYPE_QOpenCLGetDeviceInfo) myLib.resolve("clGetDeviceInfo");
    if (!QOpenCLGetDeviceInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateProgramWithSource = (PROTOTYPE_QOpenCLCreateProgramWithSource) myLib.resolve("clCreateProgramWithSource");
    if (!QOpenCLCreateProgramWithSource) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetProgramBuildInfo = (PROTOTYPE_QOpenCLGetProgramBuildInfo) myLib.resolve("clGetProgramBuildInfo");
    if (!QOpenCLGetProgramBuildInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateContext = (PROTOTYPE_QOpenCLCreateContext) myLib.resolve("clCreateContext");
    if (!QOpenCLCreateContext) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateCommandQueue = (PROTOTYPE_QOpenCLCreateCommandQueue) myLib.resolve("clCreateCommandQueue");
    if (!QOpenCLCreateCommandQueue) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateKernel = (PROTOTYPE_QOpenCLCreateKernel) myLib.resolve("clCreateKernel");
    if (!QOpenCLCreateKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLBuildProgram = (PROTOTYPE_QOpenCLBuildProgram) myLib.resolve("clBuildProgram");
    if (!QOpenCLBuildProgram) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetContextInfo = (PROTOTYPE_QOpenCLGetContextInfo) myLib.resolve("clGetContextInfo");
    if (!QOpenCLGetContextInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetPlatformIDs = (PROTOTYPE_QOpenCLGetPlatformIDs) myLib.resolve("clGetPlatformIDs");
    if (!QOpenCLGetPlatformIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceIDs = (PROTOTYPE_QOpenCLGetDeviceIDs) myLib.resolve("clGetDeviceIDs");
    if (!QOpenCLGetDeviceIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetPlatformInfo = (PROTOTYPE_QOpenCLGetPlatformInfo) myLib.resolve("clGetPlatformInfo");
    if (!QOpenCLGetPlatformInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceInfo = (PROTOTYPE_QOpenCLGetDeviceInfo) myLib.resolve("clGetDeviceInfo");
    if (!QOpenCLGetDeviceInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateProgramWithSource = (PROTOTYPE_QOpenCLCreateProgramWithSource) myLib.resolve("clCreateProgramWithSource");
    if (!QOpenCLCreateProgramWithSource) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetProgramBuildInfo = (PROTOTYPE_QOpenCLGetProgramBuildInfo) myLib.resolve("clGetProgramBuildInfo");
    if (!QOpenCLGetProgramBuildInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateContext = (PROTOTYPE_QOpenCLCreateContext) myLib.resolve("clCreateContext");
    if (!QOpenCLCreateContext) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateCommandQueue = (PROTOTYPE_QOpenCLCreateCommandQueue) myLib.resolve("clCreateCommandQueue");
    if (!QOpenCLCreateCommandQueue) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLSetKernelArg = (PROTOTYPE_QOpenCLSetKernelArg) myLib.resolve("clSetKernelArg");
    if (!QOpenCLSetKernelArg) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueNDRangeKernel = (PROTOTYPE_QOpenCLEnqueueNDRangeKernel) myLib.resolve("clEnqueueNDRangeKernel");
    if (!QOpenCLEnqueueNDRangeKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLFinish= (PROTOTYPE_QOpenCLFinish) myLib.resolve("clFinish");
    if (!QOpenCLFinish) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReleaseGLObjects = (PROTOTYPE_QOpenCLEnqueueReleaseGLObjects) myLib.resolve("clEnqueueReleaseGLObjects");
    if (!QOpenCLEnqueueReleaseGLObjects) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueAcquireGLObjects = (PROTOTYPE_QOpenCLEnqueueAcquireGLObjects) myLib.resolve("clEnqueueAcquireGLObjects");
    if (!QOpenCLEnqueueAcquireGLObjects) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateKernel = (PROTOTYPE_QOpenCLCreateKernel) myLib.resolve("clCreateKernel");
    if (!QOpenCLCreateKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadBuffer = (PROTOTYPE_QOpenCLEnqueueReadBuffer) myLib.resolve("clEnqueueReadBuffer");
    if (!QOpenCLEnqueueReadBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateBuffer = (PROTOTYPE_QOpenCLCreateBuffer) myLib.resolve("clCreateBuffer");
    if (!QOpenCLCreateBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseMemObject = (PROTOTYPE_QOpenCLReleaseMemObject) myLib.resolve("clReleaseMemObject");
    if (!QOpenCLReleaseMemObject) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateFromGLTexture2D = (PROTOTYPE_QOpenCLCreateFromGLTexture2D) myLib.resolve("clCreateFromGLTexture2D");
    if (!QOpenCLCreateFromGLTexture2D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateSampler = (PROTOTYPE_QOpenCLCreateSampler) myLib.resolve("clCreateSampler");
    if (!QOpenCLCreateSampler) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueWriteBuffer = (PROTOTYPE_QOpenCLEnqueueWriteBuffer) myLib.resolve("clEnqueueWriteBuffer");
    if (!QOpenCLEnqueueWriteBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseKernel = (PROTOTYPE_QOpenCLReleaseKernel) myLib.resolve("clReleaseKernel");
    if (!QOpenCLReleaseKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateImage2D = (PROTOTYPE_QOpenCLCreateImage2D) myLib.resolve("clCreateImage2D");
    if (!QOpenCLCreateImage2D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateImage3D = (PROTOTYPE_QOpenCLCreateImage3D) myLib.resolve("clCreateImage3D");
    if (!QOpenCLCreateImage3D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadImage = (PROTOTYPE_QOpenCLEnqueueReadImage) myLib.resolve("clEnqueueReadImage");
    if (!QOpenCLEnqueueReadImage) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseSampler = (PROTOTYPE_QOpenCLReleaseSampler) myLib.resolve("clReleaseSampler");
    if (!QOpenCLReleaseSampler) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueCopyBufferToImage  = (PROTOTYPE_QOpenCLEnqueueCopyBufferToImage ) myLib.resolve("clEnqueueCopyBufferToImage");
    if (!QOpenCLEnqueueCopyBufferToImage ) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadBufferRect  = (PROTOTYPE_QOpenCLEnqueueReadBufferRect ) myLib.resolve("clEnqueueReadBufferRect");
    if (!QOpenCLEnqueueReadBufferRect ) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());
}

OpenCLContext::OpenCLContext()
{
    initializeOpenCLFunctions();
}

OpenCLContext::~OpenCLContext()
{
    // Todo: Release resources bound to object
}


cl_command_queue OpenCLContext::queue()
{
    return p_queue;
}

cl_context OpenCLContext::context()
{
    return p_context;
}

cl_program OpenCLContext::createProgram(QStringList paths, cl_int * err)
{
    // Create program object
    Matrix<size_t> lengths(1, paths.size());
    Matrix<const char *> sources(1, paths.size());

    Matrix<QByteArray> blobs(1,paths.size());
    
    for (size_t i = 0; i < paths.size(); i++)
    {
        QFile file(paths[i]);
        if (!file.open(QIODevice::ReadOnly)) qDebug(QString(QString("Could not open file: ")+paths[i]).toStdString().c_str());
        blobs[i] = file.readAll();
        
        sources[i] = blobs[i].data();
        lengths[i] = blobs[i].length();
    }
    return QOpenCLCreateProgramWithSource(p_context, paths.size(), sources.data(), lengths.data(), err);
}

void OpenCLContext::buildProgram(cl_program * program, const char * options)
{
    // Build source
    err = QOpenCLBuildProgram(*program, 1, device, options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // Compile log
        qDebug() << "Error compiling kernel: "+QString(cl_error_cstring(err));
        std::stringstream ss;

        char* build_log;
        size_t log_size;

        QOpenCLGetProgramBuildInfo(*program, device[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        build_log = new char[log_size+1];

        QOpenCLGetProgramBuildInfo(*program, device[0], CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
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
    // Get platforms
    cl_uint num_platform_entries = 64;
    cl_uint num_platforms;

    err = QOpenCLGetPlatformIDs(num_platform_entries, platform, &num_platforms);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    if (num_platforms > 0)
    {
        for (size_t i = 0; i < num_platforms; i++)
        {
            char platform_name[128];

            err = QOpenCLGetPlatformInfo(platform[i], CL_PLATFORM_NAME, sizeof(char)*128, platform_name, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }
    else if (num_platforms == 0)
    {
        qDebug() << "No OpenCL platforms were found.";
    }

    cl_uint num_device_entries = 64;
    cl_uint num_devices;

    err = QOpenCLGetDeviceIDs( platform[0], CL_DEVICE_TYPE_ALL, num_device_entries, device, &num_devices);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    if (num_devices > 0)
    {
        for (size_t i = 0; i < num_devices; i++)
        {
            char device_name[128];

            err = QOpenCLGetDeviceInfo(device[i], CL_DEVICE_NAME, sizeof(char)*128, device_name, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }
    else if (num_devices == 0)
    {
        char platform_name[128];

        err = QOpenCLGetPlatformInfo(platform[0], CL_PLATFORM_NAME, sizeof(char)*128, platform_name, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        qDebug() << "No OpenCL devices were found on" << platform_name;
    }

}

void OpenCLContext::initSharedContext()
{
    // Context with GL interopability
    #ifdef Q_OS_LINUX
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) platform[0],
        0};

    #elif defined Q_OS_WIN
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) platform[0],
        0};
    #endif

    cl_uint num = 1;

    p_context = QOpenCLCreateContext(properties, num, device, NULL, NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void OpenCLContext::initNormalContext()
{
    // Context without GL interopability
    cl_uint num = 1;

    p_context = QOpenCLCreateContext(NULL, num, device, NULL, NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void OpenCLContext::initCommandQueue()
{
    // Command queue
    p_queue = QOpenCLCreateCommandQueue(p_context, device[0], 0, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void OpenCLContext::initResources()
{
    // Build program from OpenCL kernel source
    QStringList paths;
    paths << "kernels/mem_functions.cl";
    paths << "kernels/parallel_reduce.cl";

    program = createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    buildProgram(&program, "-Werror");

    // Kernel handles
    cl_rect_copy_float = QOpenCLCreateKernel(program, "rectCopyFloat", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    cl_parallel_reduction = QOpenCLCreateKernel(program, "psum", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}


QString OpenCLContext::cl_easy_context_info(cl_context context)
{
    QString str;

    cl_int err;

    cl_uint reference_count;

    cl_device_id device[64];
    size_t size_devices;

    cl_context_properties property[64];
    size_t size_properties;

    str = "\n*** OpenCL context info ***\n";

    err = QOpenCLGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(cl_int), &reference_count, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    str += "  Reference count: " + QString::number(reference_count) + "\n";

    err = QOpenCLGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(cl_device_id)*64, device, &size_devices);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    str += "  Devices in this context:\n";
    for (size_t i = 0; i < size_devices/sizeof(cl_device_id); i++)
    {
        str += "  " + QString::number(i) + ":\t" + cl_easy_device_info(device[i]) + "\n";
    }

    err = QOpenCLGetContextInfo(context, CL_CONTEXT_PROPERTIES, sizeof(cl_context_properties)*64, property, &size_properties);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    return str;
}

QString OpenCLContext::cl_easy_device_info(cl_device_id device)
{
    QString str;

    char device_name[128];

    cl_int err = QOpenCLGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(char)*128, device_name, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    str += device_name;

    return str;
}

QString OpenCLContext::cl_easy_platform_info(cl_platform_id platform)
{
    QString str;

    char platform_name[128];

    cl_int err = QOpenCLGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(char)*128, platform_name, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    str += platform_name;

    return str;
}

const char * cl_error_cstring(cl_int err)
{
    switch (err) {
        case CL_SUCCESS:                            return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND:                   return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:               return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE:             return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES:                   return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY:                 return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:       return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP:                   return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH:              return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:              return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_MAP_FAILURE:                        return "CL_MAP_FAILURE";
        case CL_INVALID_VALUE:                      return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE:                return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM:                   return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE:                     return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT:                    return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES:           return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE:              return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR:                   return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT:                 return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE:                 return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER:                    return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY:                     return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS:              return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM:                    return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:         return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME:                return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:          return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL:                     return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX:                  return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE:                  return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE:                   return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS:                return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION:             return "CL_INVALID_WORK_DIMENSION:";
        case CL_INVALID_WORK_GROUP_SIZE:            return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE:             return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET:              return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST:            return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT:                      return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION:                  return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT:                  return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE:                return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL:                  return "CL_INVALID_MIP_LEVEL";
        default: return "Unknown";
    }
}
