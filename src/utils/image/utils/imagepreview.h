#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

#include <QLibrary>
#include <CL/opencl.h>
#include <QOpenGLWidget>
#include <QOpenGLPaintDevice>
#include <QOpenGLFunctions>

#include "../../opengl/qxopengllib.h"
#include "../../file/qxfilelib.h"
#include "../../math/qxmathlib.h"

class ImagePreviewWorker : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    void setOpenCLContext(OpenCLContext * context);
    int projectFile(DetectorFile * file, Selection selection, Matrix<float> *samples, size_t *n_samples);
    void setReducedPixels(Matrix<float> * reduced_pixels);
    
    explicit ImagePreviewWorker(QObject *parent = 0);
    ~ImagePreviewWorker();
//    void setSharedWindow(SharedContextWindow * window);
    SeriesSet set();
    
signals:
    void changedMessageString(QString str);
//    void changedGenericProgress(int value);
    void changedMemoryUsage(int value);
//    void changedFormatGenericProgress(QString str);
    void changedFormatMemoryUsage(QString str);
    void changedRangeMemoryUsage(int min, int max);
//    void changedRangeGenericProcess(int min, int max);
    void popup(QString title, QString text);
    void qSpaceInfoChanged(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q);
    
    void resultFinished(QString str);
    void selectionAlphaChanged(bool value);
    void selectionBetaChanged(bool value);
    void noiseLowChanged(double value);
    void pathRemoved(QString path);
    void pathChanged(QString path);
    void imageRangeChanged(int low, int high);
    void currentIndexChanged(int value);
    void progressChanged(int value);
    void progressRangeChanged(int min, int max);
    void visibilityChanged(bool value);
    void showProgressBar(bool value);
    
public slots:
    void setBeamOverrideActive(bool value);
    void setBeamXOverride(double value);
    void setBeamYOverride(double value);
    
//    void initializeCLKernel();
    void killProcess();
    void setActiveAngle(QString value);
    void setOffsetOmega(double value);
    void setOffsetKappa(double value);
    void setOffsetPhi(double value);
    void reconstruct();
    
    void setMode(int value);
    void setNoise(double value);
//    void setThresholdNoiseHigh(double value);
//    void setThresholdPostCorrectionLow(double value);
//    void setThresholdPostCorrectionHigh(double value);
    void setTsfTexture(int value);
    void setTsfAlpha(int value);
    void setLog(bool value);
    void setCorrectionLorentz(bool value);
    void setCorrectionBackground(bool value);
    void setDataMin(double value);
    void setDataMax(double value);
    void calculus();
    void takeScreenShot(QString path);
    void saveImage(QString path);
    void setFrame();
    void centerImage();
    void analyze(QString str);
    void applyPlaneMarker(QString str);
    void traceSet();
    void showWeightCenter(bool value);
    void setSet(SeriesSet s);
    void setFrameByIndex(int i);
    void nextSeries();
    void prevSeries();
    void removeCurrentImage();
    void applySelection(QString);
    void setCorrectionNoise(bool value);
    void setCorrectionPlane(bool value);
    void setCorrectionClutter(bool value);
    void setCorrectionMedian(bool value);
    void setCorrectionPolarization(bool value);
    void setCorrectionFlux(bool value);
    void setCorrectionExposure(bool value);
    void setLsqSamples(int value);
    void toggleTraceTexture(bool value);
    
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent* event);
//    void resizeEvent(QResizeEvent * ev);
    
    
private:
    QPointF posGLtoQt(QPointF coord);
    QPointF posQttoGL(QPointF coord);

    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
    Matrix<GLfloat> glRect(QRectF &qt_rect);

    GLuint loadShader(GLenum type, const char *source);

    // Shaders
    GLint std_2d_tex_fragpos;
    GLint std_2d_tex_pos;
    GLint std_2d_tex_texture;
    GLint std_2d_tex_transform;
    QOpenGLShaderProgram *std_2d_tex_program;

    GLint rect_hl_2d_tex_fragpos;
    GLint rect_hl_2d_tex_pos;
    GLint rect_hl_2d_tex_texture;
    GLint rect_hl_2d_tex_transform;
    GLint rect_hl_2d_tex_bounds;
    GLint rect_hl_2d_tex_pixel_size;
    QOpenGLShaderProgram *rect_hl_2d_tex_program;

    GLint std_2d_col_color;
    GLint std_2d_col_transform;
    GLint std_2d_col_fragpos;
    QOpenGLShaderProgram *std_2d_col_program;

    GLint std_3d_col_color;
    GLint std_3d_col_transform;
    GLint std_3d_col_fragpos;
    QOpenGLShaderProgram *std_3d_col_program;

    GLint unitcell_color;
    GLint unitcell_transform;
    GLint unitcell_fragpos;
    GLint unitcell_lim_low;
    GLint unitcell_lim_high;
    GLint unitcell_u;
    QOpenGLShaderProgram *unitcell_program;

    GLint std_blend_fragpos;
    GLint std_blend_texpos;
    GLint std_blend_tex_a;
    GLint std_blend_tex_b;
    GLint std_blend_method;
    QOpenGLShaderProgram *std_blend_program;

    QOpenGLPaintDevice * paint_device_gl;
    OpenCLContext *context_cl;

    void paintGL();
    void resizeGL();
    void intitalizeGL();

    Matrix<float> * reduced_pixels;
    double offset_omega;
    double offset_kappa;
    double offset_phi;
    QString active_rotation;
    bool kill_flag;
    
    // Series
    QList<Matrix<float>> set_trace;
    SeriesSet p_set;
    cl_mem series_interpol_gpu_3Dimg;
    void setSeriesMaxFrame();

    // GPU functions
    void imageCalcuclus(cl_mem data_buf_cl, cl_mem out_buf_cl, Matrix<float> &param, Matrix<size_t> &image_size, Matrix<size_t> &local_ws, float mean, float deviation, int task);
    
    void imageCompute(cl_mem data_buf_cl, cl_mem frame_image_cl, cl_mem tsf_image_cl, Matrix<float> &data_limit, Matrix<size_t> &image_size, Matrix<size_t> & local_ws, cl_sampler tsf_sampler, int log);
    
    void copyBufferRect(cl_mem cl_buffer, cl_mem cl_copy, Matrix<size_t> &buffer_size, Matrix<size_t> &buffer_origin, Matrix<size_t> &copy_size, Matrix<size_t> &copy_origin, Matrix<size_t> &local_ws);
    
    float sumGpuArray(cl_mem cl_data, unsigned int read_size, Matrix<size_t> &local_ws);
    
    void selectionCalculus(Selection *area, cl_mem image_data_cl, cl_mem image_pos_weight_x_cl_new, cl_mem image_pos_weight_y_cl_new, Matrix<size_t> &image_size, Matrix<size_t> &local_ws);
    
    // Convenience 
    void refreshDisplay();
    void refreshSelection(Selection *area);
    
    // GPU buffer management
    void maintainImageTexture(Matrix<size_t> &image_size);
    void clMaintainImageBuffers(Matrix<size_t> &image_size);
    
    // GPU buffers
    cl_mem image_data_raw_cl;
    cl_mem image_data_trace_cl;
    cl_mem image_data_corrected_cl;
//    cl_mem xyzi_buf_cl;
    cl_mem image_data_variance_cl;
    cl_mem image_data_skewness_cl;
    cl_mem image_data_weight_x_cl;
    cl_mem image_data_weight_y_cl;
    cl_mem image_data_generic_cl;
    
    // Misc
    Matrix<double> getPlane();
    QString integrationFrameString(DetectorFile &f, ImageInfo &image);
    
//    SharedContextWindow * shared_window;
    
    size_t n_reduced_pixels;
    
    cl_int err;
    cl_program program;
    cl_kernel cl_display_image;
    cl_kernel cl_image_calculus;
    cl_kernel cl_buffer_max;
    cl_kernel project_kernel;
    cl_mem image_tex_cl;
    cl_mem source_cl;
    cl_mem tsf_tex_cl;
    cl_mem parameter_cl;
    cl_mem image_intensity_cl;
    cl_mem image_pos_weight_x_cl;
    cl_mem image_pos_weight_y_cl;
    
    cl_sampler tsf_sampler;
    cl_sampler image_sampler;

    TransferFunction tsf;
    int rgb_style, alpha_style;
    int bg_sample_interdist;
    
    int n_lsq_samples;


    GLuint image_tex_gl;
    GLuint tsf_tex_gl;
    Matrix<size_t> image_tex_size;
    Matrix<size_t> image_buffer_size;
    
    // Eventually merge the following two objects into a single class, or at least name them appropriately
    DetectorFile frame;

    void initOpenCL();
    void setParameter(Matrix<float> &data);
    void setTsf(TransferFunction & tsf);
    Matrix<double> getScatteringVector(DetectorFile & f, double x, double y);
    double getScatteringAngle(DetectorFile & f, double x, double y);
    Matrix<float> parameter;
    
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);

    // Draw
    void drawImage(QRectF rect, GLuint texture, QPainter * painter);
    void drawSelection(Selection area, QPainter *painter, Matrix<float> &color, QPointF offset = QPointF(0,0));
    void drawWeightpoint(Selection area, QPainter *painter, Matrix<float> &color);
    void drawPixelToolTip(QPainter * painter);
    void drawPlaneMarkerToolTip(QPainter *painter);
    void drawConeEwaldIntersect(QPainter *painter);

    // Boolean checks
    bool isImageTexInitialized;
    bool isTsfTexInitialized;
    bool isCLInitialized;
    bool isFrameValid;
    bool isWeightCenterActive;
    bool isInterpolGpuInitialized;
    bool isSetTraced;
    bool isSwapped;
    
    int texture_number;
    
    int isCorrectionNoiseActive; // Happens in calculus function
    int isCorrectionPlaneActive; // Happens in calculus function
    int isCorrectionClutterActive; // Should happen in a kernel that works on image objects
    int isCorrectionMedianActive; // Should happen in a kernel that works on image objects
    int isCorrectionPolarizationActive; // Happens in calculus function
    int isCorrectionFluxActive; // Happens in calculus function
    int isCorrectionExposureActive; // Happens in calculus function

    Matrix<double> texture_view_matrix; // Used to draw main texture
    Matrix<double> translation_matrix;
    Matrix<double> zoom_matrix;
    
    Matrix<double> texel_view_matrix; // Used for texel overlay
    Matrix<double> texel_offset_matrix;

    // Mouse
    QPoint pos;
    QPoint prev_pos;
    Matrix<int> prev_pixel;

    // Display
    int isLog;
    int isCorrectionLorentzActive;
    int isBackgroundCorrected;
    int mode;

    // Selection
    GLuint selections_vbo[5];
    GLuint weightpoints_vbo[5];
    QPoint getImagePixel(QPoint pos);

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

    typedef cl_int (*PROTOTYPE_QOpenCLEnqueueCopyBufferToImage) ( 	cl_command_queue command_queue,
                                                                    cl_mem src_buffer,
                                                                    cl_mem  dst_image,
                                                                    size_t src_offset,
                                                                    const size_t dst_origin[3],
                                                                    const size_t region[3],
                                                                    cl_uint num_events_in_wait_list,
                                                                    const cl_event *event_wait_list,
                                                                    cl_event *event);
    
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
                                                                    void *ptr,
                                                                    cl_uint num_events_in_wait_list,
                                                                    const cl_event *event_wait_list,
                                                                    cl_event *event);

    typedef cl_int (*PROTOTYPE_QOpenCLReleaseKernel)  ( 	cl_kernel kernel);

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
    PROTOTYPE_QOpenCLCreateKernel QOpenCLCreateKernel;
    PROTOTYPE_QOpenCLGetProgramBuildInfo QOpenCLGetProgramBuildInfo;
    PROTOTYPE_QOpenCLCreateContext QOpenCLCreateContext;
    PROTOTYPE_QOpenCLCreateCommandQueue QOpenCLCreateCommandQueue;
    PROTOTYPE_QOpenCLCreateProgramWithSource QOpenCLCreateProgramWithSource;
    PROTOTYPE_QOpenCLGetPlatformIDs QOpenCLGetPlatformIDs;
    PROTOTYPE_QOpenCLGetDeviceIDs QOpenCLGetDeviceIDs;
    PROTOTYPE_QOpenCLGetPlatformInfo QOpenCLGetPlatformInfo;
    PROTOTYPE_QOpenCLGetDeviceInfo QOpenCLGetDeviceInfo;
    PROTOTYPE_QOpenCLEnqueueReadBufferRect QOpenCLEnqueueReadBufferRect;


protected:
//    void initiaize();
//    void render(QPainter *painter);
};

//class ImagePreviewWindow : public OpenGLWindow
//{
//    Q_OBJECT

//signals:
//    void selectionActiveChanged(bool value);
    
//public:
//    ImagePreviewWindow();
//    ~ImagePreviewWindow();

//    void setSharedWindow(SharedContextWindow * window);
//    ImagePreviewWorker *worker();

//    void initializeWorker();

//public slots:
//    void renderNow();
//    void keyPressEvent(QKeyEvent *ev);
//    void keyReleaseEvent(QKeyEvent *ev);

//private:
//    bool isInitialized;

//    SharedContextWindow * shared_window;
//    ImagePreviewWorker * gl_worker;
//};

#endif // IMAGEPREVIEW_H
