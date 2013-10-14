#include "openglwindow.h"

OpenGLWindow::OpenGLWindow(QWindow *parent, QOpenGLContext * shareContext)
    : QWindow(parent)
    , isUpdatePending(false)
    , isAnimating(false)
    , context_gl(0)
    , paint_device_gl(0)
{
    this->shared_context = shareContext;

    setSurfaceType(QWindow::OpenGLSurface);

    fps_timer.setInterval(200);
    connect(&fps_timer, SIGNAL(timeout()), this, SLOT(setFps()));

    frames = 0;
    fps_timer.start();
    fps_elapsed_timer.start();
}

OpenGLWindow::~OpenGLWindow()
{
    std::cout << Q_FUNC_INFO << std::endl;
    delete paint_device_gl;
}

QOpenGLContext * OpenGLWindow::getGLContext()
{
    return context_gl;
}

void OpenGLWindow::setContextCL(ContextCL * context)
{
    this->context_cl = context;
}

void OpenGLWindow::setFps()
{
    fps = (float) frames * 1000.0 / (float) fps_elapsed_timer.restart();
    frames = 0;
}

int OpenGLWindow::getFps()
{
    return fps;
}

void OpenGLWindow::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void OpenGLWindow::initialize()
{
}

void OpenGLWindow::render()
{
    if (!paint_device_gl)
        paint_device_gl = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    paint_device_gl->setSize(size());

    QPainter painter(paint_device_gl);
    painter.setPen(Qt::yellow);
    painter.setFont(QFont("Monospace"));

    render(&painter);
}

void OpenGLWindow::renderLater()
{
    if (!isUpdatePending) {
        isUpdatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        isUpdatePending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void OpenGLWindow::preInitialize()
{
    bool needsInitialize = false;

    if (!context_gl)
    {
        context_gl = new QOpenGLContext(this);
        context_gl->setFormat(requestedFormat());
        if (shared_context != 0)
        {
            std::cout << "Sharing context" << std::endl;
            context_gl->setShareContext(shared_context);
        }
        context_gl->create();

        std::stringstream ss;

        ss << std::endl << "_____ OpenGL Context Info _____" << std::endl;
        ss << "OpenGL version:   " << context_gl->format().version().first << "." << context_gl->format().version().second << std::endl;
        ss << "MSAA Samples:     " << context_gl->format().samples() << std::endl;
        ss << "Alpha:            " << context_gl->format().hasAlpha() << std::endl;
        ss << "RGBA bits:        " << context_gl->format().redBufferSize() << " " << context_gl->format().greenBufferSize() << " " << context_gl->format().blueBufferSize() << " " << context_gl->format().alphaBufferSize() << std::endl;
        ss << "Depth bits:       " << context_gl->format().depthBufferSize() << std::endl;
        ss << "Stencil bits:     " << context_gl->format().stencilBufferSize() << std::endl;
        ss << "Stereo buffering: " << context_gl->format().stereo() << std::endl;
        ss << "Renderable type:  " << context_gl->format().renderableType() << std::endl;
        ss << "Swap behaviour:   " << context_gl->format().swapBehavior() << std::endl;
        ss << "_______________________________" << std::endl;

        qDebug() << ss.str().c_str();

        needsInitialize = true;
    }

    context_gl->makeCurrent(this);

    if (needsInitialize)
    {
        initializeOpenGLFunctions();
        initialize();
    }
}

void OpenGLWindow::renderNow()
{
    if (!isExposed())
        return;

    preInitialize();

    render();

    frames++;

    context_gl->swapBuffers(this);

    if (isAnimating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    isAnimating = animating;

    if (animating)
        renderLater();
}

