#include "openglwindow.h"

#include <QCoreApplication>
#include <QOpenGLFramebufferObject>
#include <QDebug>

OpenGLWorker::OpenGLWorker(QObject *parent)
    : QObject(parent)
    , paint_device_gl(0)
    , isInitialized(false)
    , isThreaded(false)
{
    fps_elapsed_timer.start();
}

OpenGLWorker::~OpenGLWorker()
{
    if (paint_device_gl) delete paint_device_gl;
}

void OpenGLWorker::metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
}

void OpenGLWorker::metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
}

void OpenGLWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
}

void OpenGLWorker::wheelEvent(QWheelEvent* ev)
{
    Q_UNUSED(ev);
}

//void OpenGLWorker::keyPressEvent(QKeyEvent ev)
//{
//    Q_UNUSED(ev);
//}

//void OpenGLWorker::keyReleaseEvent(QKeyEvent * ev)
//{
//    Q_UNUSED(ev);
//}

void OpenGLWorker::resizeEvent(QResizeEvent * ev)
{
    Q_UNUSED(ev);

    if (paint_device_gl) paint_device_gl->setSize(render_surface->size());
}

void OpenGLWorker::process()
{
    context_gl->makeCurrent(render_surface);

    if (!isInitialized)
    {
        initializeOpenGLFunctions();
        if (!paint_device_gl) paint_device_gl = new QOpenGLPaintDevice;
        paint_device_gl->setSize(render_surface->size());
        
        initialize();
        isInitialized = true;

//        qDebug() << "now it is initialized";
    }
    else
    {
        QPainter painter(paint_device_gl);
        render(&painter);
    }
    context_gl->swapBuffers(render_surface);
            
    setFps();
    emit finished();
}

void OpenGLWorker::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void OpenGLWorker::initialize()
{
    
}

void OpenGLWorker::setOpenGLContext(QOpenGLContext *context)
{
    context_gl = context;
}

void OpenGLWorker::setRenderSurface(QWindow *surface)
{
    render_surface = surface;
}




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

void OpenGLWorker::getPosition2D(double * pos_2d, double * pos_3d, Matrix<double> * transform)
{
    Matrix<double> pos_3d_matrix;
    Matrix<double> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = *transform * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0]/pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1]/pos_2d_matrix[3];
}

QPointF OpenGLWorker::posGLtoQt(QPointF coord)
{
    QPointF QtPoint;

    QtPoint.setX(0.5 * (float)render_surface->width() * (coord.x()+1.0) - 1.0);
    QtPoint.setY(0.5 * (float)render_surface->height() * (1.0-coord.y()) - 1.0);

    return QtPoint;
}

QPointF OpenGLWorker::posQttoGL(QPointF coord)
{
    QPointF GLPoint;
    GLPoint.setX((coord.x()+1.0)/(float) (render_surface->width())*2.0-1.0);
    GLPoint.setY((1.0 - (coord.y()+1.0)/(float) render_surface->height())*2.0-1.0);
    return GLPoint;
}

Matrix<GLfloat> OpenGLWorker::glRect(QRectF & qt_rect)
{
    Matrix<GLfloat> gl_rect(1,8);

    qreal x,y,w,h;
    qreal xf,yf,wf,hf;
    qt_rect = qt_rect.normalized();
    qt_rect.getRect(&x, &y, &w, &h);

    xf = (x / (qreal) render_surface->width()) * 2.0 - 1.0;
    yf = 1.0 - (y + h)/ (qreal) render_surface->height() * 2.0;
    wf = (w / (qreal) render_surface->width()) * 2.0;
    hf = (h / (qreal) render_surface->height()) * 2.0;

    gl_rect[0] = (GLfloat) xf;
    gl_rect[1] = (GLfloat) yf;
    gl_rect[2] = (GLfloat) xf + wf;
    gl_rect[3] = (GLfloat) yf;
    gl_rect[4] = (GLfloat) xf + wf;
    gl_rect[5] = (GLfloat) yf + hf;
    gl_rect[6] = (GLfloat) xf;
    gl_rect[7] = (GLfloat) yf + hf;

    return gl_rect;
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
    , context_gl(0)
    , isWorkerBusy(false)
    , isThreaded(false)
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
    if (isThreaded)
    {
        worker_thread->quit();
        worker_thread->wait(1000);
    }
}

void OpenGLWindow::mouseReleaseEvent(QMouseEvent* ev)
{
    emit metaMouseReleaseEventCaught((int)ev->x(), (int)ev->y(), (int)(ev->button() & Qt::LeftButton), (int)(ev->button() & Qt::MidButton), (int)(ev->button() & Qt::RightButton), (int)(ev->modifiers() & Qt::ControlModifier), (int)(ev->modifiers() & Qt::ShiftModifier));
}

void OpenGLWindow::mousePressEvent(QMouseEvent* ev)
{
    emit metaMousePressEventCaught((int)ev->x(), (int)ev->y(), (int)(ev->buttons() & Qt::LeftButton), (int)(ev->buttons() & Qt::MidButton), (int)(ev->buttons() & Qt::RightButton), (int)(ev->modifiers() & Qt::ControlModifier), (int)(ev->modifiers() & Qt::ShiftModifier));
}

void OpenGLWindow::mouseMoveEvent(QMouseEvent* ev)
{
    emit metaMouseMoveEventCaught((int)ev->x(), (int)ev->y(), (int)(ev->buttons() & Qt::LeftButton), (int)(ev->buttons() & Qt::MidButton), (int)(ev->buttons() & Qt::RightButton), (int)(ev->modifiers() & Qt::ControlModifier), (int)(ev->modifiers() & Qt::ShiftModifier));
}

void OpenGLWindow::setMultiThreading(bool value)
{
    isThreaded = value;
}

void OpenGLWorker::setMultiThreading(bool value)
{
    isThreaded = value;
}

void OpenGLWindow::wheelEvent(QWheelEvent* ev)
{
    emit wheelEventCaught(ev);
}

void OpenGLWindow::resizeEvent(QResizeEvent * ev)
{
    emit resizeEventCaught(ev);
    renderLater();
}


void OpenGLWindow::keyPressEvent(QKeyEvent * ev)
{
    Q_UNUSED(ev);
//    qDebug() << ev->key();
    
//    emit keyPressEventCaught(*ev);
}

void OpenGLWindow::keyReleaseEvent(QKeyEvent * ev)
{
    Q_UNUSED(ev);
//    emit keyReleaseEventCaught(ev);
}

void OpenGLWindow::initializeGLContext()
{
    if (!context_gl)
    {
        context_gl = new QOpenGLContext();
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

        if(isThreaded)
        {
            worker_thread = new QThread;
            context_gl->moveToThread(worker_thread);
        }
    }
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
    QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type()) 
    {
    case QEvent::UpdateRequest:
        renderNow();
        return true;
//    case QEvent::Leave:
//        qDebug() << "leave";
        
//        return true;
//    case QEvent::Enter:
////        emit setFocus();
//        qDebug() << "enter";
//        return true;
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

void OpenGLWindow::initializeWorker()
{
    /*
     * Call after the worker has been set
     * */

}

void OpenGLWindow::setSwapState()
{
    isWorkerBusy = false;
}

void OpenGLWindow::renderNow()
{

}

const char * gl_error_cstring(GLenum err)
{
    switch (err) {
        case GL_NO_ERROR:                       return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
        case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
        case GL_STACK_UNDERFLOW:                return "GL_STACK_UNDERFLOW";
        case GL_STACK_OVERFLOW:                 return "GL_STACK_OVERFLOW";
        default: return "Unknown";
    }
}
