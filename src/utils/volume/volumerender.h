#ifndef VOLUMERENDER_H
#define VOLUMERENDER_H

#include <QThread>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>

#include "../math/qxmathlib.h"
#include "../misc/transferfunction.h"
#include "../opencl/qxopencllib.h"
#include "../file/qxfilelib.h"
#include "../svo/qxsvolib.h"
#include "../misc/line.h"
#include "../misc/marker.h"

class VolumeWorker : public QObject, protected OpenCLFunctions
{
    Q_OBJECT
public:
    VolumeWorker();
    ~VolumeWorker();
    void setOpenCLContext(OpenCLContext context);
    void setKernel(cl_kernel kernel);

public slots:
    void raytrace(Matrix<size_t> ray_glb_ws, Matrix<size_t> ray_loc_ws);
    void resolveLineIntegral();
    
signals:
    void rayTraceFinished();
    void integralResolved();

private:
    OpenCLContext context_cl;
    cl_kernel p_kernel;
    cl_int err;
};

class VolumeOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, protected OpenCLFunctions
{
    Q_OBJECT
public:
    explicit VolumeOpenGLWidget(QObject *parent = 0);
    ~VolumeOpenGLWidget();

    void setSvo(SparseVoxelOctree * svo);
    void setUBMatrix(UBMatrix<double> & mat);
    UBMatrix<double> & getUBMatrix();

signals:
    void changedMessageString(QString str);
    void linesChanged();

public slots:
    void refreshLine(int value);
    void addLine();
    void saveLineIntegralAsImage(QString path);
    void saveLineIntegralAsText(QString path);
    void toggleHkl();
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
    void setURotation(bool value);
    void setHCurrent(int value);
    void setKCurrent(int value);
    void setLCurrent(int value);
    void setMiniCell(bool value);
    void setLabFrame();
    void setViewMode(int value);
    
    
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent* ev);
    
    // Alignment and fixed rotation 
    void alignLabXtoSliceX();
    void alignLabYtoSliceY();
    void alignLabZtoSliceZ();
    void alignSliceToLab();
    
    void alignSlicetoAStar();
    void alignSlicetoBStar();
    void alignSlicetoCStar();

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
    
private:
    QList<Line> * lines;
    
    QThread * workerThread;
    VolumeWorker * volumeWorker;

    QPointF posGLtoQt(QPointF coord);
    QPointF posQttoGL(QPointF coord);
    
    void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
    void getPosition2D(double * pos_2d, double * pos_3d, Matrix<double> * transform);

    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
    Matrix<GLfloat> glRect(QRectF &qt_rect);

//    GLuint loadShader(GLenum type, const char *source);

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
    
    // Sum
    float sumGpuArray(cl_mem cl_data, unsigned int read_size, size_t work_group_size);

    // Box integral
    float sumViewBox();

    // Boolean checks
    bool isCLInitialized;
    bool isGLInitialized;
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
    bool isURotationActive;
    bool isLabFrameActive;
    bool isMiniCellActive;
    bool isCountIntegrationActive;
    bool displayDistance;
//    bool displayFps;
    bool displayResolution;
    bool isHklTextActive;

    // Lines
    QList<Line> integration_lines;
    
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
    void drawLineIntegrationVolumeVisualAssist(QPainter * painter);
    void drawLineIntegralGraph(QPainter * painter, QRect position);
    
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
    Matrix<float> count_major_scalebar_ticks;
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

    void initializeCL();

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
    QPen * anything_pen;
    QFont * minicell_font;
    QBrush * fill_brush;
    QBrush * normal_brush;
    QFont * normal_font;
    QFont * hkl_font;
    QFontMetrics * normal_fontmetric;
    QFontMetrics * minicell_fontmetric;

    // Shadow
    Matrix<float> shadow_vector;
    
    
    Matrix<float> identity;
};

#endif
