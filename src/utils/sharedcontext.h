#ifndef SHAREDCONTEXT_H
#define SHAREDCONTEXT_H

/* Useful C++ libs */
#include <iostream>
#include <sstream>

//#include <QMatrix4x4>

#include <QtGlobal>


/* GL and CL*/
#include <CL/opencl.h>


/* QT */
//#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>

#include "miniarray.h"
#include "tools.h"
#include "openglwindow.h"


class SharedContextWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    explicit SharedContextWindow();
    ~SharedContextWindow();

    GLint std_2d_fragpos;
    GLint std_2d_texpos;
    GLint std_2d_texture;
    QOpenGLShaderProgram *std_2d_tex_program;

    GLint std_3d_texpos;
    GLint std_3d_color;
    GLint std_3d_transform;
    GLint std_3d_fragpos;
    QOpenGLShaderProgram *std_3d_color_program;

    GLint std_blend_fragpos;
    GLint std_blend_texpos;
    GLint std_blend_tex_a;
    GLint std_blend_tex_b;
    GLint std_blend_method;
    QOpenGLShaderProgram *std_blend_program;

signals:
    void changedMessageString(QString str);

protected:
    void initialize();

private:
    void setMessageString(QString str);

    GLuint loadShader(GLenum type, const char *source);

    cl_int err;
};

#endif
