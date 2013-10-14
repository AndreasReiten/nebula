#include "volumerender.h"

VolumeRenderWindow::VolumeRenderWindow()
    : m_frame(0)
{
}

VolumeRenderWindow::~VolumeRenderWindow()
{
    std::cout << Q_FUNC_INFO << std::endl;
}

void VolumeRenderWindow::mouseMoveEvent(QMouseEvent* ev)
{
//    std::cout << ev->x() << " " << ev->y() << std::endl;
}

void VolumeRenderWindow::wheelEvent(QWheelEvent* ev)
{
//    std::cout << ev->delta() << std::endl;
}



void VolumeRenderWindow::initialize()
{
}

void VolumeRenderWindow::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
    shared_context = window->getGLContext();
}

void VolumeRenderWindow::render(QPainter *painter)
{
    painter->drawText(50, 150, QString("Qt is a cross-platform application and UI framework for developers using C++ or QML, a CSS & JavaScript like language. Qt Creator is the supporting Qt IDE."));

    painter->drawText(50, 250, QString("Qt is a cross-platform application and UI framework for developers using C++ or QML, a CSS & JavaScript like language. Qt Creator is the supporting Qt IDE."));


    painter->beginNativePainting();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

//    glClear(GL_COLOR_BUFFER_BIT);

    shared_window->m_program->bind();

    QMatrix4x4 matrix;
    matrix.perspective(60, 4.0/3.0, 0.1, 100.0);
    matrix.translate(0, 0, -2);
    matrix.rotate(20.0f * m_frame / screen()->refreshRate(), 0, 1, 0);

    shared_window->m_program->setUniformValue(shared_window->m_matrixUniform, matrix);

    GLfloat vertices[] = {
        0.0f, 0.707f,
        -0.5f, -0.5f,
        0.5f, -0.5f
    };

    GLfloat colors[] = {
        1.0f, 0.0f, 0.0f, 1.0,
        0.0f, 1.0f, 0.0f, 0.0,
        0.0f, 0.0f, 1.0f, 0.5
    };

    glVertexAttribPointer(shared_window->m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(shared_window->m_colAttr, 4, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    shared_window->m_program->release();

    ++m_frame;

    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);

    painter->endNativePainting();

    painter->drawText(50, 50, QString("Qt is a cross-platform application and UI framework for developers using C++ or QML, a CSS & JavaScript like language. Qt Creator is the supporting Qt IDE."));

    painter->drawText(50, 100, QString::number(getFps()));

}
