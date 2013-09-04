#ifndef WORKER_H
#define WORKER_H

/* C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <limits>

/* GL and CL */
#ifdef _WIN32
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

/* QT */
#include <QCoreApplication>
#include <QMouseEvent>
#include <QGLWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>

/* Project files */
#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "file_formats.h"
#include "imagerender.h"

class BaseWorker : public QObject
{
    Q_OBJECT

    public:
        BaseWorker();
        ~BaseWorker();

        void setFilePaths(QStringList * file_paths);
        void setBrickInfo(int brick_inner_dimension, int brick_outer_dimension);
        void setFiles(QList<PilatusFile> * files);

    public slots:
        void killProcess();

    signals:
        void finished();
        void abort();
        void error(QString err);
        void changedMessageString(QString str);
        void changedGenericProgress(int value);
        void changedFormatGenericProgress(QString str);
        void enableSetFileButton(bool value);
        void enableReadFileButton(bool value);
        void enableProjectFileButton(bool value);
        void enableVoxelizeButton(bool value);
        void enableAllInOneButton(bool value);
        void showGenericProgressBar(bool value);
        void changedTabWidget(int value);

    protected:
        // Related to the runtime
        bool kill_flag;

        // Related to the GLWidgets
        ImageRenderGLWidget * imageRenderWidget;

        // Related to OpenCL
        cl_context * context;
        cl_command_queue * queue;
        cl_kernel * projection_kernel;

        // Related to Voxelize
        int brick_inner_dimension;
        int brick_outer_dimension;
        float suggested_search_radius_low;
        float suggested_search_radius_high;
        float suggested_q;

        MiniArray<float> * projected_data;

        // Related to file treatment
        QStringList * file_paths;
        QList<PilatusFile> * files;
        QList<PilatusFile> * background_files;
        Matrix<float> test_background;

    private:
        // add your variables here
};


class SetFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        SetFileWorker();
        ~SetFileWorker();

    public slots:
        void process();

    private:
        // add your variables here
};


class ReadFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ReadFileWorker();
        ~ReadFileWorker();

    public slots:
        void process();

    private:
        // add your variables here
};


class ProjectFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ProjectFileWorker();
        ~ProjectFileWorker();

    public slots:
        void process();

    private:
        // add your variables here
};


class VoxelizeWorker : public BaseWorker
{
    Q_OBJECT

    public:
        VoxelizeWorker();
        ~VoxelizeWorker();

    public slots:
        void process();

    private:
        // add your variables here
};

class AllInOneWorker : public BaseWorker
{
    Q_OBJECT

    public:
        AllInOneWorker();
        ~AllInOneWorker();

    public slots:
        void process();

    private:
        // add your variables here
};

#endif
