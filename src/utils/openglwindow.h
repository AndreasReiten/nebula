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

class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0, QOpenGLContext * shareContext = 0);
    ~OpenGLWindow();

    void setContextCL();
    void setAnimating(bool animating);
    void preInitialize(); // The idea was to call this instead of show(), but it didnt work
    void setContextCL(ContextCL * context);
    QOpenGLContext * getGLContext();


    // Convenience functions
    void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
    void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
    int getFps();

public slots:
    void renderLater();
    void renderNow();

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

    QTimer fps_timer;
    QElapsedTimer fps_elapsed_timer;

    int frames;
    int fps;
};
#endif
