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
#include <QCoreApplication>

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
#include <QScriptEngine>
#include <QPlainTextEdit>
#include <QOpenGLContext>

/* Project files */
#include "../../lib/qxlib/qxlib.h"

class BaseWorker : public QObject
{
    Q_OBJECT

    public:
        BaseWorker();
        ~BaseWorker();

        void setFilePaths(QStringList * file_paths);
//        void setQSpaceInfo(float * suggested_search_radius_low, float * suggested_search_radius_high, float * suggested_q);
        void setFiles(QList<DetectorFile> * files);
        void setReducedPixels(Matrix<float> * reduced_pixels);
        void setOpenCLContext(OpenCLContext * context);
        void setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl);
        void setSVOFile(SparseVoxelOcttree * svo);

    signals:
        void finished();
        void abort();
        void changedMessageString(QString str);
        void changedGenericProgress(int value);
        void changedFormatGenericProgress(QString str);
        void changedTabWidget(int value);
        void changedFile(QString path);
        void popup(QString title, QString text);
        void qSpaceInfoChanged(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q);

    public slots:
        void killProcess();
        void setActiveAngle(int value);
        void setOffsetOmega(double value);
        void setOffsetKappa(double value);
        void setOffsetPhi(double value);
        void setReduceThresholdLow(double value);
        void setReduceThresholdHigh(double value);
        void setProjectThresholdLow(double value);
        void setProjectThresholdHigh(double value);
        void setQSpaceInfo(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q);

    protected:
        // Runtime
        bool kill_flag;

        // OpenCL
        OpenCLContext * context_cl;
        cl_mem * alpha_img_clgl;
        cl_mem * beta_img_clgl;
        cl_mem * gamma_img_clgl;
        cl_mem * tsf_img_clgl;
        cl_int err;
        cl_program program;
        bool isCLInitialized;

        // Voxelize
        SparseVoxelOcttree * svo;
        float  suggested_search_radius_low;
        float  suggested_search_radius_high;
        float  suggested_q;

        // File treatment
        int active_angle;
        float threshold_reduce_low;
        float threshold_reduce_high;
        float threshold_project_low;
        float threshold_project_high;
        QStringList * file_paths;
        QList<DetectorFile> * files;
        Matrix<float> * reduced_pixels;
        
        double offset_omega;
        double offset_kappa;
        double offset_phi;
        
        size_t GLOBAL_VRAM_ALLOC_MAX;
};

class ReadScriptWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ReadScriptWorker();
        ~ReadScriptWorker();

        void setScriptEngine(QScriptEngine * engine);
        void setInput(QPlainTextEdit * widget);

    signals:
        void maxFramesChanged(int value);

    private slots:
        void process();

    private:
        QScriptEngine * engine;
        QPlainTextEdit * inputWidget;
};


class SetFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        SetFileWorker();
        ~SetFileWorker();

    private slots:
        void process();
};


class ReadFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ReadFileWorker();
        ~ReadFileWorker();

    private slots:
//        void test();
        void process();
};


class ProjectFileWorker : public BaseWorker
{
    Q_OBJECT

    public:
        ProjectFileWorker();
        ~ProjectFileWorker();

    signals:
        void changedImageSize(int w, int h);
        void testToWindow();
        void testToMain();

    public slots:
        void initializeCLKernel();

    private slots:
        void process();

    protected:
        // Related to OpenCL
        cl_kernel project_kernel;
};


class VoxelizeWorker : public BaseWorker
{
    Q_OBJECT

    public:
        VoxelizeWorker();
        ~VoxelizeWorker();

    public slots:
        void process();
        void initializeCLKernel();

    protected:
        cl_kernel voxelize_kernel;
        cl_kernel fill_kernel;
        
        unsigned int getOctIndex(unsigned int msdFlag, unsigned int dataFlag, unsigned int child);
        unsigned int getOctBrick(unsigned int poolX, unsigned int poolY, unsigned int poolZ);
};

class MultiWorker : public ProjectFileWorker
{
    Q_OBJECT

    public:
        MultiWorker();
        ~MultiWorker();

    public slots:
        void process();
};

#endif
