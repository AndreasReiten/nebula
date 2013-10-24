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

#include "contextcl.h"
#include "matrix.h"

//class OpenGLRenderThread : public QWindow, protected QOpenGLFunctions
//{
//    Q_OBJECT
//public:
//    explicit OpenGLWindow(QWindow *parent = 0, QOpenGLContext * shareContext = 0);
//    ~OpenGLWindow();

//    void setContextCL(ContextCL * context);

//    // Convenience functions
//    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
//    void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
//    QPointF coordQttoGL(QPointF coord);
//    void glRect(Matrix<GLfloat> * gl_rect, QRect * qt_rect);

//protected:
//    virtual void render(QPainter *painter);

//    QOpenGLContext *shared_context;
//    ContextCL * context_cl;
//    QOpenGLPaintDevice *paint_device_gl;

//private:
//    QOpenGLContext *context_gl;
//};


class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0, QOpenGLContext * shareContext = 0);
    ~OpenGLWindow();

    void setAnimating(bool animating);
    void preInitialize(); // The idea was to call this instead of show(), but it didnt work
    void setContextCL(ContextCL * context);
    QOpenGLContext * getGLContext();


    // Convenience functions
    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
    void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
    QPointF coordQttoGL(QPointF coord);
    void glRect(Matrix<GLfloat> * gl_rect, QRect * qt_rect);
    double getFps();

public slots:
    void renderLater();
    void renderNow();
    void startAnimating();
    void stopAnimating();

protected:
    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initialize();

    bool event(QEvent *event);

    void exposeEvent(QExposeEvent *event);
    QOpenGLContext *shared_context;
    ContextCL * context_cl;

    QOpenGLPaintDevice *paint_device_gl;

private slots:
    void setFps();

private:
    bool isUpdatePending;
    bool isAnimating;

    QOpenGLContext *context_gl;

//    QTimer fps_timer;
    QElapsedTimer fps_elapsed_timer;

//    int frames;
    double fps;
};
#endif
