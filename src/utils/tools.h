#ifndef TOOLS_H
#define TOOLS_H


/* QT */
#include <QtGlobal>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QString>
/* GL and CL*/
//#ifdef Q_OS_WIN
//    #define GLEW_STATIC
//#endif
//#include <GL/glew.h>
#include <CL/opencl.h>

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>

//#include "miniarray.h"
#include "matrix.h"

QString openResource(const char * path);
QByteArray openFile(const char * path);
//GLuint create_shader(const char* resource, GLenum type);s

QString timeString(size_t ms);
const char * cl_error_cstring(cl_int error);
//const char * gl_framebuffer_error_cstring(GLint error);
//void glGetErrorMessage(const char * context);
//void screenshot(int w, int h, const char* path);
//void setVbo(GLuint * vbo, float * buf, size_t length);
//void init_tsf(int color_style, int alpha_style, TsfMatrix<double> * transfer_function);
void writeToLogAndPrint(QString text, QString file, bool append);
#endif
