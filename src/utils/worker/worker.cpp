#include "worker.h"

// IN the future this whole section could be moved to QXlib and imagepreview

/* C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <limits>

//#include <QtGlobal>
#include <QCoreApplication>

#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>


#include <QOpenGLContext>

//static const size_t REDUCED_PIXELS_MAX_BYTES = 100e6; // Max amount of reduced data
static const size_t BRICK_POOL_SOFT_MAX_BYTES = 0.7e9; // Effectively limited by the max allocation size for global memory if the pool resides on the GPU during pool construction. 3D image can be used with OpenCL 1.2, allowing you to use the entire VRAM.
static const size_t MAX_POINTS_PER_CLUSTER = 10000000;
static const size_t MAX_NODES_PER_CLUSTER = 20000;


// ASCII from http://patorjk.com/software/taag/#p=display&c=c&f=Trek&t=Base%20Class
/***
 *        dBBBBb dBBBBBb  .dBBBBP   dBBBP     dBBBP  dBP dBBBBBb  .dBBBBP.dBBBBP
 *           dBP      BB  BP                                  BB  BP     BP
 *       dBBBK'   dBP BB  `BBBBb  dBBP      dBP    dBP    dBP BB  `BBBBb `BBBBb
 *      dB' db   dBP  BB     dBP dBP       dBP    dBP    dBP  BB     dBP    dBP
 *     dBBBBP'  dBBBBBBBdBBBBP' dBBBBP    dBBBBP dBBBBP dBBBBBBBdBBBBP'dBBBBP'
 *
 */
BaseWorker::BaseWorker()
    : isCLInitialized(false)
{
    this->isCLInitialized = false;
    resolveOpenCLFunctions();
}

BaseWorker::~BaseWorker()
{
    
}

void BaseWorker::resolveOpenCLFunctions()
{
    QLibrary myLib("OpenCL");

    QOpenCLGetPlatformIDs = (PROTOTYPE_QOpenCLGetPlatformIDs) myLib.resolve("clGetPlatformIDs");
    if (!QOpenCLGetPlatformIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceIDs = (PROTOTYPE_QOpenCLGetDeviceIDs) myLib.resolve("clGetDeviceIDs");
    if (!QOpenCLGetDeviceIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetPlatformInfo = (PROTOTYPE_QOpenCLGetPlatformInfo) myLib.resolve("clGetPlatformInfo");
    if (!QOpenCLGetPlatformInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceInfo = (PROTOTYPE_QOpenCLGetDeviceInfo) myLib.resolve("clGetDeviceInfo");
    if (!QOpenCLGetDeviceInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateProgramWithSource = (PROTOTYPE_QOpenCLCreateProgramWithSource) myLib.resolve("clCreateProgramWithSource");
    if (!QOpenCLCreateProgramWithSource) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetProgramBuildInfo = (PROTOTYPE_QOpenCLGetProgramBuildInfo) myLib.resolve("clGetProgramBuildInfo");
    if (!QOpenCLGetProgramBuildInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateContext = (PROTOTYPE_QOpenCLCreateContext) myLib.resolve("clCreateContext");
    if (!QOpenCLCreateContext) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateCommandQueue = (PROTOTYPE_QOpenCLCreateCommandQueue) myLib.resolve("clCreateCommandQueue");
    if (!QOpenCLCreateCommandQueue) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLSetKernelArg = (PROTOTYPE_QOpenCLSetKernelArg) myLib.resolve("clSetKernelArg");
    if (!QOpenCLSetKernelArg) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueNDRangeKernel = (PROTOTYPE_QOpenCLEnqueueNDRangeKernel) myLib.resolve("clEnqueueNDRangeKernel");
    if (!QOpenCLEnqueueNDRangeKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLFinish= (PROTOTYPE_QOpenCLFinish) myLib.resolve("clFinish");
    if (!QOpenCLFinish) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReleaseGLObjects = (PROTOTYPE_QOpenCLEnqueueReleaseGLObjects) myLib.resolve("clEnqueueReleaseGLObjects");
    if (!QOpenCLEnqueueReleaseGLObjects) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueAcquireGLObjects = (PROTOTYPE_QOpenCLEnqueueAcquireGLObjects) myLib.resolve("clEnqueueAcquireGLObjects");
    if (!QOpenCLEnqueueAcquireGLObjects) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateKernel = (PROTOTYPE_QOpenCLCreateKernel) myLib.resolve("clCreateKernel");
    if (!QOpenCLCreateKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadBuffer = (PROTOTYPE_QOpenCLEnqueueReadBuffer) myLib.resolve("clEnqueueReadBuffer");
    if (!QOpenCLEnqueueReadBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateBuffer = (PROTOTYPE_QOpenCLCreateBuffer) myLib.resolve("clCreateBuffer");
    if (!QOpenCLCreateBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseMemObject = (PROTOTYPE_QOpenCLReleaseMemObject) myLib.resolve("clReleaseMemObject");
    if (!QOpenCLReleaseMemObject) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateFromGLTexture2D = (PROTOTYPE_QOpenCLCreateFromGLTexture2D) myLib.resolve("clCreateFromGLTexture2D");
    if (!QOpenCLCreateFromGLTexture2D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateSampler = (PROTOTYPE_QOpenCLCreateSampler) myLib.resolve("clCreateSampler");
    if (!QOpenCLCreateSampler) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueWriteBuffer = (PROTOTYPE_QOpenCLEnqueueWriteBuffer) myLib.resolve("clEnqueueWriteBuffer");
    if (!QOpenCLEnqueueWriteBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseKernel = (PROTOTYPE_QOpenCLReleaseKernel) myLib.resolve("clReleaseKernel");
    if (!QOpenCLReleaseKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateImage2D = (PROTOTYPE_QOpenCLCreateImage2D) myLib.resolve("clCreateImage2D");
    if (!QOpenCLCreateImage2D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateImage3D = (PROTOTYPE_QOpenCLCreateImage3D) myLib.resolve("clCreateImage3D");
    if (!QOpenCLCreateImage3D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadImage = (PROTOTYPE_QOpenCLEnqueueReadImage) myLib.resolve("clEnqueueReadImage");
    if (!QOpenCLEnqueueReadImage) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseSampler = (PROTOTYPE_QOpenCLReleaseSampler) myLib.resolve("clReleaseSampler");
    if (!QOpenCLReleaseSampler) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());
}

//void BaseWorker::setOpenCLContext(OpenCLContext * context)
//{
//    this->context_cl = context;
//}

//void BaseWorker::setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl)
//{

//    this->alpha_img_clgl = alpha_img_clgl;
//    this->beta_img_clgl = beta_img_clgl;
//    this->gamma_img_clgl = gamma_img_clgl;
//    this->tsf_img_clgl = tsf_img_clgl;
//}

void BaseWorker::setSVOFile(SparseVoxelOctree * svo)
{

    this->svo = svo;
}

void BaseWorker::setOffsetOmega(double value)
{
    offset_omega = value*pi/180.0;
}
void BaseWorker::setOffsetKappa(double value)
{
    offset_kappa = value*pi/180.0;
}
void BaseWorker::setOffsetPhi(double value)
{
    offset_phi = value*pi/180.0;
}

void BaseWorker::setActiveAngle(int value)
{
    active_angle = value;
}

//void BaseWorker::setNoiseLow(double value)
//{
//    this->thld_noise_low = (float) value;
//}
//void BaseWorker::setNoiseHigh(double value)
//{
//    this->thld_noise_high = (float) value;
//}
//void BaseWorker::setThldProjectLow(double value)
//{
//    this->thld_project_low = (float) value;
//}
//void BaseWorker::setThldProjectHigh(double value)
//{
//    this->thld_project_high = (float) value;
//}

void BaseWorker::killProcess()
{

    kill_flag = true;
}

//void BaseWorker::setFilePaths(QStringList * file_paths)
//{
//    this->file_paths = file_paths;
//}

void BaseWorker::setQSpaceInfo(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q)
{

    this->suggested_search_radius_low = suggested_search_radius_low;
    this->suggested_search_radius_high = suggested_search_radius_high;
    this->suggested_q = suggested_q;
}


//void BaseWorker::setFiles(QList<DetectorFile> * files)
//{

//    this->files = files;
//}
void BaseWorker::setReducedPixels(Matrix<float> *reduced_pixels)
{

    this->reduced_pixels = reduced_pixels;
}

void BaseWorker::setSet(SeriesSet set)
{
    this->set = set;
    
//    qDebug() << set << set.begin()->begin()->selection();
}

/***
 *      .dBBBBP   dBBBP dBBBBBBP
 *      BP
 *      `BBBBb  dBBP     dBP
 *         dBP dBP      dBP
 *    dBBBBP' dBBBBP   dBP
 *
 */

//SetFileWorker::SetFileWorker()
//{
//    this->isCLInitialized = false;
//}

//SetFileWorker::~SetFileWorker()
//{

//}

//void SetFileWorker::process()
//{
//    QCoreApplication::processEvents();

//    int verbose = 0;

//    kill_flag = false;

//    if (file_paths->size() <= 0)
//    {
//        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No paths specified!");

//        emit changedMessageString(str);
//        kill_flag = true;
//    }

//    // Emit to appropriate slots
//    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Processing "+QString::number(file_paths->size())+" files...");
//    emit changedRangeGenericProcess(1, file_paths->size());
//    emit changedFormatGenericProgress(QString("Set file %v of %m (%p%)"));
//    emit changedTabWidget(0);

//    // Reset suggested values
//    suggested_q = std::numeric_limits<float>::min();
//    suggested_search_radius_low = std::numeric_limits<float>::max();
//    suggested_search_radius_high = std::numeric_limits<float>::min();

//    // Clear previous data
//    files->clear();
//    files->reserve(file_paths->size());

//    QElapsedTimer stopwatch;
//    stopwatch.start();

//    for (size_t i = 0; i < (size_t) file_paths->size(); i++)
//    {
//        // Kill process if requested
//        if (kill_flag)
//        {
//            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(file_paths->size()));
//            files->clear();

//            break;
//        }
        
//        // Update the progress bar
//        emit changedGenericProgress(i+1);

//        // Set file and get status
//        files->append(DetectorFile());
//        int STATUS_OK = files->back().set(file_paths->at(i));
//        if (STATUS_OK)
//        {
//            // Get suggestions on the minimum search radius that can safely be applied during interpolation
//            if (suggested_search_radius_low > files->back().getSearchRadiusLowSuggestion()) suggested_search_radius_low = files->back().getSearchRadiusLowSuggestion();
//            if (suggested_search_radius_high < files->back().getSearchRadiusHighSuggestion()) suggested_search_radius_high = files->back().getSearchRadiusHighSuggestion();

//            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
//            if (suggested_q < files->back().getQSuggestion()) suggested_q = files->back().getQSuggestion();
            
//            emit changedFile(files->last().getPath());
//        }
//        else
//        {
//            files->removeLast();
//            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Could not process \""+QString(file_paths->at(i))+"\"");
//        }

        
//    }
//    size_t t = stopwatch.restart();

//    if (!kill_flag)
//    {
//        emit changedMessageString(" "+QString::number(files->size())+" of "+QString::number(file_paths->size())+" files were processed successfully (" + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");

//        // From q and the search radius it is straigthforward to calculate the required resolution and thus octree level
//        float resolution_min = 2*suggested_q/suggested_search_radius_high;
//        float resolution_max = 2*suggested_q/suggested_search_radius_low;

////        float level_min = std::log(resolution_min/(float)svo->getBrickInnerDimension())/std::log(2.0);
//        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2.0);

//        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
//        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
//        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggested minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
//        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Use at least octree level "+QString::number((int)level_max)+" to achieve good resolution");
//    }

////    qDebug() << suggested_q;

//    emit qSpaceInfoChanged(suggested_search_radius_low, suggested_search_radius_high, suggested_q);
//    emit finished();
//}


/***
 *       dBBBBBb    dBBBP dBBBBBb     dBBBBb
 *           dBP               BB        dBP
 *       dBBBBK'  dBBP     dBP BB   dBP dBP
 *      dBP  BB  dBP      dBP  BB  dBP dBP
 *     dBP  dB' dBBBBP   dBBBBBBB dBBBBBP
 *
 */

//ReadFileWorker::ReadFileWorker()
//{
//    this->isCLInitialized = false;
//}

//ReadFileWorker::~ReadFileWorker()
//{

//}

//void ReadFileWorker::test()
//{
//    qDebug("Test");
//}

//void ReadFileWorker::process()
//{
//    QCoreApplication::processEvents();
    
//    if (files->size() <= 0)
//    {
//        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files specified!");
//        emit changedMessageString(str);
//        kill_flag = true;
//    }

//    // Emit to appropriate slots
//    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Reading "+QString::number(files->size())+" files...");
//    emit changedRangeGenericProcess(1, files->size());
//    emit changedFormatGenericProgress(QString("Reading file %v of %m (%p%)"));
//    emit changedTabWidget(1);


//    QElapsedTimer stopwatch;
//    stopwatch.start();
//    kill_flag = false;
//    size_t size_raw = 0;

//    for (size_t i = 0; i < (size_t) files->size(); i++)
//    {
//        // Kill process if requested
//        if (kill_flag)
//        {
//            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(files->size()));
//            break;
//        }

//        // Read file and get status
//        int STATUS_OK = (*files)[i].readData();
//        size_raw += (*files)[i].getBytes();
        
//        emit changedFile(files->at(i).getPath());
        
//        if (!STATUS_OK)
//        {
//            QString str("\n["+QString(this->metaObject()->className())+"] Warning: could not read \""+files->at(i).getPath()+"\"");

//            emit changedMessageString(str);

//            kill_flag = true;
//        }

//        // Update the progress bar
//        emit changedGenericProgress(i+1);
//    }
//    size_t t = stopwatch.restart();

//    if (!kill_flag)
//    {
//        emit changedMessageString(" "+QString::number(files->size())+" files were read successfully ("+QString::number(size_raw/1000000.0, 'f', 3)+" MB) (" + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'f', 3)+" ms/file)");
//    }

//    emit finished();
//}


/***
 *       dBBBBBb dBBBBBb    dBBBBP    dBP dBBBP  dBBBP dBBBBBBP
 *           dB'     dBP   dBP.BP
 *       dBBBP'  dBBBBK   dBP.BP    dBP dBBP   dBP      dBP
 *      dBP     dBP  BB  dBP.BP dB'dBP dBP    dBP      dBP
 *     dBP     dBP  dB' dBBBBP dBBBBP dBBBBP dBBBBP   dBP
 *
 */

//ReconstructWorker::ReconstructWorker()
//{
//    this->isCLInitialized = false;
//    resolveOpenCLFunctions();

//    offset_omega = 0;
//    offset_kappa = 0;
//    offset_phi = 0;
    
////    selection.setRect(0,0,1e5,1e5);
//}

//ReconstructWorker::~ReconstructWorker()
//{

//    if (isCLInitialized && project_kernel)
//    {
//        err = QOpenCLReleaseKernel(project_kernel);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
//}

////void ProjectFileWorker::setSelection(Selection area)
////{
////    selection = area;
////}

//void ReconstructWorker::initializeCLKernel()
//{
//    QStringList paths;
//    paths << "kernels/project.cl";

//    program = context_cl.createProgram(paths, &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    context_cl.buildProgram(&program, "-Werror");

//    // Kernel handles
//    project_kernel = QOpenCLCreateKernel(program, "FRAME_FILTER", &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    isCLInitialized = true;
//}

//void ProjectFileWorker::process()
//{
//    /* For each file, project the detector coordinate and corresponding intensity down onto the Ewald sphere. Intensity corrections are also carried out in this step. The header of each file should include all the required information to to the transformations. The result is stored in a seprate container. There are different file formats, and all files coming here should be of the same base type. */
//    QCoreApplication::processEvents();
    
//    if (files->size() <= 0)
//    {
//        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files have been specified!");

//        emit changedMessageString(str);

//        kill_flag = true;
//    }

//    // Emit to appropriate slots
//    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Processing "+QString::number(files->size())+" files...");
//    emit changedRangeGenericProcess(1, file_paths->size());
//    emit changedFormatGenericProgress(QString("Projecting file %v of %m (%p%)"));

//    QElapsedTimer stopwatch;
//    stopwatch.start();
//    kill_flag = false;

//    n_reduced_pixels = 0;

//    reduced_pixels->reserve(1, REDUCED_PIXELS_MAX_BYTES/sizeof(float));
    
//    for (size_t i = 0; i < (size_t) files->size(); i++)
//    {
//        // Kill process if requested
//        if (kill_flag)
//        {
//            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(files->size()));
//            reduced_pixels->clear();
//            break;
//        }

//        // Project and correct file and get status
//        if (n_reduced_pixels + 3 >= reduced_pixels->size())
//        {
//            // Break if there is too much data.
//            emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Warning: There was too much data!"));
//            kill_flag = true;
//        }
//        else
//        {
//            int STATUS_OK = projectFile(&(*files)[i]);
//            if (!STATUS_OK)
//            {
//                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: could not process data \""+files->at(i).getPath()+"\"");
//                kill_flag = true;
//            }
//        }
//        // Update the progress bar
//        emit changedGenericProgress(i+1);

//    }
//    size_t t = stopwatch.restart();

//    if (!kill_flag)
//    {
//        reduced_pixels->resize(1, n_reduced_pixels);

//        emit changedMessageString(" "+QString::number(files->size())+" files were processed successfully ("+QString::number((float)reduced_pixels->bytes()/(float)1000000.0, 'f', 3)+" MB) (" + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'f', 3)+" ms/file)");
//    }



//    /* Create dummy dataset for debugging purposes.
//    */
//    if (0) // A sphere
//    {
//        int theta_max = 180; // Up to 180
//        int phi_max = 360; // Up to 360

//        reduced_pixels->resize(1, theta_max*phi_max*4);

//        float radius = 0.7;
//        double pi = 4.0*std::atan(1.0);

//        for (int i = 0; i < theta_max; i++)
//        {
//            for (int j = 0; j < phi_max; j++)
//            {
//                float theta = ((float) i)/180.0 * pi;
//                float phi = ((float) j)/180.0 * pi;

//                (*reduced_pixels)[(i*phi_max+j)*4+0] = radius * std::sin(theta)*std::cos(phi);
//                (*reduced_pixels)[(i*phi_max+j)*4+1] = radius * std::sin(theta)*std::sin(phi);
//                (*reduced_pixels)[(i*phi_max+j)*4+2] = radius * std::cos(theta);
//                (*reduced_pixels)[(i*phi_max+j)*4+3] = 100;
//            }
//        }
//    }
//    else if (0) // A box
//    {
//        int res = 64;
//        reduced_pixels->resize(1, res*res*res*4);

//        for (int i = 0; i < res; i++)
//        {
//            for (int j = 0; j < res; j++)
//            {
//                for (int k = 0; k < res; k++)
//                {
//                    (*reduced_pixels)[(i+j*res+k*res*res)*4+0] = (((float)i/(float)(res-1)) - 0.5)*2.0*0.5;
//                    (*reduced_pixels)[(i+j*res+k*res*res)*4+1] = (((float)j/(float)(res-1)) - 0.5)*2.0*0.5;
//                    (*reduced_pixels)[(i+j*res+k*res*res)*4+2] = (((float)k/(float)(res-1)) - 0.5)*2.0*0.5;
//                    (*reduced_pixels)[(i+j*res+k*res*res)*4+3] = 100.0;
//                }
//            }
//        }
//    }



//    emit finished();
//}

//int ReconstructWorker::projectFile(DetectorFile * file, Selection selection)
//{
//    // Project and correct the data
//    cl_image_format target_format;
//    target_format.image_channel_order = CL_RGBA;
//    target_format.image_channel_data_type = CL_FLOAT;

//    // Prepare the target for storage of projected and corrected pixels (intensity but also xyz position)
//    cl_mem xyzi_target_cl = QOpenCLCreateImage2D ( context_cl.context(),
//        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
//        &target_format,
//        file->getFastDimension(),
//        file->getSlowDimension(),
//        0,
//        NULL,
//        &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    // Load data into a CL texture
//    cl_image_format source_format;
//    source_format.image_channel_order = CL_INTENSITY;
//    source_format.image_channel_data_type = CL_FLOAT;

//    cl_mem source_cl = QOpenCLCreateImage2D ( context_cl.context(),
//        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
//        &source_format,
//        file->getFastDimension(),
//        file->getSlowDimension(),
//        file->getFastDimension()*sizeof(cl_float),
//        file->getData().data(),
//        &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    // A sampler. The filtering should be CL_FILTER_NEAREST unless a linear interpolation of the data is actually what you want
//    cl_sampler intensity_sampler = QOpenCLCreateSampler(context_cl.context(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    // Sample rotation matrix to be applied to each projected pixel to account for rotations. First set the active angle. Ideally this would be given by the header file, but for some reason it is not stated in there. Maybe it is just so normal to rotate around the omega angle to keep the resolution function consistent
    
//    double phi = 0, kappa = 0, omega = 0;
//    if(active_angle == 0)
//    {
//        phi = file->start_angle + 0.5*file->angle_increment;
//        kappa = file->kappa;
//        omega = file->omega;
//    }
//    else if(active_angle == 1) 
//    {
//        phi = file->phi;
//        kappa = file->start_angle + 0.5*file->angle_increment;
//        omega = file->omega;
//    }
//    else if(active_angle == 2) 
//    {
//        phi = file->phi;
//        kappa = file->kappa;
//        omega = file->start_angle + 0.5*file->angle_increment;
//    }
    
//    RotationMatrix<double> PHI;
//    RotationMatrix<double> KAPPA;
//    RotationMatrix<double> OMEGA;
//    RotationMatrix<double> sampleRotMat;

//    file->alpha =  0.8735582;
//    file->beta =  0.000891863;
    
//    PHI.setArbRotation(file->beta, 0, -(phi+offset_phi)); 
//    KAPPA.setArbRotation(file->alpha, 0, -(kappa+offset_kappa));
//    OMEGA.setZRotation(-(omega+offset_omega));
    
//    // The sample rotation matrix. Some rotations perturb the other rotation axes, and in the above calculations for phi, kappa, and omega we use fixed axes. It is therefore neccessary to put a rotation axis back into its basic position before the matrix is applied. In our case omega perturbs kappa and phi, and kappa perturbs phi. Thus we must first rotate omega back into the base position to recover the base rotation axis of kappa. Then we recover the base rotation axis for phi in the same manner. The order of matrix operations thus becomes:
    
//    sampleRotMat = PHI*KAPPA*OMEGA;

//    cl_mem sample_rotation_matrix_cl = QOpenCLCreateBuffer(context_cl.context(),
//        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
//        sampleRotMat.toFloat().bytes(),
//        sampleRotMat.toFloat().data(),
//        &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    // The sampler for cl_tsf_tex
//    cl_sampler tsf_sampler = QOpenCLCreateSampler(context_cl.context(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    // Set kernel arguments
//    err = QOpenCLSetKernelArg(project_kernel, 0, sizeof(cl_mem), (void *) &xyzi_target_cl);
//    err |= QOpenCLSetKernelArg(project_kernel, 1, sizeof(cl_mem), (void *) &source_cl);
//    err |= QOpenCLSetKernelArg(project_kernel, 2, sizeof(cl_sampler), &tsf_sampler);
//    err |= QOpenCLSetKernelArg(project_kernel, 3, sizeof(cl_sampler), &intensity_sampler);
//    err |= QOpenCLSetKernelArg(project_kernel, 4, sizeof(cl_mem), (void *) &sample_rotation_matrix_cl);
////    float threshold_one[2], threshold_two[2];
////    threshold_one[0] = this->thld_noise_low;
////    threshold_one[1] = this->thld_noise_high;
////    threshold_two[0] = this->thld_project_low;
////    threshold_two[1] = this->thld_project_high;
    
    
    
////    err |= QOpenCLSetKernelArg(project_kernel, 5, 2*sizeof(cl_float), threshold_one);
////    err |= QOpenCLSetKernelArg(project_kernel, 6, 2*sizeof(cl_float), threshold_two);
////    err |= QOpenCLSetKernelArg(project_kernel, 7, sizeof(cl_float), &file->background_flux);
////    err |= QOpenCLSetKernelArg(project_kernel, 8, sizeof(cl_float), &file->backgroundExpTime);
//    err |= QOpenCLSetKernelArg(project_kernel, 5, sizeof(cl_float), &file->pixel_size_x);
//    err |= QOpenCLSetKernelArg(project_kernel, 6, sizeof(cl_float), &file->pixel_size_y);
////    err |= QOpenCLSetKernelArg(project_kernel, 11, sizeof(cl_float), &file->exposure_time);
//    err |= QOpenCLSetKernelArg(project_kernel, 7, sizeof(cl_float), &file->wavelength);
//    err |= QOpenCLSetKernelArg(project_kernel, 8, sizeof(cl_float), &file->detector_distance);
//    err |= QOpenCLSetKernelArg(project_kernel, 9, sizeof(cl_float), &file->beam_x);
//    err |= QOpenCLSetKernelArg(project_kernel, 10, sizeof(cl_float), &file->beam_y);
////    err |= QOpenCLSetKernelArg(project_kernel, 16, sizeof(cl_float), &file->flux);
//    err |= QOpenCLSetKernelArg(project_kernel, 11, sizeof(cl_float), &file->start_angle);
//    err |= QOpenCLSetKernelArg(project_kernel, 12, sizeof(cl_float), &file->angle_increment);
//    err |= QOpenCLSetKernelArg(project_kernel, 13, sizeof(cl_float), &file->kappa);
//    err |= QOpenCLSetKernelArg(project_kernel, 14, sizeof(cl_float), &file->phi);
//    err |= QOpenCLSetKernelArg(project_kernel, 15, sizeof(cl_float), &file->omega);
////    err |= QOpenCLSetKernelArg(project_kernel, 22, sizeof(cl_float), &file->max_counts);
//    err |= QOpenCLSetKernelArg(project_kernel, 16, sizeof(cl_int4), selection.lrtb().data());
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
////    selection.lrtb().print(0,"lrtb");

//    /* Launch rendering kernel */
//    size_t area_per_call[2] = {128, 128};
//    size_t call_offset[2] = {0,0};
//    size_t loc_ws[2];
//    size_t glb_ws[2];
    
//    loc_ws[0] = 16;
//    loc_ws[1] = 16;
//    glb_ws[0] = file->getFastDimension() + loc_ws[0] - (file->getFastDimension()%loc_ws[0]);
//    glb_ws[1] = file->getSlowDimension() + loc_ws[1] - (file->getSlowDimension()%loc_ws[1]);
    
//    for (size_t glb_x = 0; glb_x < glb_ws[0]; glb_x += area_per_call[0])
//    {
//        for (size_t glb_y = 0; glb_y < glb_ws[1]; glb_y += area_per_call[1])
//        {
//            call_offset[0] = glb_x;
//            call_offset[1] = glb_y;

//            err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), project_kernel, 2, call_offset, area_per_call, loc_ws, 0, NULL, NULL);
//            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//        }
//    }
//    QOpenCLFinish(context_cl.queue());
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    // Read the data
//    size_t origin[3];
//    origin[0] = selection.left();
//    origin[1] = selection.top();
//    origin[2] = 0;

//    size_t region[3];
//    region[0] = selection.width();
//    region[1] = selection.height();
//    region[2] = 1;
    
////    qDebug() << origin[0] << origin[1];
////    qDebug() << region[0] << region[1];
    
////    qDebug() << selection;
    
//    Matrix<float> projected_data_buf(1,selection.width()*selection.height()*4);
    
//    err = QOpenCLEnqueueReadImage ( context_cl.queue(),
//                                    xyzi_target_cl, 
//                                    true, 
//                                    origin, 
//                                    region, 
//                                    0, 0, 
//                                    projected_data_buf.data(), 
//                                    0, NULL, NULL);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
////    projected_data_buf.print(2,"Data");
    
//    if (xyzi_target_cl){
//        err = QOpenCLReleaseMemObject(xyzi_target_cl);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
//    if (source_cl){
//        err = QOpenCLReleaseMemObject(source_cl);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
//    if (sample_rotation_matrix_cl){
//        err = QOpenCLReleaseMemObject(sample_rotation_matrix_cl);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
//    if (intensity_sampler){
//        err = QOpenCLReleaseSampler(intensity_sampler);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
//    if (tsf_sampler){
//        err = QOpenCLReleaseSampler(tsf_sampler);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }

//    emit changedFormatMemoryUsage(QString("Mem usage: %p% (%v of %m KB)"));
    
    
//    for (int i = 0; i < selection.width()*selection.height(); i++)
//    {
//        if (projected_data_buf[i*4+3] > 0.0) // Above 0 check
//        {
//            if ((n_reduced_pixels)+3 < reduced_pixels->size())
//            {
//                (*reduced_pixels)[(n_reduced_pixels)+0] = projected_data_buf[i*4+0];
//                (*reduced_pixels)[(n_reduced_pixels)+1] = projected_data_buf[i*4+1];
//                (*reduced_pixels)[(n_reduced_pixels)+2] = projected_data_buf[i*4+2];
//                (*reduced_pixels)[(n_reduced_pixels)+3] = projected_data_buf[i*4+3];
//                (n_reduced_pixels)+=4;
//            }
//            else
//            {
//                emit changedRangeMemoryUsage(0,REDUCED_PIXELS_MAX_BYTES/1e3);
//                emit changedMemoryUsage(n_reduced_pixels*4/1e3);
//                return 0;
//            }
//        }
//    }

//    emit changedRangeMemoryUsage(0,REDUCED_PIXELS_MAX_BYTES/1e3);
//    emit changedMemoryUsage(n_reduced_pixels*4/1e3);

//    return 1;
//}

/***
 *      dBBBBBb     dBP    dBP
 *           BB
 *       dBP BB   dBP    dBP
 *      dBP  BB  dBP    dBP
 *     dBBBBBBB dBBBBP dBBBBP
 *
 */


//MultiWorker::MultiWorker()
//{
//    this->isCLInitialized = false;
//    resolveOpenCLFunctions();
    
//    offset_omega = 0;
//    offset_kappa = 0;
//    offset_phi = 0;
    
//    selection.setRect(0,0,1e5,1e5);
//}

//MultiWorker::~MultiWorker()
//{

//}

//void ReconstructWorker::process()
//{
//    QCoreApplication::processEvents();

//    int verbose = 0;

//    kill_flag = false;
//    if (set.size() <= 0)
//    {
//        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files have been specified");

//        emit changedMessageString(str);
//        kill_flag = true;
//    }

//    // Emit to appropriate slots
//    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Processing set "+QString::number(set.size())+": ");
//    emit changedFormatGenericProgress(QString("Processing file %v of %m (%p%)"));
////    emit changedTabWidget(1);

//    // Parameters for Ewald's projection
//    reduced_pixels->reserve(1, REDUCED_PIXELS_MAX_BYTES/sizeof(float));

//    // Reset suggested values
//    suggested_q = std::numeric_limits<float>::min();
//    suggested_search_radius_low = std::numeric_limits<float>::max();
//    suggested_search_radius_high = std::numeric_limits<float>::min();

//    QElapsedTimer stopwatch;
//    stopwatch.start();
//    size_t n_ok_files = 0;
//    n_reduced_pixels = 0;
//    size_t size_raw = 0;
    
//    // Set to first series
//    set.begin();
    
//    for (size_t i = 0; i < (size_t) set.size(); i++)
//    {
//        // Set to first frame
//        set.current()->begin();
//        emit changedRangeGenericProcess(1, set.current()->size());
        
//        for (size_t j = 0; j < (size_t) set.current()->size(); j++)
//        {
//            // Kill process if requested
//            if (kill_flag)
//            {
//                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at set "+QString::number(i+1)+", frame "+QString::number(j+1));
//                reduced_pixels->clear();
    
//                break;
//            }
    
//            // Set file and get status
//            DetectorFile file;
//            int STATUS_OK = file.set(set.current()->current()->path());
    
//            if (STATUS_OK)
//            {
////                emit changedFile(set.current()->current()->path());
    
//                // Read file and get status
//                int STATUS_OK = file.readData();
//                if (STATUS_OK)
//                {
//                    size_raw += file.getBytes();
    
//                    // Project and correct file and get status
//                    Selection selection = set.current()->current()->selection();
//                    if (selection.width() > file.getFastDimension()) selection.setWidth(file.getFastDimension());
//                    if (selection.height() > file.getSlowDimension()) selection.setHeight(file.getSlowDimension());
//                    int STATUS_OK = projectFile(&file, selection);
    
//                    if (STATUS_OK)
//                    {
//                        // Get suggestions on the minimum search radius that can safely be applied during interpolation
//                        if (suggested_search_radius_low > file.getSearchRadiusLowSuggestion()) suggested_search_radius_low = file.getSearchRadiusLowSuggestion();
//                        if (suggested_search_radius_high < file.getSearchRadiusHighSuggestion()) suggested_search_radius_high = file.getSearchRadiusHighSuggestion();
    
//                        // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
//                        if (suggested_q < file.getQSuggestion()) suggested_q = file.getQSuggestion();
    
//                        n_ok_files++;
                        
//                        set.current()->next();
//                    }
//                    else
//                    {
//                        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Could not process \""+set.current()->current()->path()+"\".\n Too much data was kept during reconstruction. Try increasing the lower data thresholds (noise, PCT).");
//                        kill_flag = true;
//                    }
//                }
//                else if (!STATUS_OK)
//                {
//    //                QString str("\n["+QString(this->metaObject()->className())+"] Warning: Could not read \""+file.getPath()+"\"");
//                    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Could not read \""+set.current()->current()->path()+"\"");
//                    kill_flag = true;
//                }
                
//                // Update the progress bar
//                emit changedGenericProgress(j+1);
//            }
//            else
//            {
//                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Could not set \""+QString(set.current()->current()->path())+"\"");
//                kill_flag = true;
//            }
            
            
//        }
        
//        set.next();
//    }

//    size_t t = stopwatch.restart();

//    if (!kill_flag)
//    {
//        reduced_pixels->resize(1, n_reduced_pixels);

//        emit changedMessageString(" "+QString::number(n_ok_files)+" files were successfully processed ("+QString::number(size_raw/1000000.0, 'f', 3)+" MB -> "+QString::number((float)reduced_pixels->bytes()/(float)1000000.0, 'f', 3)+" MB, " + QString::number(t) + " ms, "+QString::number((float)t/(float)n_ok_files, 'g', 3)+" ms/file)");

//        // From q and the search radius it is straigthforward to calculate the required resolution and thus octree level
//        float resolution_min = 2*suggested_q/suggested_search_radius_high;
//        float resolution_max = 2*suggested_q/suggested_search_radius_low;

//        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2.0);

//        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
//        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
//        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggested minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
//        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Use at least octree level "+QString::number((int)level_max)+" to achieve good resolution");

//        emit qSpaceInfoChanged(suggested_search_radius_low, suggested_search_radius_high, suggested_q);
//    }


//    emit finished();
//}


/***
 *      dBP dP  dBBBBP`Bb  .BP    dBBBP  dBP    dBP dBBBBBP    dBBBP
 *             dBP.BP     .BP
 *     dB .BP dBP.BP    dBBK    dBBP   dBP    dBP     dBP    dBBP
 *     BB.BP dBP.BP    dB'     dBP    dBP    dBP     dBP    dBP
 *     BBBP dBBBBP    dB' dBP dBBBBP dBBBBP dBP     dBBBBP dBBBBP
 *
 */

VoxelizeWorker::VoxelizeWorker()
{
    this->isCLInitialized = false;
    resolveOpenCLFunctions();
}

VoxelizeWorker::~VoxelizeWorker()
{
    if (isCLInitialized)
    {
        err = QOpenCLReleaseKernel(voxelize_kernel);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

unsigned int VoxelizeWorker::getOctIndex(unsigned int msdFlag, unsigned int dataFlag, unsigned int child)
{
    return (msdFlag << 31) | (dataFlag << 30) | child;
}

unsigned int VoxelizeWorker::getOctBrick(unsigned int poolX, unsigned int poolY, unsigned int poolZ)
{
    return (poolX << 20) | (poolY << 10) | (poolZ << 0);
}

void VoxelizeWorker::initializeCLKernel()
{
//    context_cl = new OpenCLContext;
    context_cl.initDevices();
    context_cl.initNormalContext();
    context_cl.initCommandQueue();
    context_cl.initResources();

    QStringList paths;
    paths << "kernels/voxelize.cl";

    program = context_cl.createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl.buildProgram(&program, "-Werror");


    // Kernel handles
    voxelize_kernel = QOpenCLCreateKernel(program, "voxelize", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    fill_kernel = QOpenCLCreateKernel(program, "fill", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isCLInitialized = true;
}

void VoxelizeWorker::process()
{
    // Note: The term node and brick are used a lot. A brick is a 3D voxel grid of data points, and each node has one brick and some extra metadata. 
    
    QCoreApplication::processEvents();

    kill_flag = false;

    if (reduced_pixels->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No data available!");
        emit changedMessageString(str);
        kill_flag = true;
    }



    if (!kill_flag)
    {
        QElapsedTimer totaltime;
        totaltime.start();

        // Emit to appropriate slots
        emit changedFormatGenericProgress(QString(" Creating Interpolation Data Structure: %p%"));
        emit changedRangeGenericProcess(0, 100);
        emit showProgressBar(true);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Generating Sparse Voxel Octree "+QString::number(svo->levels())+" levels deep.");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] The source data is "+QString::number(reduced_pixels->bytes()/1000000.0, 'g', 3)+" MB");

        // The number of data points in a single brick
        size_t n_points_brick = svo->brickOuterDimension()*svo->brickOuterDimension()*svo->brickOuterDimension();

        // Save the extent of the volume
        svo->setExtent(suggested_q);
        
        // Save initial metadata (just text)
        svo->setMetaData("You can write notes about the dataset here.");

        // Prepare the brick pool in which we will store bricks
        Matrix<int> pool_dimension(1, 4, 0);
        pool_dimension[0] = (1 << svo->brickPoolPower())*svo->brickOuterDimension();
        pool_dimension[1] = (1 << svo->brickPoolPower())*svo->brickOuterDimension();
        pool_dimension[2] = (BRICK_POOL_SOFT_MAX_BYTES/(sizeof(cl_float)*svo->brickOuterDimension()*svo->brickOuterDimension()*svo->brickOuterDimension())) / ((1 << svo->brickPoolPower())*(1 << svo->brickPoolPower()));

        if (pool_dimension[2] < 1) pool_dimension[2] = 1;
        pool_dimension[2] *= svo->brickOuterDimension();

        size_t BRICK_POOL_HARD_MAX_BYTES = pool_dimension[0]*pool_dimension[1]*pool_dimension[2]*sizeof(float);

        emit changedFormatMemoryUsage(QString("Mem usage: %p% (%v of %m MB)"));
        emit changedRangeMemoryUsage(0,BRICK_POOL_HARD_MAX_BYTES/1e6);
        emit changedMemoryUsage(0);

        size_t n_max_bricks = BRICK_POOL_HARD_MAX_BYTES/(n_points_brick*sizeof(float));

        // Prepare the relevant OpenCL buffers
        cl_mem pool_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            BRICK_POOL_HARD_MAX_BYTES,
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        cl_mem pool_cluster_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*svo->brickOuterDimension()*svo->brickOuterDimension()*svo->brickOuterDimension()*sizeof(float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        cl_mem brick_extent_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*6*sizeof(float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        cl_mem point_data_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_POINTS_PER_CLUSTER*sizeof(cl_float4),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        cl_mem point_data_offset_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*sizeof(cl_int),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                
        cl_mem point_data_count_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*sizeof(cl_int),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                           
        Matrix<float> min_check(1, MAX_NODES_PER_CLUSTER, 0);
        cl_mem min_check_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*sizeof(cl_float),
            min_check.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        Matrix<float> sum_check(1, MAX_NODES_PER_CLUSTER, 0);
        cl_mem sum_check_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*sizeof(cl_float),
            sum_check.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        Matrix<float> variance_check(1, MAX_NODES_PER_CLUSTER, 0);
        cl_mem variance_check_cl = QOpenCLCreateBuffer(context_cl.context(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            MAX_NODES_PER_CLUSTER*sizeof(cl_float),
            variance_check.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        // Place all data points in an octree data structure from which to construct the bricks in the brick pool
        SearchNode root(NULL, svo->extent().data());
        
//        qDebug() << "Voxelize size" << reduced_pixels->size();
        
        for (size_t i = 0; i < reduced_pixels->size()/4; i++)
        {
            if (kill_flag)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(reduced_pixels->size()/4));
                break;
            }
            root.insert(reduced_pixels->data()+i*4);
            emit changedGenericProgress((i+1)*100/(reduced_pixels->size()/4));
        }
        
        if (!kill_flag)
        {
            /* Create an octree from brick data. The nodes are maintained in a linear array rather than a tree. This is mainly due to (current) lack of proper support for recursion on GPUs */
            Matrix<BrickNode> gpuHelpOctree(1, n_max_bricks*16);
            gpuHelpOctree[0].setParent(0);
            
            // An array to store number of nodes per level
            Matrix<unsigned int> nodes;
            nodes.set(1, 16, (unsigned int) 0); // Note: make bigger
            nodes[0] = 1;

            unsigned int nodes_prev_lvls = 0, non_empty_node_counter = 0;

            QElapsedTimer timer;
            
            // Keep track of the maximum sum returned by a node and use it later to estimate max value in data set
            float max_brick_sum = 0.0;
            
            // Containers 
            Matrix<double> brick_extent(1, 6*MAX_NODES_PER_CLUSTER); // The extent of each brick in a kernel invocation
            Matrix<int> point_data_offset(1, MAX_NODES_PER_CLUSTER); // The data offset for each brick in a kernel invocation
            Matrix<int> point_data_count(1, MAX_NODES_PER_CLUSTER); // The data size for each brick in a kernel invocation
            Matrix<float> point_data(MAX_POINTS_PER_CLUSTER,4); // Temporaily holds the data for a single brick
            
            
            
            // Cycle through the levels
            for (size_t lvl = 0; lvl < svo->levels(); lvl++)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl+1)+" (dim: "+QString::number(svo->brickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("Constructing Level "+QString::number(lvl+1)+" (dim: "+QString::number(svo->brickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                // Find the correct range search radius, which will be smaller for each level until it approaches the distance between samples in the data set
                float search_radius = sqrt(3.0f)*0.5f*((svo->extent().at(1)-svo->extent().at(0))/ (svo->brickInnerDimension()*(1 << lvl)));
                if (search_radius < suggested_search_radius_high) search_radius = suggested_search_radius_high;

                double tmp = (svo->extent().at(1) - svo->extent().at(0)) / (1 << lvl);

                size_t n_nodes_treated = 0;

                // For each cluster of nodes
                while (n_nodes_treated <= nodes[lvl])
                {
                    if((non_empty_node_counter+1) >= n_max_bricks)
                    {
                        QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed due to memory overflow. The dataset has grown too large! ("+QString::number(non_empty_node_counter*n_points_brick*sizeof(cl_float)/1e6, 'g', 3)+" MB)");
                        emit changedMessageString(str);
                        kill_flag = true;
                    }

                    if (kill_flag) break;
                        
                    // First pass: find relevant data for each brick in the node cluster
                    unsigned int currentId;
                    size_t n_points_harvested = 0; // The number of xyzi data points gathered
                    size_t n_nodes_treated_in_cluster = 0; // The number of nodes treated in this iteration of the enclosing while loop

                    while (n_points_harvested < MAX_POINTS_PER_CLUSTER)
                    {
                        // The id of the octnode in the octnode array
                        currentId = nodes_prev_lvls + n_nodes_treated + n_nodes_treated_in_cluster;
                        
                        // Set the level
                        gpuHelpOctree[currentId].setLevel(lvl);

                        // Set the brick id
                        gpuHelpOctree[currentId].calcBrickId((n_nodes_treated + n_nodes_treated_in_cluster)%8 ,&gpuHelpOctree[gpuHelpOctree[currentId].getParent()]);

                        // Based on brick id calculate the brick extent 
                        unsigned int * brick_id = gpuHelpOctree[currentId].getBrickId();
                        
                        brick_extent[n_nodes_treated_in_cluster*6 + 0] = svo->extent().at(0) + tmp*brick_id[0];
                        brick_extent[n_nodes_treated_in_cluster*6 + 1] = svo->extent().at(0) + tmp*(brick_id[0]+1);
                        brick_extent[n_nodes_treated_in_cluster*6 + 2] = svo->extent().at(2) + tmp*brick_id[1];
                        brick_extent[n_nodes_treated_in_cluster*6 + 3] = svo->extent().at(2) + tmp*(brick_id[1]+1);
                        brick_extent[n_nodes_treated_in_cluster*6 + 4] = svo->extent().at(4) + tmp*brick_id[2];
                        brick_extent[n_nodes_treated_in_cluster*6 + 5] = svo->extent().at(4) + tmp*(brick_id[2]+1);
                        
                        // Offset of points accumulated thus far
                        point_data_offset[n_nodes_treated_in_cluster] = n_points_harvested;
                        size_t premature_termination = 0;

                        // Get point data needed for this brick 
                        if( root.getData(
                                    MAX_POINTS_PER_CLUSTER,
                                    brick_extent.data() + n_nodes_treated_in_cluster*6,
                                    point_data.data(),
                                    &n_points_harvested,
                                    search_radius))
                        {
                            premature_termination = n_nodes_treated_in_cluster;

                            qDebug() << lvl+1 << premature_termination << "of" << MAX_NODES_PER_CLUSTER << "pts:" << n_points_harvested;
                        }

                        // Number of points for this node
                        point_data_count[n_nodes_treated_in_cluster] = n_points_harvested - point_data_offset[n_nodes_treated_in_cluster];
                        
                        // Upload this point data to an OpenCL buffer
                        if (point_data_count[n_nodes_treated_in_cluster] > 0)
                        {
                            err = QOpenCLEnqueueWriteBuffer(context_cl.queue(),
                                point_data_cl ,
                                CL_TRUE,
                                point_data_offset[n_nodes_treated_in_cluster]*sizeof(cl_float4),
                                point_data_count[n_nodes_treated_in_cluster]*sizeof(cl_float4),
                                point_data.data() + point_data_offset[n_nodes_treated_in_cluster]*4, // ?? Redundant storage? At least move outisde of loop?
                                0, NULL, NULL);
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                        }

                        n_nodes_treated_in_cluster++;


                        // Break off loop when all nodes are processed
                        if ((n_nodes_treated + n_nodes_treated_in_cluster >= nodes[lvl]) || (premature_termination) || (n_nodes_treated_in_cluster >= MAX_NODES_PER_CLUSTER) ) break;
                    }

                    // The extent of each brick
                    err = QOpenCLEnqueueWriteBuffer(
                        context_cl.queue(),
                        brick_extent_cl ,
                        CL_TRUE,
                        0,
                        brick_extent.toFloat().bytes(),
                        brick_extent.toFloat().data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    // The data offset for each brick
                    err = QOpenCLEnqueueWriteBuffer(context_cl.queue(),
                        point_data_offset_cl ,
                        CL_TRUE,
                        0,
                        point_data_offset.bytes(),
                        point_data_offset.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    // The data size for each brick
                    err = QOpenCLEnqueueWriteBuffer(context_cl.queue(),
                        point_data_count_cl ,
                        CL_TRUE,
                        0,
                        point_data_count.bytes(),
                        point_data_count.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    // Second pass: calculate the data for each node in the cluster (OpenCL)
                    // Set kernel arguments
//                    qDebug() << point_data.size();
//                    qDebug() << point_data_offset.size();
//                    qDebug() << point_data_offset.bytes();
//                    qDebug() << point_data_count.size();
//                    qDebug() << brick_extet.size();

                    err = QOpenCLSetKernelArg( voxelize_kernel, 0, sizeof(cl_mem), (void *) &point_data_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 1, sizeof(cl_mem), (void *) &point_data_offset_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 2, sizeof(cl_mem), (void *) &point_data_count_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 3, sizeof(cl_mem), (void *) &brick_extent_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 4, sizeof(cl_mem), (void *) &pool_cluster_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 5, sizeof(cl_mem), (void *) &min_check_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 6, sizeof(cl_mem), (void *) &sum_check_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 7, sizeof(cl_mem), (void *) &variance_check_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 8, svo->brickOuterDimension()*svo->brickOuterDimension()*svo->brickOuterDimension()*sizeof(cl_float), NULL);
                    int tmp = svo->brickOuterDimension(); // why a separate variable?
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 9, sizeof(cl_int), &tmp);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 10, sizeof(cl_float), &search_radius);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    err = QOpenCLFinish(context_cl.queue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                            
                    // Interpolate data for each brick
                    for (size_t j = 0; j < n_nodes_treated_in_cluster; j++)
                    {
                        // Launch kernel
                        size_t glb_offset[3] = {0,0,8*j};
                        size_t loc_ws[3] = {8,8,8};
                        size_t glb_ws[3] = {8,8,8};
                        err = QOpenCLEnqueueNDRangeKernel(
                                    context_cl.queue(),
                                    voxelize_kernel, 
                                    3, 
                                    glb_offset, 
                                    glb_ws, 
                                    loc_ws, 
                                    0, NULL, NULL);
                        if ( err != CL_SUCCESS)
                        {
                            qDebug() << j << "of" << n_nodes_treated_in_cluster << tmp << search_radius;
                            qFatal(cl_error_cstring(err));
                        }
                        
//                        if (i + j + 1 >= nodes[lvl]) break;
                    }
                    err = QOpenCLFinish(context_cl.queue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    // The minimum value data point in each brick
                    err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                        min_check_cl,
                        CL_TRUE,
                        0,
                        MAX_NODES_PER_CLUSTER*sizeof(float),
                        min_check.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

                    // The sum of data points in each brick
                    err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                        sum_check_cl,
                        CL_TRUE,
                        0,
                        MAX_NODES_PER_CLUSTER*sizeof(float),
                        sum_check.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    // The variance of data points in each brick
                    err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                        variance_check_cl,
                        CL_TRUE,
                        0,
                        MAX_NODES_PER_CLUSTER*sizeof(float),
                        variance_check.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    err = QOpenCLFinish(context_cl.queue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    // Third pass: transfer non-empty nodes to svo data structure (OpenCL)
                    for (size_t j = 0; j < n_nodes_treated_in_cluster; j++)
                    {
                        // The id of the octnode in the octnode array
                        currentId = nodes_prev_lvls + n_nodes_treated + j;
                        
                        // If a node has no relevant data
                        if ((sum_check[j] <= 0.0))
                        {
                            gpuHelpOctree[currentId].setDataFlag(0);
                            gpuHelpOctree[currentId].setMsdFlag(1);
                            gpuHelpOctree[currentId].setChild(0);
                        }
                        // Else a node has data and possibly qualifies for children
                        else
                        {
                            if ((non_empty_node_counter+1)*n_points_brick*sizeof(float) >= BRICK_POOL_HARD_MAX_BYTES)
                            {
                                emit popup(QString("Warning - Data Overflow"), QString("The dataset you are trying to create grew too large, and exceeded the limit of " +QString::number(BRICK_POOL_HARD_MAX_BYTES/1e6)+ " MB. The issue can be remedied by applying more stringent reconstruction parameters or by reducing the octree level."));
                                kill_flag = true;
                                break;
                            }
                            
                            gpuHelpOctree[currentId].setDataFlag(1);
                            gpuHelpOctree[currentId].setMsdFlag(0);

                            // Set maximum subdivision if the max level is reached or if the variance of the brick data is small compared to the average.
                            float average = sum_check[j]/(float)n_points_brick;
                            float std_dev = sqrt(variance_check[j]);

                            if ((lvl >= svo->levels() - 1) || // Max level
                                ((std_dev <= 0.5 * average) && (min_check[j] > 0) && (svo->levels() - lvl < 3 )) || // Voxel data is self-similar and all voxels are non-zero
                                ((std_dev <= 0.2 * average) && (svo->levels() - lvl < 3 ))) // Voxel data is self-similar
                            {
//                                qDebug() << "Terminated brick early at lvl" << lvl << "Average:" << sum_check[j]/(float)n_points_brick << "Variance:" << variance_check[j];
                                gpuHelpOctree[currentId].setMsdFlag(1);
                            }

                            // Set the pool id of the brick corresponding to the node
                            gpuHelpOctree[currentId].calcPoolId(svo->brickPoolPower(), non_empty_node_counter);

                            // Find the max sum of a brick
                            if (sum_check[j] > max_brick_sum) max_brick_sum = sum_check[j];

                            
                            // Transfer brick data to pool
                            err = QOpenCLSetKernelArg( fill_kernel, 0, sizeof(cl_mem), (void *) &pool_cluster_cl);
                            err |= QOpenCLSetKernelArg( fill_kernel, 1, sizeof(cl_mem), (void *) &pool_cl);
                            err |= QOpenCLSetKernelArg( fill_kernel, 2, sizeof(cl_int4), pool_dimension.data());
                            int tmp = svo->brickOuterDimension(); // ?
                            err |= QOpenCLSetKernelArg( fill_kernel, 3, sizeof(cl_uint), &tmp);
                            err |= QOpenCLSetKernelArg( fill_kernel, 4, sizeof(cl_uint), &non_empty_node_counter);
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                            
                            // Write data to the brick pool
                            size_t glb_offset[3] = {0,0,8*j};
                            size_t loc_ws[3] = {8,8,8};
                            size_t glb_ws[3] = {8,8,8};
                            err = QOpenCLEnqueueNDRangeKernel(
                                        context_cl.queue(),
                                        fill_kernel, 
                                        3, 
                                        glb_offset, 
                                        glb_ws, 
                                        loc_ws, 
                                        0, NULL, NULL);
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

                            err = QOpenCLFinish(context_cl.queue());
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                            
                            non_empty_node_counter++;
                            
                            // Account for children
                            if (!gpuHelpOctree[currentId].getMsdFlag())
                            {
                                unsigned int childId = nodes_prev_lvls + nodes[lvl] + nodes[lvl+1];
                                gpuHelpOctree[currentId].setChild(childId); // Index points to first child only

                                // For each child
                                for (size_t k = 0; k < 8; k++)
                                {
                                    gpuHelpOctree[childId+k].setParent(currentId);
                                    nodes[lvl+1]++;
                                }
                            }
                        }
//                        if (i + j + 1 >= nodes[lvl]) break;
                    }

                    n_nodes_treated += n_nodes_treated_in_cluster;

                    emit changedMemoryUsage(non_empty_node_counter*n_points_brick*sizeof(float)/1e6);

                    emit changedGenericProgress((n_nodes_treated+1)*100/nodes[lvl]);
                }
                if (kill_flag)
                {

                    QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(lvl+1)+" of "+QString::number(svo->levels())+"!");

                    emit changedMessageString(str);
                    break;
                }

                nodes_prev_lvls += nodes[lvl];
                
                size_t t = timer.restart();
                emit changedMessageString(" ...done ("+QString::number(t)+" ms, "+QString::number(nodes[lvl])+" nodes)");
            }
            
            if (!kill_flag)
            {
                // Use the node structure to populate the GPU arrays
                emit changedFormatGenericProgress(QString(" Transforming: %p%"));
                svo->index()->reserve(1, nodes_prev_lvls);
                svo->brick()->reserve(1, nodes_prev_lvls);

                for (size_t i = 0; i < nodes_prev_lvls; i++)
                {
                    (*svo->index())[i] = getOctIndex(gpuHelpOctree[i].getMsdFlag(), gpuHelpOctree[i].getDataFlag(), gpuHelpOctree[i].getChild());
                    (*svo->brick())[i] = getOctBrick(gpuHelpOctree[i].getPoolId()[0], gpuHelpOctree[i].getPoolId()[1], gpuHelpOctree[i].getPoolId()[2]);

                    emit changedGenericProgress((i+1)*100/nodes_prev_lvls);
                }

                // Round up to the lowest number of bricks that is multiple of the brick pool dimensions. Use this value to reserve data for the data pool
                unsigned int non_empty_node_counter_rounded_up = non_empty_node_counter + ((pool_dimension[0] * pool_dimension[1] / (svo->brickOuterDimension()*svo->brickOuterDimension())) - (non_empty_node_counter % (pool_dimension[0] * pool_dimension[1] / (svo->brickOuterDimension()*svo->brickOuterDimension()))));

                // Read results
                svo->pool()->reserve(1, non_empty_node_counter_rounded_up*n_points_brick);
                
                svo->setMin(0.0f);
                svo->setMax(max_brick_sum/(float)(n_points_brick));
                
                err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                    pool_cl,
                    CL_TRUE,
                    0,
                    non_empty_node_counter_rounded_up*n_points_brick*sizeof(float),
                    svo->pool()->data(),
                    0, NULL, NULL);
                if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            }

            if (!kill_flag)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Done ("+QString::number(totaltime.elapsed())+" ms).\nThe dataset consists of "+QString::number(nodes_prev_lvls)+" bricks and is approx "+QString::number((svo->bytes())/1e6, 'g', 3)+" MB\nThe dataset can now be saved");

            }
        }

        emit showProgressBar(false);

        err = QOpenCLReleaseMemObject(point_data_cl);
        err |= QOpenCLReleaseMemObject(point_data_offset_cl);
        err |= QOpenCLReleaseMemObject(point_data_count_cl);
        err |= QOpenCLReleaseMemObject(brick_extent_cl);
        err |= QOpenCLReleaseMemObject(pool_cluster_cl);
        err |= QOpenCLReleaseMemObject(pool_cl);
        err |= QOpenCLReleaseMemObject(min_check_cl);
        err |= QOpenCLReleaseMemObject(sum_check_cl);
        err |= QOpenCLReleaseMemObject(variance_check_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    emit finished();
}

