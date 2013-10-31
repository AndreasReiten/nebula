#include "openglwindow.h"

OpenGLWorker::OpenGLWorker(QObject *parent)
    : QObject(parent)
    , isInitialized(false)
//    , isRenderSurfaceActive(false)
    , paint_device_gl(0)
{
    fps_elapsed_timer.start();
}

OpenGLWorker::~OpenGLWorker()
{
    qDebug();
    if (paint_device_gl) delete paint_device_gl;
}

void OpenGLWorker::mouseMoveEvent(QMouseEvent* ev)
{
    Q_UNUSED(ev);
}

void OpenGLWorker::wheelEvent(QWheelEvent* ev)
{
    Q_UNUSED(ev);
}

void OpenGLWorker::resizeEvent(QResizeEvent * ev)
{
    Q_UNUSED(ev);

    if (paint_device_gl) paint_device_gl->setSize(render_surface->size());
}

void OpenGLWorker::process()
{
//    if (isRenderSurfaceActive)
//    {
        context_gl->makeCurrent(render_surface);

        if (!isInitialized)
        {
            initializeOpenGLFunctions();
            if (!paint_device_gl) paint_device_gl = new QOpenGLPaintDevice;
            initialize();
            isInitialized = true;
        }
        else
        {
            QPainter painter(paint_device_gl);
            render(&painter);
        }
        context_gl->swapBuffers(render_surface);
        context_gl->doneCurrent();
        context_gl->moveToThread(qApp->thread());

        setFps();

        emit finished();
//    }
}

void OpenGLWorker::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void OpenGLWorker::initialize()
{

}

//void OpenGLWorker::render()
//{

//}

void OpenGLWorker::setGLContext(QOpenGLContext *context)
{
    context_gl = context;
}

void OpenGLWorker::setRenderSurface(QWindow *surface)
{
    render_surface = surface;
//    isRenderSurfaceActive = true;
}

//void OpenGLWorker::stopRendering()
//{
//    isRenderSurfaceActive = false;
//}


void OpenGLWorker::getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform)
{
    Matrix<float> pos_3d_matrix;
    Matrix<float> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = transform->toFloat() * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0]/pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1]/pos_2d_matrix[3];
}

QPointF OpenGLWorker::coordQttoGL(QPointF coord)
{
    QPointF GLPoint;
    GLPoint.setX((coord.x()/(float) render_surface->width())*2.0-1.0);
    GLPoint.setY((1.0 - coord.y()/(float) render_surface->height())*2.0-1.0);
    return GLPoint;
}

void OpenGLWorker::glRect(Matrix<GLfloat> * gl_rect, QRect * qt_rect)
{
    int x,y,w,h;
    float xf,yf,wf,hf;
    qt_rect->getRect(&x, &y, &w, &h);

    xf = (x / (float) render_surface->width()) * 2.0 - 1.0;
    yf = (1.0 - (y + h)/ (float) render_surface->height()) * 2.0 - 1.0;
    wf = (w / (float) render_surface->width()) * 2.0;
    hf = (h / (float) render_surface->height()) * 2.0;

    (*gl_rect)[0] = xf;
    (*gl_rect)[1] = yf;
    (*gl_rect)[2] = xf + wf;
    (*gl_rect)[3] = yf;
    (*gl_rect)[4] = xf + wf;
    (*gl_rect)[5] = yf + hf;
    (*gl_rect)[6] = xf;
    (*gl_rect)[7] = yf + hf;
}

void OpenGLWorker::setVbo(GLuint vbo, float * buf, size_t length, GLenum usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*length, buf, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLWorker::setOpenCLContext(OpenCLContext * context)
{
    this->context_cl = context;
}

QOpenGLContext * OpenGLWindow::getGLContext()
{
    return context_gl;
}

OpenCLContext * OpenGLWindow::getCLContext()
{
    return context_cl;
}

OpenGLWindow::OpenGLWindow(QWindow *parent, QOpenGLContext * shareContext)
    : QWindow(parent)
    , isUpdatePending(false)
    , isBufferBeingSwapped(false)
    , isAnimating(false)
    , context_gl(0)
    , gl_surface(0)
{
    this->shared_context = shareContext;

    setSurfaceType(QWindow::OpenGLSurface);
}

void OpenGLWindow::setOpenCLContext(OpenCLContext * context)
{
    this->context_cl = context;
}

OpenGLWindow::~OpenGLWindow()
{
//    emit destroy();
    qDebug();
}

void OpenGLWindow::mouseMoveEvent(QMouseEvent* ev)
{
    emit mouseMoveEventCaught(ev);
}

void OpenGLWindow::wheelEvent(QWheelEvent* ev)
{
    emit wheelEventCaught(ev);
}

void OpenGLWindow::resizeEvent(QResizeEvent * ev)
{
    emit resizeEventCaught(ev);
}

void OpenGLWindow::setOpenGLWorker(OpenGLWorker * worker)
{
    gl_surface = worker;
    worker->setRenderSurface(this);
}

void OpenGLWorker::setFps()
{
    fps = 1.0 / ((float) fps_elapsed_timer.nsecsElapsed() * 1.0e-9);
    fps_elapsed_timer.restart();
}

double OpenGLWorker::getFps()
{
    return fps;
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

void OpenGLWindow::initialize()
{

}

void OpenGLWindow::preInitialize()
{
//    bool needsInitialize = false;

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

        // Intialize context depndtent stuff
        context_gl->makeCurrent(this);
        initialize();
        context_gl->doneCurrent();

//        needsInitialize = true;
        if (gl_surface)
        {
            // Set up worker thread
            worker_thread = new QThread;
    //        gl_surface = new OpenGLWorker();
            gl_surface->setGLContext(context_gl);
    //        gl_surface->setRenderSurface(this);

            gl_surface->moveToThread(worker_thread);
            connect(worker_thread, SIGNAL(started()), gl_surface, SLOT(process()));
            connect(gl_surface, SIGNAL(finished()), worker_thread, SLOT(quit()));
            connect(gl_surface, SIGNAL(finished()), this, SLOT(setSwapState()));

            // Transfering mouse events
            connect(this, SIGNAL(mouseMoveEventCaught(QMouseEvent*)), gl_surface, SLOT(mouseMoveEvent(QMouseEvent*)), Qt::BlockingQueuedConnection);
            connect(this, SIGNAL(resizeEventCaught(QResizeEvent*)), gl_surface, SLOT(resizeEvent(QResizeEvent*)), Qt::BlockingQueuedConnection);
            connect(this, SIGNAL(wheelEventCaught(QWheelEvent*)), gl_surface, SLOT(wheelEvent(QWheelEvent*)), Qt::BlockingQueuedConnection);

            // Ensure safe destruction
//            connect(this, SIGNAL(destroyed()), worker_thread, SLOT(quit()));
        }
    }
    context_gl->makeCurrent(this);
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

        if (gl_surface)
        {
            context_gl->doneCurrent();
            isBufferBeingSwapped = true;

            context_gl->moveToThread(worker_thread);
            worker_thread->start();
        }
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


