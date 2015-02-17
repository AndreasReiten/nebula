#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

#include <QLibrary>
#include <CL/opencl.h>
#include <QOpenGLWidget>
#include <QPainter>
#include <QOpenGLPaintDevice>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QThread>
#include <QStaticText>


#include "../../opengl/qxopengllib.h"
#include "../../file/qxfilelib.h"
#include "../../opencl/qxopencllib.h"
#include "../../math/qxmathlib.h"

class ImageWorker : public QObject, protected OpenCLFunctions
{
    Q_OBJECT
    
public:
    ImageWorker();
    ~ImageWorker();
    void setOpenCLContext(OpenCLContext context);
    void setTraceContainer(QList<Matrix<float>> * list);

public slots:
    void traceSeries(SeriesSet set);

signals:
    void traceFinished();
    void visibilityChanged(bool value);
    void pathChanged(QString path);
    void progressRangeChanged(int,int);
    void progressChanged(int);

private:
    OpenCLContext context_cl;
    cl_kernel cl_buffer_max;
    QList<Matrix<float>> * traces;
    
    
    void initializeOpenCLKernels();
    cl_int err;
    cl_program program;
};

class ImageOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, protected OpenCLFunctions
{
    Q_OBJECT
public:
    int projectFile(DetectorFile * file, Selection selection, Matrix<float> *samples, size_t *n_samples);
    void setReducedPixels(Matrix<float> * reduced_pixels);
    
    explicit ImageOpenGLWidget(QObject *parent = 0);
    ~ImageOpenGLWidget();
    SeriesSet set();
    ImageWorker *worker();
    
signals:
    void runTraceWorker(SeriesSet set);
    
    void changedMessageString(QString str);
    void changedMemoryUsage(int value);
    void changedFormatMemoryUsage(QString str);
    void changedRangeMemoryUsage(int min, int max);
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
    void setSeriesTrace();
    void traceSeriesSlot();
    
    void setBeamOverrideActive(bool value);
    void setBeamXOverride(double value);
    void setBeamYOverride(double value);
    
    void killProcess();
    void setActiveAngle(QString value);
    void setOffsetOmega(double value);
    void setOffsetKappa(double value);
    void setOffsetPhi(double value);
    void reconstruct();
    
    void setMode(int value);
    void setNoise(double value);
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
//    void traceSeries();
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
    
    
private:
    float beam_x_override, beam_y_override; 
    
    QStaticText m_staticText;

    QThread * workerThread;
    ImageWorker * imageWorker;

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

    OpenCLContext context_cl;

    void paintGL();
    void resizeGL(int w, int h);
    void initializeGL();

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
    cl_mem image_data_variance_cl;
    cl_mem image_data_skewness_cl;
    cl_mem image_data_weight_x_cl;
    cl_mem image_data_weight_y_cl;
    cl_mem image_data_generic_cl;
    
    // Misc
    Matrix<double> getPlane();
    QString integrationFrameString(DetectorFile &f, ImageInfo &image);
    
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
    DetectorFile file;

    void initializeCL();
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
    bool isBeamOverrideActive;
    bool isImageTexInitialized;
    bool isTsfTexInitialized;
    bool isCLInitialized;
    bool isGLInitialized;
//    bool isFrameValid;
    bool isWeightCenterActive;
//    bool isInterpolGpuInitialized;
    bool isSetTraced;
//    bool isSwapped;
    
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
};

#endif // IMAGEPREVIEW_H
