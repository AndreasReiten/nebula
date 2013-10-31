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

class OpenGLRenderWorker  : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLRenderWorker(QObject *parent = 0);
    ~OpenGLRenderWorker();

    // Sets
    void setContextCL(ContextCL * context);
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
    void setFps(); // stay
    double getFps();  // stay

protected:
    // Render
    virtual void render(QPainter *painter);
//    virtual void render();

    virtual void initialize();
    QOpenGLPaintDevice *paint_device_gl;
    QOpenGLContext *context_gl;
    ContextCL * context_cl;
    QWindow *render_surface;

private:
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

    void setAnimating(bool animating);  // stay
    void preInitialize();  // stay // The idea was to call this instead of show(), but it didnt work
    QOpenGLContext * getGLContext();  // stay
    ContextCL * getCLContext();  // stay
    void setOpenGLWorker(OpenGLRenderWorker * worker);


signals:
    void swappy();  // stay
    void mouseMoveEventCaught(QMouseEvent* ev);
    void wheelEventCaught(QWheelEvent* ev);
    void resizeEventCaught(QResizeEvent* ev);


public slots:
    void renderLater(); // stay
    void renderNow(); // stay
    void startAnimating();  // stay
    void stopAnimating();  // stay
    void setSwapState();  // stay
    void setContextCL(ContextCL * context);

protected:
    // Event handling
    virtual void initialize();
    bool event(QEvent *event);  // stay
    void mouseMoveEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void exposeEvent(QExposeEvent *event); // stay
    QOpenGLContext *shared_context; // stay


private:
    OpenGLRenderWorker * swap_surface; // stay
    QThread * swap_thread; // stay
    QOpenGLContext *context_gl; // stay
    ContextCL *context_cl;

    bool isBufferBeingSwapped; // stay
    bool isUpdatePending; // stay
    bool isAnimating; // stay
};
#endif
