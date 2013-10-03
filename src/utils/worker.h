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

#include <QtGlobal>


/* GL and CL */
#ifdef Q_OS_WIN
    #define GLEW_STATIC
#endif
//#include <GL/glew.h>
#include <CL/opencl.h>
//#include <CL/cl_gl.h>

/* QT */
#include <QCoreApplication>
//#include <QMouseEvent>
#include <QTimer>
#include <QElapsedTimer>
//#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>
//#include <QGLWidget>

//#ifdef Q_OS_WIN
//    #include <windows.h>
//#elif defined Q_OS_LINUX
//    #include <GL/glx.h>
//#endif

/* Project files */
#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "fileformat.h"
#include "imagerender.h"
#include "searchnode.h"
#include "bricknode.h"
#include "sparsevoxelocttree.h"

class BaseWorker : public QObject
{
    Q_OBJECT

    public:
        BaseWorker();
        ~BaseWorker();

        void setFilePaths(QStringList * file_paths);
        void setQSpaceInfo(float * suggested_search_radius_low, float * suggested_search_radius_high, float * suggested_q);
        void setFiles(QList<PilatusFile> * files);
        void setReducedPixels(MiniArray<float> * reduced_pixels);
        void setOpenCLContext(cl_device * device, cl_context * context, cl_command_queue * queue);
        void setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl);
        void setSVOFile(SparseVoxelOcttree * svo);

    public slots:
        void killProcess();
        void setReduceThresholdLow(float * value);
        void setReduceThresholdHigh(float * value);
        void setProjectThresholdLow(float * value);
        void setProjectThresholdHigh(float * value);

    signals:
        void finished();
        void abort();
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
        void aquireSharedBuffers();
        void releaseSharedBuffers();

    protected:
        // Related to the runtime
        bool kill_flag;
        int verbosity;
        void writeLog(QString str);

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
        SparseVoxelOcttree * svo;
        float * suggested_search_radius_low;
        float * suggested_search_radius_high;
        float * suggested_q;

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

    private slots:
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

    private slots:
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

    signals:
        void changedImageWidth(int value);
        void changedImageHeight(int value);

    public slots:
        void initializeCLKernel();

    private slots:
        void process();

    protected:
        // Related to OpenCL
        cl_kernel project_kernel;
};

class DisplayFileWorker: public ProjectFileWorker
{
    Q_OBJECT

    public:
        DisplayFileWorker();
        ~DisplayFileWorker();
        void setDisplayFile(int value);

    private slots:
        void process();

    private:
        int display_file;
        Matrix<float> test_background;
};


class VoxelizeWorker : public BaseWorker
{
    Q_OBJECT

    public:
        VoxelizeWorker();
        ~VoxelizeWorker();
        void setResources(MiniArray<unsigned int> * gpuIndices, MiniArray<unsigned int> * gpuBricks, MiniArray<float> * gpuBrickPool);

    public slots:
        void process();
        void initializeCLKernel();

    protected:
        cl_kernel voxelize_kernel;

        unsigned int getOctIndex(unsigned int msdFlag, unsigned int dataFlag, unsigned int child);
        unsigned int getOctBrick(unsigned int poolX, unsigned int poolY, unsigned int poolZ);
};

class AllInOneWorker : public ProjectFileWorker
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
