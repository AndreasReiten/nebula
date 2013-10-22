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
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>

/* Project files */
#include "tools.h"
#include "miniarray.h"
#include "matrix.h"
#include "fileformat.h"
#include "imagerender.h"
#include "searchnode.h"
#include "bricknode.h"
#include "sparsevoxelocttree.h"
#include "contextcl.h"

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
            void setOpenCLContext(ContextCL * context);
        void setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl);
        void setSVOFile(SparseVoxelOcttree * svo);
    signals:
        void finished();
        void abort();
        void changedMessageString(QString str);
        void changedGenericProgress(int value);
        void changedFormatGenericProgress(QString str);
        void changedTabWidget(int value);
        void repaintImageWidget();
        void aquireSharedBuffers();
        void releaseSharedBuffers();


    public slots:
        void killProcess();
        void setReduceThresholdLow(float * value);
        void setReduceThresholdHigh(float * value);
        void setProjectThresholdLow(float * value);
        void setProjectThresholdHigh(float * value);

    protected:
        // Runtime
        bool kill_flag;

        // OpenCL
        ContextCL * context_cl;
        cl_mem * alpha_img_clgl;
        cl_mem * beta_img_clgl;
        cl_mem * gamma_img_clgl;
        cl_mem * tsf_img_clgl;
        cl_int err;
        cl_program program;
        bool isCLInitialized;

        // Voxelize
        SparseVoxelOcttree * svo;
        float * suggested_search_radius_low;
        float * suggested_search_radius_high;
        float * suggested_q;

        // File treatment
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
