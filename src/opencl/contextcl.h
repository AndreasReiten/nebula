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

#include "../math/matrix.h"

const char * cl_error_cstring(cl_int err);


class OpenCLFunctions
{
    public:
        OpenCLFunctions();
        ~OpenCLFunctions();

    protected:
        void initializeOpenCLFunctions();

        typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformIDs)(  	cl_uint num_entries,
                cl_platform_id * platforms,
                cl_uint * num_platforms);

        typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceIDs)(        cl_platform_id platform,
                cl_device_type device_type,
                cl_uint num_entries,
                cl_device_id * device,
                cl_uint * num_devices);

        typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformInfo)( 	cl_platform_id platform,
                cl_platform_info param_name,
                size_t param_value_size,
                void * param_value,
                size_t * param_value_size_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceInfo)(       cl_device_id device,
                cl_device_info param_name,
                size_t param_value_size,
                void * param_value,
                size_t * param_value_size_ret);

        typedef cl_program (*PROTOTYPE_QOpenCLCreateProgramWithSource)( 	cl_context context,
                cl_uint count,
                const char ** strings,
                const size_t * lengths,
                cl_int * errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLGetProgramBuildInfo)( 	cl_program  program,
                cl_device_id  device,
                cl_program_build_info  param_name,
                size_t  param_value_size,
                void * param_value,
                size_t * param_value_size_ret);

        typedef cl_context (*PROTOTYPE_QOpenCLCreateContext)( 	cl_context_properties * properties,
                cl_uint num_devices,
                const cl_device_id * devices,
                void * pfn_notify (
                    const char * errinfo,
                    const void * private_info,
                    size_t cb,
                    void * user_data),
                void * user_data,
                cl_int * errcode_ret);

        typedef cl_command_queue (*PROTOTYPE_QOpenCLCreateCommandQueue)( 	cl_context context,
                cl_device_id device,
                cl_command_queue_properties properties,
                cl_int * errcode_ret);

        typedef cl_kernel (*PROTOTYPE_QOpenCLCreateKernel) ( 	cl_program  program,
                const char * kernel_name,
                cl_int * errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLBuildProgram) ( 	cl_program program,
                cl_uint num_devices,
                const cl_device_id * device_list,
                const char * options,
                void (*pfn_notify)(cl_program, void * user_data),
                void * user_data);

        typedef cl_int (*PROTOTYPE_QOpenCLGetContextInfo)( 	cl_context context,
                cl_context_info param_name,
                size_t param_value_size,
                void * param_value,
                size_t * param_value_size_ret);




        typedef cl_int (*PROTOTYPE_QOpenCLSetKernelArg) ( 	cl_kernel kernel,
                cl_uint arg_index,
                size_t arg_size,
                const void * arg_value);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueNDRangeKernel)( 	cl_command_queue command_queue,
                cl_kernel kernel,
                cl_uint work_dim,
                const size_t * global_work_offset,
                const size_t * global_work_size,
                const size_t * local_work_size,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_int (*PROTOTYPE_QOpenCLFinish)( 	cl_command_queue command_queue);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueAcquireGLObjects)( 	cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem * mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReleaseGLObjects)( 	cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem * mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReadBuffer)( 	cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_read,
                size_t offset,
                size_t cb,
                void * ptr,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_mem (*PROTOTYPE_QOpenCLCreateBuffer) ( 	cl_context context,
                cl_mem_flags flags,
                size_t size,
                void * host_ptr,
                cl_int * errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLReleaseMemObject) ( 	cl_mem memobj);

        typedef cl_mem (*PROTOTYPE_QOpenCLCreateFromGLTexture2D) ( 	cl_context context,
                cl_mem_flags flags,
                GLenum texture_target,
                GLint miplevel,
                GLuint texture,
                cl_int * errcode_ret);

        typedef cl_sampler (*PROTOTYPE_QOpenCLCreateSampler)( 	cl_context context,
                cl_bool normalized_coords,
                cl_addressing_mode addressing_mode,
                cl_filter_mode filter_mode,
                cl_int * errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueWriteBuffer) ( 	cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_write,
                size_t offset,
                size_t cb,
                const void * ptr,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_int (*PROTOTYPE_QOpenCLReleaseSampler) ( 	cl_sampler sampler);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReadImage)( 	cl_command_queue command_queue,
                cl_mem image,
                cl_bool blocking_read,
                const size_t origin[3],
                const size_t region[3],
                size_t row_pitch,
                size_t slice_pitch,
                void * ptr,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_mem (*PROTOTYPE_QOpenCLCreateImage2D)( 	cl_context context,
                cl_mem_flags flags,
                const cl_image_format * image_format,
                size_t image_width,
                size_t image_height,
                size_t image_row_pitch,
                void * host_ptr,
                cl_int * errcode_ret);


        typedef cl_mem (*PROTOTYPE_QOpenCLCreateImage3D) ( 	cl_context context,
                cl_mem_flags flags,
                const cl_image_format * image_format,
                size_t image_width,
                size_t image_height,
                size_t image_depth,
                size_t image_row_pitch,
                size_t image_slice_pitch,
                void * host_ptr,
                cl_int * errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueCopyBufferToImage) ( 	cl_command_queue command_queue,
                cl_mem src_buffer,
                cl_mem  dst_image,
                size_t src_offset,
                const size_t dst_origin[3],
                const size_t region[3],
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReadBufferRect) ( 	 	cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_read,
                const size_t buffer_origin[3],
                const size_t host_origin[3],
                const size_t region[3],
                size_t buffer_row_pitch,
                size_t buffer_slice_pitch,
                size_t host_row_pitch,
                size_t host_slice_pitch,
                void * ptr,
                cl_uint num_events_in_wait_list,
                const cl_event * event_wait_list,
                cl_event * event);

        typedef cl_int (*PROTOTYPE_QOpenCLReleaseKernel)  ( 	cl_kernel kernel);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueCopyBufferRect) ( 	cl_command_queue command_queue,
                                                    cl_mem src_buffer,
                                                    cl_mem dst_buffer,
                                                    const size_t src_origin[3],
                                                    const size_t dst_origin[3],
                                                    const size_t region[3],
                                                    size_t src_row_pitch,
                                                    size_t src_slice_pitch,
                                                    size_t dst_row_pitch,
                                                    size_t dst_slice_pitch,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event *event_wait_list,
                                                    cl_event *event);

        PROTOTYPE_QOpenCLGetProgramBuildInfo QOpenCLGetProgramBuildInfo;
        PROTOTYPE_QOpenCLCreateContext QOpenCLCreateContext;
        PROTOTYPE_QOpenCLCreateCommandQueue QOpenCLCreateCommandQueue;
        PROTOTYPE_QOpenCLBuildProgram QOpenCLBuildProgram;
        PROTOTYPE_QOpenCLCreateKernel QOpenCLCreateKernel;
        PROTOTYPE_QOpenCLGetContextInfo QOpenCLGetContextInfo;
        PROTOTYPE_QOpenCLCreateProgramWithSource QOpenCLCreateProgramWithSource;
        PROTOTYPE_QOpenCLGetPlatformIDs QOpenCLGetPlatformIDs;
        PROTOTYPE_QOpenCLGetDeviceIDs QOpenCLGetDeviceIDs;
        PROTOTYPE_QOpenCLGetPlatformInfo QOpenCLGetPlatformInfo;
        PROTOTYPE_QOpenCLGetDeviceInfo QOpenCLGetDeviceInfo;
        PROTOTYPE_QOpenCLReleaseKernel QOpenCLReleaseKernel;
        PROTOTYPE_QOpenCLCreateImage2D QOpenCLCreateImage2D;
        PROTOTYPE_QOpenCLReleaseSampler QOpenCLReleaseSampler;
        PROTOTYPE_QOpenCLEnqueueCopyBufferToImage QOpenCLEnqueueCopyBufferToImage;
        PROTOTYPE_QOpenCLEnqueueReadImage QOpenCLEnqueueReadImage;
        PROTOTYPE_QOpenCLCreateImage3D QOpenCLCreateImage3D;
        PROTOTYPE_QOpenCLSetKernelArg QOpenCLSetKernelArg;
        PROTOTYPE_QOpenCLEnqueueNDRangeKernel QOpenCLEnqueueNDRangeKernel;
        PROTOTYPE_QOpenCLFinish QOpenCLFinish;
        PROTOTYPE_QOpenCLEnqueueAcquireGLObjects QOpenCLEnqueueAcquireGLObjects;
        PROTOTYPE_QOpenCLEnqueueReleaseGLObjects QOpenCLEnqueueReleaseGLObjects;
        PROTOTYPE_QOpenCLEnqueueReadBuffer QOpenCLEnqueueReadBuffer;
        PROTOTYPE_QOpenCLCreateBuffer QOpenCLCreateBuffer;
        PROTOTYPE_QOpenCLReleaseMemObject QOpenCLReleaseMemObject;
        PROTOTYPE_QOpenCLCreateFromGLTexture2D QOpenCLCreateFromGLTexture2D;
        PROTOTYPE_QOpenCLCreateSampler QOpenCLCreateSampler;
        PROTOTYPE_QOpenCLEnqueueWriteBuffer QOpenCLEnqueueWriteBuffer;
        PROTOTYPE_QOpenCLEnqueueReadBufferRect QOpenCLEnqueueReadBufferRect;
        PROTOTYPE_QOpenCLEnqueueCopyBufferRect QOpenCLEnqueueCopyBufferRect;
};

class OpenCLContext : protected OpenCLFunctions
{
    public:
        OpenCLContext();
        ~OpenCLContext();
        void initDevices();
        void initSharedContext();
        void initNormalContext();
        void initCommandQueue();
        cl_command_queue queue();
        cl_context context();

        cl_program createProgram(QStringList paths, cl_int * err);
        void buildProgram(cl_program program, QString options);

        QString cl_easy_context_info(cl_context context);
        QString cl_easy_device_info(cl_device_id device);
        QString cl_easy_platform_info(cl_platform_id platform);

    private:
        cl_platform_id platform[64];
        cl_device_id device[64];

        cl_program program;

        cl_command_queue p_queue;
        cl_context p_context;
        cl_int err;
};

#endif // CONTEXTCL_H
