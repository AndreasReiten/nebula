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
        void setReducedPixels(MiniArray<float> * reduced_pixels);
        void setOpenCLContext(cl_device * device, cl_context * context, cl_command_queue * queue);
        void setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl);

    public slots:
        void killProcess();
        void setReduceThresholdLow(float * value);
        void setReduceThresholdHigh(float * value);
        void setProjectThresholdLow(float * value);
        void setProjectThresholdHigh(float * value);

    signals:
        void finished();
        void abort();
        void writeLog(QString err);
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
        void repaintImageWidget();

    protected:
        // Related to the runtime
        bool kill_flag;
        int verbose;

        // Related to OpenCL
        cl_mem * alpha_img_clgl;
        cl_mem * beta_img_clgl;
        cl_mem * gamma_img_clgl;
        cl_mem * tsf_img_clgl;
        cl_device * device;
        cl_context * context;
        cl_command_queue * queue;
        cl_int err;
        cl_program program;
        bool isCLInitialized;

        // Related to Voxelize
        int brick_inner_dimension;
        int brick_outer_dimension;
        float suggested_search_radius_low;
        float suggested_search_radius_high;
        float suggested_q;

        // Related to file treatment
        float * threshold_reduce_low;
        float * threshold_reduce_high;
        float * threshold_project_low;
        float * threshold_project_high;
        QStringList * file_paths;
        QList<PilatusFile> * files;
        QList<PilatusFile> * background_files;
        MiniArray<float> * reduced_pixels;
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
        //~void setImageRenderWidget(ImageRenderGLWidget * imageRenderWidget);

    signals:
        //~void testSignal(int value);

    public slots:
        void process();
        void initializeCLKernel();

    private:
        // Related to OpenCL
        cl_kernel projection_kernel;

        // Related to the GLWidgets
        //~ImageRenderGLWidget * imageRenderWidget;
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
