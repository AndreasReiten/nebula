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
#include <QOpenGLFramebufferObject>
#include <QGLShaderProgram>


#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "sparsevoxelocttree.h"
#include "openglwindow.h"

#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QScreen>
#include <QPainter>

#include <QtCore/qmath.h>


class VolumeRenderWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    VolumeRenderWindow();
    ~VolumeRenderWindow();

protected:
    void initialize();
    void render(QPainter *painter);

private:
    GLuint loadShader(GLenum type, const char *source);

    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;

    QOpenGLShaderProgram *m_program;
    int m_frame;
};
#endif
