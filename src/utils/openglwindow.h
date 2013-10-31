#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

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

signals:
    void finished();

public slots:
    void process();
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

    double fps;
    QElapsedTimer fps_elapsed_timer;
};


class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0, QOpenGLContext * shareContext = 0);
    ~OpenGLWindow();

    void setAnimating(bool animating);
    void preInitialize();   // The idea was to call this instead of show(), but it didnt work
    QOpenGLContext * getGLContext();
    OpenCLContext * getCLContext();
    void setOpenGLWorker(OpenGLWorker * worker);


signals:
    void mouseMoveEventCaught(QMouseEvent* ev);
    void wheelEventCaught(QWheelEvent* ev);
    void resizeEventCaught(QResizeEvent* ev);

public slots:
    void renderLater();
    void renderNow();
    void startAnimating();
    void stopAnimating();
    void setSwapState();
    void setOpenCLContext(OpenCLContext * context);

protected:
    // Event handling
    virtual void initialize();
    bool event(QEvent *event);
    void mouseMoveEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void exposeEvent(QExposeEvent *event);
    QOpenGLContext *shared_context;


private:
    OpenGLWorker * gl_surface;
    QThread * worker_thread;
    QOpenGLContext *context_gl;
    OpenCLContext *context_cl;

    bool isBufferBeingSwapped;
    bool isUpdatePending;
    bool isAnimating;
};
#endif
