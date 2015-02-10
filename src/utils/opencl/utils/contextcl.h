#ifndef CONTEXTCL_H
#define CONTEXTCL_H

/*
 * This class initializes an OpenCL context.
 * */

#include <QLibrary>
#include <CL/opencl.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QDebug>

#include "../../math/qxmathlib.h"

const char * cl_error_cstring(cl_int err);

class OpenCLContext
{
public:
    OpenCLContext();
    void initDevices();
    void initSharedContext();
    void initNormalContext();
    void initCommandQueue();
    void initResources();
    cl_command_queue queue();
    cl_context context();

    cl_program createProgram(QStringList paths, cl_int * err);
    void buildProgram(cl_program * program, const char * options);
    
    cl_kernel cl_rect_copy_float; 
    cl_kernel cl_parallel_reduction;

    QString cl_easy_context_info(cl_context context);
    QString cl_easy_device_info(cl_device_id device);
    QString cl_easy_platform_info(cl_platform_id platform);
    
private:
//    void initializeGL();
    
    cl_platform_id platform[64];
    cl_device_id device[64];

    cl_program program;
    
    cl_command_queue p_queue;
    cl_context p_context;
    cl_int err;

    typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformIDs)(  	cl_uint num_entries,
                                                            cl_platform_id *platforms,
                                                            cl_uint *num_platforms);

    typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceIDs)(        cl_platform_id platform,
                                                            cl_device_type device_type,
                                                            cl_uint num_entries,
                                                            cl_device_id *device,
                                                            cl_uint *num_devices);

    typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformInfo)( 	cl_platform_id platform,
                                                            cl_platform_info param_name,
                                                            size_t param_value_size,
                                                            void *param_value,
                                                            size_t *param_value_size_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceInfo)(       cl_device_id device,
                                                            cl_device_info param_name,
                                                            size_t param_value_size,
                                                            void *param_value,
                                                            size_t *param_value_size_ret);

    typedef cl_program (*PROTOTYPE_QOpenCLCreateProgramWithSource)( 	cl_context context,
                                                                    cl_uint count,
                                                                    const char **strings,
                                                                    const size_t *lengths,
                                                                    cl_int *errcode_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLGetProgramBuildInfo)( 	cl_program  program,
                                                                cl_device_id  device,
                                                                cl_program_build_info  param_name,
                                                                size_t  param_value_size,
                                                                void  *param_value,
                                                                size_t  *param_value_size_ret);

    typedef cl_context (*PROTOTYPE_QOpenCLCreateContext)( 	cl_context_properties *properties,
                                                        cl_uint num_devices,
                                                        const cl_device_id *devices,
                                                        void *pfn_notify (
                                                        const char *errinfo,
                                                        const void *private_info,
                                                        size_t cb,
                                                        void *user_data),
                                                        void *user_data,
                                                        cl_int *errcode_ret);

    typedef cl_command_queue (*PROTOTYPE_QOpenCLCreateCommandQueue)( 	cl_context context,
                                                    cl_device_id device,
                                                    cl_command_queue_properties properties,
                                                    cl_int *errcode_ret);

    typedef cl_kernel (*PROTOTYPE_QOpenCLCreateKernel) ( 	cl_program  program,
                                                        const char *kernel_name,
                                                        cl_int *errcode_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLBuildProgram) ( 	cl_program program,
                                                        cl_uint num_devices,
                                                        const cl_device_id *device_list,
                                                        const char *options,
                                                        void (*pfn_notify)(cl_program, void *user_data),
                                                        void *user_data);

    typedef cl_int (*PROTOTYPE_QOpenCLGetContextInfo)( 	cl_context context,
                                                        cl_context_info param_name,
                                                        size_t param_value_size,
                                                        void *param_value,
                                                        size_t * param_value_size_ret);

    //    typedef cl_int (*PROTOTYPE_QOpenCL)();

    //    typedef cl_int (*PROTOTYPE_QOpenCL)();


    PROTOTYPE_QOpenCLGetProgramBuildInfo QOpenCLGetProgramBuildInfo;
    PROTOTYPE_QOpenCLCreateContext QOpenCLCreateContext;
    PROTOTYPE_QOpenCLCreateCommandQueue QOpenCLCreateCommandQueue;

    PROTOTYPE_QOpenCLBuildProgram QOpenCLBuildProgram;
    PROTOTYPE_QOpenCLCreateKernel QOpenCLCreateKernel;

    PROTOTYPE_QOpenCLGetContextInfo QOpenCLGetContextInfo;
    //    PROTOTYPE_QOpenCL QOpenCL;
    //    PROTOTYPE_QOpenCL QOpenCL;
    PROTOTYPE_QOpenCLCreateProgramWithSource QOpenCLCreateProgramWithSource;
    PROTOTYPE_QOpenCLGetPlatformIDs QOpenCLGetPlatformIDs;
    PROTOTYPE_QOpenCLGetDeviceIDs QOpenCLGetDeviceIDs;
    PROTOTYPE_QOpenCLGetPlatformInfo QOpenCLGetPlatformInfo;
    PROTOTYPE_QOpenCLGetDeviceInfo QOpenCLGetDeviceInfo;
};

#endif // CONTEXTCL_H
