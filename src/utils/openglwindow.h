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
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QTimer>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QThread>
#include <QOpenGLFramebufferObject>
#include <QMouseEvent>
#include <QDebug>

#include "contextcl.h"
#include "matrix.h"

class OpenGLWorker  : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWorker(QObject *parent = 0);
    ~OpenGLWorker();

    // Sets
    void setOpenCLContext(OpenCLContext * context);
    void setGLContext(QOpenGLContext *context);
    void setRenderSurface(QWindow *surface);

    // Convenience functions
    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
    void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
    QPointF coordQttoGL(QPointF coord);
    void glRect(Matrix<GLfloat> * gl_rect, QRect * qt_rect);
    void setMultiThreading(bool value);

signals:
    void finished();

public slots:
    void process();
    virtual void metaMouseMoveEventCompact(QMouseEvent ev);
    virtual void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    virtual void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    virtual void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    virtual void mouseMoveEvent(QMouseEvent* ev);
    virtual void wheelEvent(QWheelEvent* ev);
    virtual void resizeEvent(QResizeEvent * ev);
    void setFps();

protected:
    double getFps();

    // Render
    virtual void render(QPainter *painter);
    virtual void initialize();

    QOpenGLPaintDevice *paint_device_gl;
    QOpenGLContext *context_gl;
    OpenCLContext *context_cl;
    QWindow *render_surface;

private:
    // Boolean checks
    bool isInitialized;
    bool isMultiThreaded;

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
    void setAnimating(bool animating);
    void setMultiThreading(bool value);
    QOpenGLContext * getGLContext();
    OpenCLContext * getCLContext();

signals:
    void metaMouseMoveEventCaught(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEventCaught(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEventCaught(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseMoveEventCaughtCompact(QMouseEvent ev);
    void mouseMoveEventCaught(QMouseEvent* ev);
    void wheelEventCaught(QWheelEvent* ev);
    void resizeEventCaught(QResizeEvent* ev);
    void stopRendering();
    void render();

public slots:
    void startAnimating();
    void stopAnimating();
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
    void resizeEvent(QResizeEvent * ev);
    void exposeEvent(QExposeEvent *event);



//private:
    QOpenGLContext *shared_context;
    QThread *worker_thread;
    QOpenGLContext *context_gl;
    OpenCLContext *context_cl;
    
    bool isUpdatePending;
    bool isWorkerBusy;
    bool isAnimating;
    bool isMultiThreaded;
};
#endif
