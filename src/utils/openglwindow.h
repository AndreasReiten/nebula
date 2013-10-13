#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <iostream>
#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QTimer>
#include <QElapsedTimer>
#include <QCoreApplication>

#include "contextcl.h"

class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0, OpenGLWindow * shareWindow = 0);
    ~OpenGLWindow();

    void setContextCL();
    void setAnimating(bool animating);
    void preInitialize(); // The idea was to call this instead of show(), but it didnt work
    void setContextCL(ContextCL * context);
    void setSharedWindow(OpenGLWindow * window);

    QOpenGLContext * getGLContext();
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
    OpenGLWindow *shared_window;
    ContextCL * context_cl;

private slots:
    void setFps();

private:
    bool isUpdatePending;
    bool isAnimating;

    QOpenGLContext *context_gl;
    QOpenGLPaintDevice *paint_device_gl;

    QTimer fps_timer;
    QElapsedTimer fps_elapsed_timer;

    int frames;
    int fps;
};
#endif
