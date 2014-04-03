#ifndef VOLUMERENDER_H
#define VOLUMERENDER_H

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
//#include <cstdio>

#include <QtGlobal>
#include <QDebug>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QResizeEvent>
#include <QPolygonF>


#include "tools.h"
//#include "miniarray.h"
#include "matrix.h"
#include "marker.h"
#include "sparsevoxelocttree.h"
#include "openglwindow.h"
#include "sharedcontext.h"
#include "transferfunction.h"

#include <QScreen>
#include <QPainter>



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
//    void renderState(int value);
    void changedMessageString(QString str);

public slots:
    void addMarker();
    void setQuality(int value);
    void setScalebar();
    void setProjection();
    void setBackground();
    void setLogarithmic();
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
    
    
//    void metaMouseMoveEventCompact(QMouseEvent ev);
    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
//    void mouseMoveEvent(QMouseEvent* ev);
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
    bool isRendering;
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
//    bool isUBActive;
    
    // Markers
    QVector<Marker> markers;
    QVector<GLuint> marker_vbo;
    void drawMarkers(QPainter *painter);
    
    GLuint marker_centers_vbo;
    int n_marker_indices;
    Matrix<GLuint> markers_selected_indices;
//    Matrix<double> marker_centers;
    
    // Mini unit cell
    void drawHelpCell(QPainter *painter);
    GLuint minicell_vbo;
    
    
    // Ray texture
    Matrix<double> pixel_size;
    Matrix<int> ray_tex_dim;
    Matrix<size_t> ray_glb_ws;
    Matrix<size_t> ray_loc_ws;
    float weird_parameter;
    cl_mem ray_tex_cl;
    GLuint ray_tex_gl;
    void setRayTexture();
    void raytrace(cl_kernel kernel);
    double work, work_time, quality_factor;
    
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
    
    
    int fps_string_width_prev;
    
    // Drawing rectangles
    QRect multiplier_string_rect;
    
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
    float fps_requested;
    QElapsedTimer ray_kernel_timer;

    // Mouse
    int last_mouse_pos_x;
    int last_mouse_pos_y;
    
    

    // View matrices
    Matrix<double> view_matrix;
//    Matrix<double> marker_matrix;
    CameraToClipMatrix<double> ctc_matrix;
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
//    Matrix<double> lab_frame_view_matrix;
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
    Color<GLfloat> marker_line_color;
    Color<GLfloat> white;
    Color<GLfloat> black;
    Color<GLfloat> yellow;
    Color<GLfloat> red;
    Color<GLfloat> green;
    Color<GLfloat> green_light;
    Color<GLfloat> blue_light;
    Color<GLfloat> magenta;
    Color<GLfloat> magenta_light;
    Color<GLfloat> blue;
    Color<GLfloat> clear_color;
    Color<GLfloat> clear_color_inverse;
    Color<GLfloat> centerline_color;

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

    // Shadow
    Matrix<float> shadow_vector;
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
