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
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QObject>
#include <QRunnable>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QMutex>


#include "../misc/transferfunction.h"
#include "../misc/imagemarker.h"
#include "../file/framecontainer.h"
#include "../file/fileformat.h"
#include "../opencl/contextcl.h"
#include "../math/matrix.h"
#include "../math/colormatrix.h"
#include "../math/rotationmatrix.h"
#include "../svo/sparsevoxeloctree.h"


class ImageWorker : public QObject, protected OpenCLFunctions
{
        Q_OBJECT

    public:
        ImageWorker();
        ~ImageWorker();
        void setTraceContainer(QList<Matrix<float>> * list);

    public slots:
        void traceSeries(SeriesSet set);
        void reconstructSet(SeriesSet set);

    signals:
        void traceFinished();
        void progressRangeChanged(int, int);
        void progressChanged(int);

    private:
        OpenCLContextQueueProgram context_cl;
        cl_kernel cl_buffer_max;
        QList<Matrix<float>> * traces;


        void initializeOpenCLKernels();
        cl_int err;
//        cl_program program;
};

class ImageOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, protected OpenCLFunctions
{
        Q_OBJECT
    public:
        int projectFile(DetectorFile * image, Selection selection, Matrix<float> * samples, size_t * n_samples);
        void setReducedPixels(Matrix<float> * reduced_pixels);

        explicit ImageOpenGLWidget(QObject * parent = 0);
        ~ImageOpenGLWidget();
//        SeriesSet set();
        ImageWorker * worker();

        QFutureWatcher<void> *tree_GrowWatcher();
        QFutureWatcher<void> *tree_RebuildBranchesWatcher();
        QFutureWatcher<void> *tree_RecombineWatcher();
        QFutureWatcher<void> *tree_ReorganizeWatcher();
        QFutureWatcher<void> *voxelTreeWatcher();

    signals:
        void runTraceWorker(SeriesSet set);

        void message(QString);
        void message(QString, int);
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
        void imageRangeChanged(int low, int high);
        void currentIndexChanged(int value);
//        void progressBarFormatChanged(QString);
        void progressChanged(int value);
        void progressRangeChanged(int min, int max);
        void progressTaskActive(bool value);

    public slots:
        void pollProgress();

        void growVoxelTree();
        void on_growVoxelTree_finished();
        void on_growVoxelTree_canceled();

        void tree_Recombine();
        void on_tree_Recombine_finished();
        void on_tree_Recombine_canceled();

        void tree_Reorganize();
        void on_tree_Reorganize_finished();
        void on_tree_Reorganize_canceled();

        void tree_RebuildBranches();
        void on_tree_RebuildBranches_finished();
        void on_tree_RebuildBranches_canceled();

        void tree_Grow();
        void on_tree_Grow_finished();
        void on_tree_Grow_canceled();

        void setApplicationMode(QString str);
        void setFilePath(QString str);

        void setSeriesTrace();
        void traceSeriesSlot();

        void setBeamOverrideActive(bool value);
        void setBeamXOverride(double value);
        void setBeamYOverride(double value);

        void killProcess();
        void setOffsetOmega(double value);
        void setOffsetKappa(double value);
        void setOffsetPhi(double value);

        void setMode(int value);
        void setNoise(double value);
        void setRgb(QString style);
        void setAlpha(QString style);
        void setLog(bool value);
        void setCorrectionLorentz(bool value);
        void setCorrectionBackground(bool value);
        void setDataMin(double value);
        void setDataMax(double value);
        void processScatteringDataProxy();
        void takeScreenShot(QString path);
        void saveImage(QString path);
        void setFrame();
        void centerImage(QSizeF size);
        void centerCurrentImage();
        void analyze(QString str);
        void applyPlaneMarker(QString str);
        void showWeightCenter(bool value);
        void showImageTooltip(bool value);
        void showEwaldCircle(bool value);
        void applySelection();
        void setCorrectionNoise(bool value);
        void setCorrectionPlane(bool value);
        void setCorrectionClutter(bool value);
        void setCorrectionMedian(bool value);
        void setCorrectionPolarization(bool value);
        void setCorrectionFlux(bool value);
        void setCorrectionExposure(bool value);
        void setCorrectionPixelProjection(bool value);
        void setLsqSamples(int value);
        void showTraceTexture(bool value);

        void mouseMoveEvent(QMouseEvent * event);
        void mousePressEvent(QMouseEvent * event);
        void mouseReleaseEvent(QMouseEvent * event);
        void wheelEvent(QWheelEvent * event);


    private:
        SparseVoxelOctree p_voxel_tree;

        int p_n_returned_tasks;
        QStringList p_file_checklist;

        QOpenGLTexture * texture_noimage;
        QOpenGLTexture * texture_image_marker;
        QList<QList<ImageMarker>> image_markers;

        float beam_x_override, beam_y_override;

        QStaticText m_staticText;

        QThread * workerThread;
        ImageWorker * imageWorker;

        QPointF posGLtoQt(QPointF coord);
        QPointF posQttoGL(QPointF coord);

        void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
        Matrix<GLfloat> glRect(QRectF &qt_rect);

        GLuint loadShader(GLenum type, const char * source);

        // Shaders
        GLint std_2d_tex_fragpos;
        GLint std_2d_tex_pos;
        GLint std_2d_tex_texture;
        GLint std_2d_tex_transform;
        QOpenGLShaderProgram * std_2d_tex_program;

        GLint std_2d_sprite_fragpos;
        GLint std_2d_sprite_pos;
        GLint std_2d_sprite_texture;
        GLint std_2d_sprite_transform;
        QOpenGLShaderProgram * std_2d_sprite_program;

        GLint std_2d_col_color;
        GLint std_2d_col_transform;
        GLint std_2d_col_fragpos;
        QOpenGLShaderProgram * std_2d_col_program;

        OpenCLContextQueueProgram context_cl;

        void paintGL();
        void resizeGL(int w, int h);
        void initializeGL();

        Matrix<float> * reduced_pixels;
        double offset_omega;
        double offset_kappa;
        double offset_phi;
//        QString active_rotation;
        bool kill_flag;

        // Series
        QList<Matrix<float>> set_trace;
//        SeriesSet p_set;
        QString p_current_filepath;
        QString p_application_mode;
        QMap<QString, ImageInfo> p_working_data;
        cl_mem series_interpol_gpu_3Dimg;


        // GPU functions
        void processScatteringData(cl_mem data_buf_cl, cl_mem out_buf_cl, Matrix<float> &param, Matrix<size_t> &image_size, Matrix<size_t> &local_ws, float mean, float deviation, int task);

        void scatteringDataToImage(cl_mem data_buf_cl, cl_mem frame_image_cl, cl_mem tsf_image_cl, Matrix<float> &data_limit, Matrix<size_t> &image_size, Matrix<size_t> &local_ws, cl_sampler tsf_sampler, int log);

        float sumGpuArray(cl_mem cl_data, unsigned int read_size, Matrix<size_t> &local_ws);

        void processSelectionData(Selection * area, cl_mem image_data_cl, cl_mem image_pos_weight_x_cl_new, cl_mem image_pos_weight_y_cl_new, Matrix<size_t> &image_size, Matrix<size_t> &local_ws);

        // Convenience
        void updateImageTexture();
        void processSelectionDataProxy(Selection * area);

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
//        cl_program program;
        cl_kernel cl_data_to_image;
        cl_kernel cl_process_data;
        cl_kernel cl_buffer_max;
        cl_kernel cl_project_data;
        cl_kernel cl_parallel_reduction;
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
//        QString rgb_style, alpha_style;
        int bg_sample_interdist;

        int n_lsq_samples;


        GLuint image_tex_gl;
        GLuint tsf_tex_gl;
        Matrix<size_t> image_tex_size;
        Matrix<size_t> image_buffer_size;

        // Eventually merge the following two objects into a single class, or at least name them appropriately
        DetectorFile image;

        void initializeCL();
        void setParameter(Matrix<float> &data);
        void setTsf(TransferFunction &tsf);
        Matrix<double> getScatteringVector(DetectorFile &f, double x, double y);
        double getScatteringAngle(DetectorFile &f, double x, double y);
        Matrix<float> parameter;

        void beginRawGLCalls(QPainter * painter);
        void endRawGLCalls(QPainter * painter);

        // Draw
        void drawImage(QRectF rect, GLuint texture, QPainter * painter);
        void drawSelection(Selection area, QPainter * painter, Matrix<float> &color, QPointF offset = QPointF(0, 0));
        void drawWeightpoint(Selection area, QPainter * painter);
        void drawPixelToolTip(QPainter * painter);
        void drawPlaneMarkerToolTip(QPainter * painter);
        void drawConeEwaldIntersect(QPainter * painter);
        void drawImageMarkers(QPainter * painter);

        // Boolean checks
        bool isBeamOverrideActive;
        bool isImageTexInitialized;
        bool isTsfTexInitialized;
        bool isCLInitialized;
        bool isGLInitialized;
        bool isWeightCenterActive;
        bool isSetTraced;
        bool isEwaldCircleActive;
        bool isImageTooltipActive;
        bool is_tree_Grow_canceled;
        bool is_tree_Recombine_canceled;
        bool is_tree_RebuildBranches_canceled;
        bool is_growVoxelTree_canceled;
        bool is_tree_Reorganize_canceled;

        int texture_number;

        int isCorrectionNoiseActive; // Happens in calculus function
        int isCorrectionPlaneActive; // Happens in calculus function
        int isCorrectionClutterActive; // Should happen in a kernel that works on image objects
        int isCorrectionMedianActive; // Should happen in a kernel that works on image objects
        int isCorrectionPolarizationActive; // Happens in calculus function
        int isCorrectionFluxActive; // Happens in calculus function
        int isCorrectionExposureActive; // Happens in calculus function
        int isCorrectionPixelProjectionActive;

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

        QMutex mutex;

        QTimer * progressPollTimer;

        QList<DetectorFile> p_detectorfile_future_list;
        QVector<QList<SearchNode*>> p_searchnode_hierarchy_future_list;
        QVector<SearchNode*> p_searchnode_all_future_list;
        QFutureWatcher<void> * p_tree_grow_future_watcher;
        QFutureWatcher<void> * p_tree_rebuild_branches_future_watcher;
        QFutureWatcher<void> * p_tree_reorganize_future_watcher;
        QFutureWatcher<void> * p_tree_recombine_future_watcher;
        QFutureWatcher<void> * p_tree_voxelize_future_watcher;

        SearchNode p_data_tree;

        void printNodes();
};

#endif // IMAGEPREVIEW_H
