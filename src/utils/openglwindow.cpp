#include "openglwindow.h"

OpenGLWorker::OpenGLWorker(QWindow *parent)
{
}

OpenGLWorker::~OpenGLWorker()
{

}

void OpenGLWorker::process()
{
    context_gl->makeCurrent(render_surface);
    compute();
    context_gl->swapBuffers(render_surface);
    context_gl->doneCurrent();
    context_gl->moveToThread(qApp->thread());
    emit finished();
}

void OpenGLWorker::compute()
{

}

void OpenGLWorker::setGLContext(QOpenGLContext *context)
{
    context_gl = context;
}

void OpenGLWorker::setRenderSurface(QSurface *surface)
{
    render_surface = surface;
}

OpenGLWindow::OpenGLWindow(QWindow *parent, QOpenGLContext * shareContext)
    : QWindow(parent)
    , isUpdatePending(false)
    , isBufferBeingSwapped(false)
    , isAnimating(false)
    , paint_device_gl(0)
    , context_gl(0)
{
    this->shared_context = shareContext;

    setSurfaceType(QWindow::OpenGLSurface);

    fps_elapsed_timer.start();
}

OpenGLWindow::~OpenGLWindow()
{
    delete paint_device_gl;
}

QPointF OpenGLWindow::coordQttoGL(QPointF coord)
{
    QPointF GLPoint;
    GLPoint.setX((coord.x()/(float) width())*2.0-1.0);
    GLPoint.setY((1.0 - coord.y()/(float) height())*2.0-1.0);
    return GLPoint;
}

void OpenGLWindow::glRect(Matrix<GLfloat> * gl_rect, QRect * qt_rect)
{
    int x,y,w,h;
    float xf,yf,wf,hf;
    qt_rect->getRect(&x, &y, &w, &h);

    xf = (x / (float) width()) * 2.0 - 1.0;
    yf = (1.0 - (y + h)/ (float) height()) * 2.0 - 1.0;
    wf = (w / (float) width()) * 2.0;
    hf = (h / (float) height()) * 2.0;

    (*gl_rect)[0] = xf;
    (*gl_rect)[1] = yf;
    (*gl_rect)[2] = xf + wf;
    (*gl_rect)[3] = yf;
    (*gl_rect)[4] = xf + wf;
    (*gl_rect)[5] = yf + hf;
    (*gl_rect)[6] = xf;
    (*gl_rect)[7] = yf + hf;
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
    fps = 1.0 / ((float) fps_elapsed_timer.nsecsElapsed() * 1.0e-9);
    fps_elapsed_timer.restart();
}

double OpenGLWindow::getFps()
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
        context_gl = new QOpenGLContext(); // Maybe not set pareent here
        context_gl->setFormat(requestedFormat());
        if (shared_context != 0)
        {
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

        swap_thread = new QThread;
        swap_surface = new OpenGLWorker();
        swap_surface->setGLContext(context_gl);
        swap_surface->setRenderSurface(this);

        swap_surface->moveToThread(swap_thread);
        connect(swap_thread, SIGNAL(started()), swap_surface, SLOT(process()));
        connect(swap_surface, SIGNAL(finished()), swap_thread, SLOT(quit()));
        connect(swap_surface, SIGNAL(finished()), this, SLOT(setSwapState()));
    }

    context_gl->makeCurrent(this);

    if (needsInitialize)
    {
        initializeOpenGLFunctions();
        if (!paint_device_gl) paint_device_gl = new QOpenGLPaintDevice;
        initialize();
    }
}

void OpenGLWindow::setSwapState()
{
    isBufferBeingSwapped = false;
}

void OpenGLWindow::renderNow()
{
    if (!isExposed()) return;
    if (isBufferBeingSwapped)
    {
        if (isAnimating) renderLater();
        return;
    }
    else
    {
        preInitialize();

        context_gl->makeCurrent(this);

        render();

        context_gl->doneCurrent();
        isBufferBeingSwapped = true;

        context_gl->moveToThread(swap_thread);
        swap_thread->start();
        setFps();
    }
    if (isAnimating) renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    isAnimating = animating;

    if (isAnimating)
        renderLater();
}

void OpenGLWindow::startAnimating()
{
    setAnimating(true);
}

void OpenGLWindow::stopAnimating()
{
    setAnimating(false);
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
