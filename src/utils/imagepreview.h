#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

/* GL and CL*/
#include <CL/opencl.h>

#include <QObject>
#include "openglwindow.h"
#include "sharedcontext.h"
#include "matrix.h"
#include "fileformat.h"
#include "transferfunction.h"

class ImagePreviewWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImagePreviewWorker(QObject *parent = 0);
    ~ImagePreviewWorker();
    void setSharedWindow(SharedContextWindow * window);

signals:
    
public slots:
    void setMode(int value);
    void setThresholdAlow(double value);
    void setThresholdAhigh(double value);
    void setThresholdBlow(double value);
    void setThresholdBhigh(double value);
    void setIntensityMin(double value);
    void setIntensityMax(double value);
    
    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void setImageFromPath(QString path);
    
private:
    SharedContextWindow * shared_window;

    cl_int err;
    cl_program program;
    cl_kernel cl_image_preview;
    cl_mem image_tex_cl;
    cl_mem source_cl;
    cl_mem tsf_tex_cl;
    cl_mem parameter_cl;
    
    cl_sampler tsf_sampler;
    cl_sampler image_sampler;

    TransferFunction tsf;

    GLuint image_tex_gl;
    GLuint tsf_tex_gl;

    DetectorFile frame;

    void initResourcesCL();
    void update(size_t w, size_t h);
    void setParameter(Matrix<float> &data);
    void setTsf(TransferFunction & tsf);
    Matrix<float> parameter;
    
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);

    bool isImageTexInitialized;
    bool isTsfTexInitialized;
    
protected:
    void initialize();
    void render(QPainter *painter);
};

class ImagePreviewWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    ImagePreviewWindow();
    ~ImagePreviewWindow();

    void setSharedWindow(SharedContextWindow * window);
    ImagePreviewWorker *getWorker();

    void initializeWorker();

public slots:
    void renderNow();

private:
    bool isInitialized;

    SharedContextWindow * shared_window;
    ImagePreviewWorker * gl_worker;
};

#endif // IMAGEPREVIEW_H
