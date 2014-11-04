#ifndef WORKER_H
#define WORKER_H

//#include <QScriptEngine>
#include <QPlainTextEdit>

/* Project files */
#include "../../lib/qxlib/qxlib.h"

class BaseWorker : public QObject
{
    Q_OBJECT

    public:
        BaseWorker();
        ~BaseWorker();

        void setFilePaths(QStringList * file_paths);
//        void setQSpaceInfo(float * suggested_search_radius_low, float * suggested_search_radius_high, float * suggested_q);
        void setFiles(QList<DetectorFile> * files);
        void setReducedPixels(Matrix<float> * reduced_pixels);
        void setOpenCLContext(OpenCLContext * context);
        void setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl);
        void setSVOFile(SparseVoxelOcttree * svo);

    signals:
        void finished();
        void abort();
        void changedMessageString(QString str);
        void changedGenericProgress(int value);
        void changedMemoryUsage(int value);
        void changedFormatGenericProgress(QString str);
        void changedFormatMemoryUsage(QString str);
        void changedRangeMemoryUsage(int min, int max);
        void changedRangeGenericProcess(int min, int max);
        void changedTabWidget(int value);
        void changedFile(QString path);
        void popup(QString title, QString text);
        void qSpaceInfoChanged(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q);

    public slots:
        void killProcess();
        void setActiveAngle(int value);
        void setOffsetOmega(double value);
        void setOffsetKappa(double value);
        void setOffsetPhi(double value);
        void setNoiseLow(double value);
        void setNoiseHigh(double value);
        void setThldProjectLow(double value);
        void setThldProjectHigh(double value);
        void setQSpaceInfo(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q);

    protected:
        // Runtime
        bool kill_flag;
        
        // OpenCL
        OpenCLContext * context_cl;
        cl_mem * alpha_img_clgl;
        cl_mem * beta_img_clgl;
        cl_mem * gamma_img_clgl;
        cl_mem * tsf_img_clgl;
        cl_int err;
        cl_program program;
        bool isCLInitialized;

        // Voxelize
        SparseVoxelOcttree * svo;
        float  suggested_search_radius_low;
        float  suggested_search_radius_high;
        float  suggested_q;

        // File treatment
        int active_angle;
        float thld_noise_low;
        float thld_noise_high;
        float thld_project_low;
        float thld_project_high;
        QStringList * file_paths;
        QList<DetectorFile> * files;
        Matrix<float> * reduced_pixels;
        
        double offset_omega;
        double offset_kappa;
        double offset_phi;
        
        size_t GLOBAL_VRAM_ALLOC_MAX;

        // Resolved OpenCL functions
        void resolveOpenCLFunctions();

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

        typedef cl_int (*PROTOTYPE_QOpenCLSetKernelArg) ( 	cl_kernel kernel,
                                                            cl_uint arg_index,
                                                            size_t arg_size,
                                                            const void *arg_value);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueNDRangeKernel)( 	cl_command_queue command_queue,
                                                                    cl_kernel kernel,
                                                                    cl_uint work_dim,
                                                                    const size_t *global_work_offset,
                                                                    const size_t *global_work_size,
                                                                    const size_t *local_work_size,
                                                                    cl_uint num_events_in_wait_list,
                                                                    const cl_event *event_wait_list,
                                                                    cl_event *event);

        typedef cl_int (*PROTOTYPE_QOpenCLFinish)( 	cl_command_queue command_queue);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueAcquireGLObjects)( 	cl_command_queue command_queue,
                                                                        cl_uint num_objects,
                                                                        const cl_mem *mem_objects,
                                                                        cl_uint num_events_in_wait_list,
                                                                        const cl_event *event_wait_list,
                                                                        cl_event *event);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReleaseGLObjects)( 	cl_command_queue command_queue,
                                                                        cl_uint num_objects,
                                                                        const cl_mem *mem_objects,
                                                                        cl_uint num_events_in_wait_list,
                                                                        const cl_event *event_wait_list,
                                                                        cl_event *event);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReadBuffer)( 	cl_command_queue command_queue,
                                                                cl_mem buffer,
                                                                cl_bool blocking_read,
                                                                size_t offset,
                                                                size_t cb,
                                                                void *ptr,
                                                                cl_uint num_events_in_wait_list,
                                                                const cl_event *event_wait_list,
                                                                cl_event *event);

        typedef cl_mem (*PROTOTYPE_QOpenCLCreateBuffer) ( 	cl_context context,
                                                            cl_mem_flags flags,
                                                            size_t size,
                                                            void *host_ptr,
                                                            cl_int *errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLReleaseMemObject) ( 	cl_mem memobj);

        typedef cl_mem (*PROTOTYPE_QOpenCLCreateFromGLTexture2D) ( 	cl_context context,
                                                                    cl_mem_flags flags,
                                                                    GLenum texture_target,
                                                                    GLint miplevel,
                                                                    GLuint texture,
                                                                    cl_int *errcode_ret);

        typedef cl_sampler (*PROTOTYPE_QOpenCLCreateSampler)( 	cl_context context,
                                                            cl_bool normalized_coords,
                                                            cl_addressing_mode addressing_mode,
                                                            cl_filter_mode filter_mode,
                                                            cl_int *errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueWriteBuffer) ( 	cl_command_queue command_queue,
                                                                    cl_mem buffer,
                                                                    cl_bool blocking_write,
                                                                    size_t offset,
                                                                    size_t cb,
                                                                    const void *ptr,
                                                                    cl_uint num_events_in_wait_list,
                                                                    const cl_event *event_wait_list,
                                                                    cl_event *event);

        typedef cl_kernel (*PROTOTYPE_QOpenCLCreateKernel) ( 	cl_program  program,
                                                            const char *kernel_name,
                                                            cl_int *errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLReleaseSampler) ( 	cl_sampler sampler);

        typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReadImage)( 	cl_command_queue command_queue,
                                                                cl_mem image,
                                                                cl_bool blocking_read,
                                                                const size_t origin[3],
                                                                const size_t region[3],
                                                                size_t row_pitch,
                                                                size_t slice_pitch,
                                                                void *ptr,
                                                                cl_uint num_events_in_wait_list,
                                                                const cl_event *event_wait_list,
                                                                cl_event *event);

        typedef cl_mem (*PROTOTYPE_QOpenCLCreateImage2D)( 	cl_context context,
                                                            cl_mem_flags flags,
                                                            const cl_image_format *image_format,
                                                            size_t image_width,
                                                            size_t image_height,
                                                            size_t image_row_pitch,
                                                            void *host_ptr,
                                                            cl_int *errcode_ret);


        typedef cl_mem (*PROTOTYPE_QOpenCLCreateImage3D) ( 	cl_context context,
                                                            cl_mem_flags flags,
                                                            const cl_image_format *image_format,
                                                            size_t image_width,
                                                            size_t image_height,
                                                            size_t image_depth,
                                                            size_t image_row_pitch,
                                                            size_t image_slice_pitch,
                                                            void *host_ptr,
                                                            cl_int *errcode_ret);

        typedef cl_int (*PROTOTYPE_QOpenCLReleaseKernel)  ( 	cl_kernel kernel);

        PROTOTYPE_QOpenCLReleaseKernel QOpenCLReleaseKernel;
        PROTOTYPE_QOpenCLCreateImage2D QOpenCLCreateImage2D;
        PROTOTYPE_QOpenCLReleaseSampler QOpenCLReleaseSampler;
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
        PROTOTYPE_QOpenCLCreateKernel QOpenCLCreateKernel;
        PROTOTYPE_QOpenCLGetProgramBuildInfo QOpenCLGetProgramBuildInfo;
        PROTOTYPE_QOpenCLCreateContext QOpenCLCreateContext;
        PROTOTYPE_QOpenCLCreateCommandQueue QOpenCLCreateCommandQueue;
        PROTOTYPE_QOpenCLCreateProgramWithSource QOpenCLCreateProgramWithSource;
        PROTOTYPE_QOpenCLGetPlatformIDs QOpenCLGetPlatformIDs;
        PROTOTYPE_QOpenCLGetDeviceIDs QOpenCLGetDeviceIDs;
        PROTOTYPE_QOpenCLGetPlatformInfo QOpenCLGetPlatformInfo;
        PROTOTYPE_QOpenCLGetDeviceInfo QOpenCLGetDeviceInfo;
};

class SetFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        SetFileWorker();
        ~SetFileWorker();

    private slots:
        void process();
};


class ReadFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ReadFileWorker();
        ~ReadFileWorker();

    private slots:
        void process();
};


class ProjectFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ProjectFileWorker();
        ~ProjectFileWorker();
        int projectFile(DetectorFile * file);
        
    signals:
        void testToWindow();
        void testToMain();

    public slots:
        void initializeCLKernel();
        void setSelection(Selection area); 

    private slots:
        void process();

    protected:
        // Related to OpenCL
        cl_kernel project_kernel;
        size_t n_reduced_pixels;
        Selection selection;
};


class VoxelizeWorker : public BaseWorker
{
    Q_OBJECT

    public:
        VoxelizeWorker();
        ~VoxelizeWorker();

    public slots:
        void process();
        void initializeCLKernel();

    protected:
        cl_kernel voxelize_kernel;
        cl_kernel fill_kernel;
        
        unsigned int getOctIndex(unsigned int msdFlag, unsigned int dataFlag, unsigned int child);
        unsigned int getOctBrick(unsigned int poolX, unsigned int poolY, unsigned int poolZ);
};

class MultiWorker : public ProjectFileWorker
{
    Q_OBJECT

    public:
        MultiWorker();
        ~MultiWorker();

    public slots:
        void process();
};

#endif
