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

class ImagePreviewWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImagePreviewWorker(QObject *parent = 0);
    ~ImagePreviewWorker();
    int setImageFromPath(QString path);
    void setSharedWindow(SharedContextWindow * window);

signals:
    
public slots:
    void setMode(int value);
    
    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    
private:
    SharedContextWindow * shared_window;

    cl_mem ray_tex_cl;
    GLuint ray_tex_gl;

    DetectorFile frame;
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
