#ifndef VOLUMERENDER_H
#define VOLUMERENDER_H

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>


#include <QtGlobal>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMouseEvent>
//#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
//#include <QGLShaderProgram>


#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "sparsevoxelocttree.h"
#include "openglwindow.h"
#include "sharedcontext.h"

#include <QMatrix4x4>

#include <QScreen>
#include <QPainter>

#include <QtCore/qmath.h>


class VolumeRenderWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    VolumeRenderWindow();
    ~VolumeRenderWindow();

    void setSharedWindow(SharedContextWindow * window);

protected:
    void initialize();
    void render(QPainter *painter);
    void mouseMoveEvent(QMouseEvent* ev);
    void wheelEvent(QWheelEvent* ev);

private:
    SharedContextWindow * shared_window;

    int m_frame;
};
#endif
