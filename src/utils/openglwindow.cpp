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
    delete paint_device_gl;
}

void OpenGLWindow::setVbo(GLuint vbo, float * buf, size_t length, GLenum usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*length, buf, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    QPainter painter(paint_device_gl);
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
        ss << "Context:          " << context_gl << std::endl;
        ss << "Shared context:   " << context_gl->shareContext() << std::endl;
        ss << "OpenGL version:   " << context_gl->format().version().first << "." << context_gl->format().version().second << std::endl;
        ss << "MSAA samples:     " << context_gl->format().samples() << std::endl;
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
        if (!paint_device_gl) paint_device_gl = new QOpenGLPaintDevice;
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

    context_gl->swapBuffers(this); // Swapping buffers appears to be compute heavy (15 ms)

    if (isAnimating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    isAnimating = animating;

    if (animating)
        renderLater();
}

void OpenGLWindow::getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform)
{
    Matrix<float> pos_3d_matrix;
    Matrix<float> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = transform->toFloat() * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0]/pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1]/pos_2d_matrix[3];
}
