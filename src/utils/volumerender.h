#ifndef VOLUMERENDER_H
#define VOLUMERENDER_H

#include "../../lib/qxlib/qxlib.h"
#include "marker.h"

class VolumeRenderWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit VolumeRenderWorker(QObject *parent = 0);
    ~VolumeRenderWorker();

    void setSharedWindow(SharedContextWindow * window);
    void setSvo(SparseVoxelOcttree * svo);
    void setUBMatrix(UBMatrix<double> & mat);
    UBMatrix<double> & getUBMatrix();

signals:
    void changedMessageString(QString str);

public slots:
    void setCountIntegration();
    void addMarker();
    void setQuality(int value);
    void refreshTexture();
    void setScalebar();
    void setProjection();
    void setBackground();
    void setLog(bool value);
    void setLogarithmic2D();
    void setDataStructure();
    void setTsfColor(int value);
    void setTsfAlpha(int value);
    void setDataMin(double value);
    void setDataMax(double Value);
    void setAlpha(double value);
    void setBrightness(double value);
    void setUnitcell();
    void setModel();
    void setModelParam0(double value);
    void setModelParam1(double value);
    void setModelParam2(double value);
    void setModelParam3(double value);
    void setModelParam4(double value);
    void setModelParam5(double value);
    void setSlicing();
    void setIntegration2D();
    void setIntegration3D();
    void setShadow();
    void setShadowVector();
    void setOrthoGrid();
    void takeScreenShot(QString path);
    void updateUnitCellText();
    void updateUnitCellVertices();
    void setURotation();
    void setHCurrent(int value);
    void setKCurrent(int value);
    void setLCurrent(int value);
    void setMiniCell();
    void setLabFrame();
    void setViewMode(int value);
    
    
    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    
    // Alignment and fixed rotation 
    void alignLabXtoSliceX();
    void alignLabYtoSliceY();
    void alignLabZtoSliceZ();
    void alignSliceToLab();
    
    void alignAStartoZ();
    void alignBStartoZ();
    void alignCStartoZ();
    
    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();
    void rollCW();
    void rollCCW();
    
    
    void toggleRuler();
    
    // UB matrix help slots
    void setUB_a(double value);
    void setUB_b(double value);
    void setUB_c(double value);
    
    void setUB_alpha(double value);
    void setUB_beta(double value);
    void setUB_gamma(double value);
    
protected:
    void initialize();
    void render(QPainter *painter);

private:
    SharedContextWindow * shared_window;

    // Sum
    float sumGpuArray(cl_mem cl_data, unsigned int read_size, size_t work_group_size);

    // Box integral
    float sumViewBox();

    // Boolean checks
    bool isInitialized;
    bool isRayTexInitialized;
    bool isTsfTexInitialized;
    bool isIntegrationTexInitialized;
    bool isDSActive;
    bool isOrthonormal;
    bool isLogarithmic;
    bool isModelActive;
    bool isUnitcellActive;
    bool isSvoInitialized;
    bool isScalebarActive;
    bool isSlicingActive;
    bool isIntegration2DActive;
    bool isIntegration3DActive;
    bool isShadowActive;
    bool isLogarithmic2D;
    bool isOrthoGridActive;
    bool isBackgroundBlack;
    bool isDataExtentReadOnly;
    bool isCenterlineActive;
    bool isRulerActive;
    bool isLMBDown;
    bool isURotationActive;
    bool isLabFrameActive;
    bool isMiniCellActive;
    bool isCountIntegrationActive;
    bool displayDistance;
    bool displayFps;
    bool displayResolution;
    
    // Markers
    QVector<Marker> markers;
    QVector<GLuint> marker_vbo;
    void drawMarkers(QPainter *painter);
    
    GLuint marker_centers_vbo;
    int n_marker_indices;
    Matrix<GLuint> markers_selected_indices;
    
    // Mini unit cell
    void drawHelpCell(QPainter *painter);
    GLuint minicell_vbo;
    
    
    // Ray texture
    Matrix<double> pixel_size;
    Matrix<int> ray_tex_dim;
    Matrix<size_t> ray_glb_ws;
    Matrix<size_t> ray_loc_ws;
    int quality_percentage;
    cl_mem ray_tex_cl;
    GLuint ray_tex_gl;
    void setRayTexture(int percentage);
    void raytrace(cl_kernel kernel);
    
    // Center line
    GLuint centerline_vbo;
    void setCenterLine();
    Matrix<GLfloat> centerline_coords;
    
    // Roll 
    double accumulated_roll;
    
    // Hkl selection
    Matrix<int> hklCurrent;
    void setHkl(Matrix<int> & hkl);
    
    // Hkl text 
    Matrix<double> hkl_text;
    size_t hkl_text_counter;
    
    // Integration
    cl_sampler integration_sampler_cl;
    cl_mem integration_tex_alpha_cl;
    cl_mem integration_tex_beta_cl;
    
    // Lab frame
    void drawLabFrame(QPainter * painter);
    GLuint lab_frame_vbo;
    
    // UB matrix implementation
    void drawUnitCell(QPainter *painter);
    
    GLuint unitcell_vbo;
    int unitcell_nodes;
    UBMatrix<double> UB;
    
    // Ruler
    Matrix<double> ruler;
    
    // Ticks
    void tickzerize(double min, double max, double size, double min_interdist, double *qualified_exponent, double * start, size_t *num_ticks);
    
    // Sense of rotation
    GLuint point_vbo;
    
    // Drawing functions
    void drawRayTex(QPainter *painter);
    void drawPositionScalebars(QPainter *painter);
    void drawOverlay(QPainter *painter);
    void drawIntegral(QPainter *painter);
    void drawRuler(QPainter *painter);
    void drawGrid(QPainter *painter);
    void drawCountScalebar(QPainter *painter);
    void drawCenterLine(QPainter *painter);
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);
    void drawSenseOfRotation(double zeta, double eta, double rpm);
    void drawHklText(QPainter * painter);
    void drawCountIntegral(QPainter *painter);
    
    
    int fps_string_width_prev;
    
    // Drawing rectangles
    QRect fps_string_rect;
    
    // Core set functions
    void setDataExtent();
    void setViewMatrices();
    void resetViewMatrix();
    void setTsfParameters();
    void setMiscArrays();

    // Misc compute functions
    void computePixelSize();

    // Scalebars
    size_t setScaleBars();
    size_t scalebar_coord_count;
    GLuint scalebar_vbo;
    GLuint count_scalebar_vbo;
    double scalebar_multiplier;
    
    size_t n_position_scalebar_ticks;
    size_t n_count_scalebar_ticks;
    size_t n_count_minor_scalebar_ticks;
    
    
    Matrix<float> position_scalebar_ticks;
    Matrix<float> count_scalebar_ticks;
    Matrix<float> count_minor_scalebar_ticks;
    
    
    // Transfer function texture
    void setTsfTexture();
    cl_mem tsf_tex_cl;
    cl_sampler tsf_tex_sampler;
    GLuint tsf_tex_gl;
    GLuint tsf_tex_gl_thumb;
    TransferFunction tsf;
    int tsf_color_scheme;
    int tsf_alpha_scheme;
    
    // Timing
    QElapsedTimer session_age;
    
    // Ray texture timing
    QElapsedTimer ray_kernel_timer;

    // Mouse
    int last_mouse_pos_x;
    int last_mouse_pos_y;
    
    

    // View matrices
    Matrix<double> view_matrix;
    CCMatrix<double> ctc_matrix;
    RotationMatrix<double> rotation;
    Matrix<double> data_translation;
    Matrix<double> data_scaling;
    Matrix<double> bbox_scaling;
    Matrix<double> minicell_scaling;
    Matrix<double> bbox_translation;
    Matrix<double> normalization_scaling;
    Matrix<double> scalebar_view_matrix;
    Matrix<double> unitcell_view_matrix;
    Matrix<double> minicell_view_matrix;
    RotationMatrix<double> scalebar_rotation;
    Matrix<double> projection_scaling;
    RotationMatrix<double> U;
    
    // Other matrices
    Matrix<double> data_extent;
    Matrix<double> data_view_extent;
    Matrix<double> tsf_parameters_model;
    Matrix<double> tsf_parameters_svo;
    Matrix<int> misc_ints;
    Matrix<double> model_misc_floats;

    // OpenGL
    void initResourcesGL();

    // OpenCL
    cl_int err;
    cl_program program;
    cl_kernel cl_svo_raytrace;
    cl_kernel cl_model_raytrace;
    cl_kernel cl_integrate_image;
    cl_kernel cl_box_sampler;
    cl_kernel cl_parallel_reduce;

    cl_mem cl_glb_work;
    cl_mem cl_view_matrix_inverse;
    cl_mem cl_scalebar_rotation;
    cl_mem cl_data_extent;
    cl_mem cl_data_view_extent;
    cl_mem cl_tsf_parameters_model;
    cl_mem cl_tsf_parameters_svo;
    cl_mem cl_misc_ints;
    cl_mem cl_model_misc_floats;

    void initResourcesCL();

    // Svo
    cl_mem cl_svo_pool;
    cl_mem cl_svo_index;
    cl_mem cl_svo_brick;
    cl_sampler cl_svo_pool_sampler;

    // Colors
    ColorMatrix<GLfloat> marker_line_color;
    ColorMatrix<GLfloat> white;
    ColorMatrix<GLfloat> black;
    ColorMatrix<GLfloat> yellow;
    ColorMatrix<GLfloat> red;
    ColorMatrix<GLfloat> green;
    ColorMatrix<GLfloat> green_light;
    ColorMatrix<GLfloat> blue_light;
    ColorMatrix<GLfloat> magenta;
    ColorMatrix<GLfloat> magenta_light;
    ColorMatrix<GLfloat> blue;
    ColorMatrix<GLfloat> clear_color;
    ColorMatrix<GLfloat> clear_color_inverse;
    ColorMatrix<GLfloat> centerline_color;

    // Pens
    void initializePaintTools();
    QPen * normal_pen;
    QPen * border_pen;
    QPen * whatever_pen;
    QFont * minicell_font;
    QBrush * fill_brush;
    QBrush * histogram_brush;
    QBrush * normal_brush;
    QBrush * dark_fill_brush;
    QFont * normal_font;
    QFont * tiny_font;
    QFont * emph_font;
    QFontMetrics * normal_fontmetric;
    QFontMetrics * emph_fontmetric;
    QFontMetrics * minicell_fontmetric;

    // Shadow
    Matrix<float> shadow_vector;
    
    
    Matrix<float> identity;


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

class VolumeRenderWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    VolumeRenderWindow();
    ~VolumeRenderWindow();

    void setSharedWindow(SharedContextWindow * window);
    VolumeRenderWorker *getWorker();

    void initializeWorker();

public slots:
    void renderNow();

private:
    bool isInitialized;

    SharedContextWindow * shared_window;
    VolumeRenderWorker * gl_worker;
};

#endif
