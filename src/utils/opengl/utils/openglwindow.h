#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

/*
 * The motivation for having the rendering happen in a separate worker is to enable threading.
 * A OpenGLWindow which does not reside on the main thread will not block GUI events. However,
 * since explicit GUI calls must be done in the GUI (main) thread, it is unfeasible to simply
 * allocate the entire OpenGLWindow on a separate thread.
 */

#include <iostream>
#include <sstream>

#include <QWindow>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QElapsedTimer>
#include <QThread>
#include <QMouseEvent>

#include "../../opencl/qxopencllib.h"
#include "../../math/qxmathlib.h"

class OpenGLWorker  : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWorker(QObject *parent = 0);
    ~OpenGLWorker();

    // Sets
    void setOpenCLContext(OpenCLContext * context);
    void setOpenGLContext(QOpenGLContext * context);
    void setRenderSurface(QWindow * surface);

    // Convenience functions
    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
    void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
    void getPosition2D(double * pos_2d, double * pos_3d, Matrix<double> * transform);

    QPointF posGLtoQt(QPointF coord);
    QPointF posQttoGL(QPointF coord);
    Matrix<GLfloat> glRect(QRectF &qt_rect);
    void setMultiThreading(bool value);

signals:
    void finished();

public slots:
    void process();
    virtual void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    virtual void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    virtual void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    virtual void wheelEvent(QWheelEvent* ev);
    virtual void resizeEvent(QResizeEvent * ev);
//    virtual void keyPressEvent(QKeyEvent ev);
//    virtual void keyReleaseEvent(QKeyEvent * ev);
    void setFps();
    virtual void initialize();

protected:
    double getFps();

    // Render
    virtual void render(QPainter *painter);
//    virtual void initOpenGL();
//    virtual void initOpenCL();

    QOpenGLPaintDevice *paint_device_gl;
    QOpenGLContext *context_gl;
    OpenCLContext *context_cl;
    QWindow *render_surface;

    bool isInitialized;

private:
    // Boolean checks

    bool isThreaded;

    double fps;
    QElapsedTimer fps_elapsed_timer;
};


class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0, QOpenGLContext * shareContext = 0);
    ~OpenGLWindow();
    virtual void initializeWorker();
    void setMultiThreading(bool value);
    QOpenGLContext * getGLContext();
    OpenCLContext * getCLContext();

signals:
    void metaMouseMoveEventCaught(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEventCaught(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEventCaught(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
//    void mouseMoveEventCaught(QMouseEvent ev);
    void keyPressEventCaught(QKeyEvent ev);
    void keyReleaseEventCaught(QKeyEvent * ev);
    void wheelEventCaught(QWheelEvent* ev);
    void resizeEventCaught(QResizeEvent* ev);
    void stopRendering();
    void render();
    void setFocus();
    void workerReady();

public slots:
    void setSwapState();
    void setOpenCLContext(OpenCLContext * context);
    virtual void renderNow();

protected:
    void renderLater();
    virtual void initialize();
    void initializeGLContext();

    // Event handling
    bool event(QEvent *event);
    void mouseMoveEvent(QMouseEvent* ev);
    void mousePressEvent(QMouseEvent* ev);
    void mouseReleaseEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);
    void keyPressEvent(QKeyEvent * ev);
    void keyReleaseEvent(QKeyEvent * ev);
    void resizeEvent(QResizeEvent * ev);
    void exposeEvent(QExposeEvent *event);



//private:
    QOpenGLContext *shared_context;
    QThread *worker_thread;
    QOpenGLContext *context_gl;
    OpenCLContext *context_cl;
    
    bool isWorkerBusy;
    bool isThreaded;
};

const char * gl_error_cstring(GLenum err);

#endif
