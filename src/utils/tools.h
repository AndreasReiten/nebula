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
#include <CL/opencl.h>

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>

#include "matrix.h"

QString openResource(const char * path);
//QByteArray openFile(const char * path);

QString timeString(size_t ms);

void writeToLogAndPrint(QString text, QString file, bool append);
#endif
