#include "worker.h"

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


#include <QOpenGLContext>

static const size_t REDUCED_PIXELS_MAX_BYTES = 1e9;
static const size_t BRICK_POOL_SOFT_MAX_BYTES = 0.7e9; // Effectively limited by the max allocation size for global memory if the pool resides on the GPU during pool construction. 3D image can be used with OpenCL 1.2, allowing you to use the entire VRAM.
static const size_t nodes_per_kernel_call = 1024;
static const size_t max_points_per_cluster = 50e6;


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
}

BaseWorker::~BaseWorker()
{
    
}

void BaseWorker::setOpenCLContext(OpenCLContext * context)
{
    this->context_cl = context;
}

void BaseWorker::setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl)
{

    this->alpha_img_clgl = alpha_img_clgl;
    this->beta_img_clgl = beta_img_clgl;
    this->gamma_img_clgl = gamma_img_clgl;
    this->tsf_img_clgl = tsf_img_clgl;
}

void BaseWorker::setSVOFile(SparseVoxelOcttree * svo)
{

    this->svo = svo;
}

void BaseWorker::setOffsetOmega(double value)
{
    offset_omega = value;
}
void BaseWorker::setOffsetKappa(double value)
{
    offset_kappa = value;
}
void BaseWorker::setOffsetPhi(double value)
{
    offset_phi = value;
}

void BaseWorker::setActiveAngle(int value)
{
    active_angle = value;
}

void BaseWorker::setReduceThresholdLow(double value)
{
    this->threshold_reduce_low = (float) value;
}
void BaseWorker::setReduceThresholdHigh(double value)
{
    this->threshold_reduce_high = (float) value;
}
void BaseWorker::setProjectThresholdLow(double value)
{
    this->threshold_project_low = (float) value;
}
void BaseWorker::setProjectThresholdHigh(double value)
{
    this->threshold_project_high = (float) value;
}

void BaseWorker::killProcess()
{

    kill_flag = true;
}

void BaseWorker::setFilePaths(QStringList * file_paths)
{
    this->file_paths = file_paths;
}

void BaseWorker::setQSpaceInfo(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q)
{

    this->suggested_search_radius_low = suggested_search_radius_low;
    this->suggested_search_radius_high = suggested_search_radius_high;
    this->suggested_q = suggested_q;
}


void BaseWorker::setFiles(QList<DetectorFile> * files)
{

    this->files = files;
}
void BaseWorker::setReducedPixels(Matrix<float> *reduced_pixels)
{

    this->reduced_pixels = reduced_pixels;
}



ReadScriptWorker::ReadScriptWorker()
{

}

ReadScriptWorker::~ReadScriptWorker()
{

}

void ReadScriptWorker::setScriptEngine(QScriptEngine * engine)
{
    this->engine = engine;
}

void ReadScriptWorker::setInput(QPlainTextEdit * widget)
{
    this->inputWidget = widget;
}

void ReadScriptWorker::process()
{
    QCoreApplication::processEvents();
    
    kill_flag = false;

    // Set the corresponding tab
    emit changedTabWidget(0);
    emit changedFormatGenericProgress(QString("Progress: %p%"));

    // Evaluate the script input
    engine->evaluate("var files = [];");
    engine->evaluate(inputWidget->toPlainText());

    // Handle exceptions
    if (engine->hasUncaughtException() == true)
    {
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Script error: Uncaught exception in line " + QString::number(engine->uncaughtExceptionLineNumber()) + "\n["+QString(this->metaObject()->className())+"] " + engine->uncaughtException().toString());
    }
    else
    {
        // Store evaluated file paths in a list
        *file_paths = engine->globalObject().property("files").toVariant().toStringList();
        int n = file_paths->removeDuplicates();
        emit changedMessageString( "\n["+QString(this->metaObject()->className())+"] Script ran successfully and could register "+QString::number(file_paths->size())+" files...");

        if (n > 0) emit changedMessageString( "\n["+QString(this->metaObject()->className())+"] Removed "+QString::number(n)+" duplicates...");

        size_t n_files = file_paths->size();

        for (int i = 0; i < file_paths->size(); i++)
        {
            if (kill_flag)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(file_paths->size()));
                file_paths->clear();
                break;
            }

            if(i >= file_paths->size()) break;

            QString fileName = file_paths->at(i);

            QFileInfo curFile(fileName);

            if (!curFile.exists())
            {
                emit changedMessageString( "\n["+QString(this->metaObject()->className())+"]  Warning: \"" + fileName + "\" - missing or no access!");
                file_paths->removeAt(i);
                i--;
            }

            // Update the progress bar
            if (file_paths->size() > 0) emit changedGenericProgress(100*(i+1)/file_paths->size());
        }
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+ QString::number(file_paths->size())+" of "+QString::number(n_files)+" files successfully found ("+QString::number(n_files-file_paths->size())+"  missing or no access)");
    }

    emit maxFramesChanged(file_paths->size()-1);

    emit finished();
}


/***
 *      .dBBBBP   dBBBP dBBBBBBP
 *      BP
 *      `BBBBb  dBBP     dBP
 *         dBP dBP      dBP
 *    dBBBBP' dBBBBP   dBP
 *
 */

SetFileWorker::SetFileWorker()
{
    this->isCLInitialized = false;
}

SetFileWorker::~SetFileWorker()
{

}

void SetFileWorker::process()
{
    QCoreApplication::processEvents();

    int verbose = 0;

    kill_flag = false;

    if (file_paths->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No paths specified!");

        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Processing "+QString::number(file_paths->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit changedTabWidget(0);

    // Reset suggested values
    suggested_q = std::numeric_limits<float>::min();
    suggested_search_radius_low = std::numeric_limits<float>::max();
    suggested_search_radius_high = std::numeric_limits<float>::min();

    // Clear previous data
    files->clear();
    files->reserve(file_paths->size());

    QElapsedTimer stopwatch;
    stopwatch.start();

    for (size_t i = 0; i < (size_t) file_paths->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(file_paths->size()));
            files->clear();

            break;
        }

        // Set file and get status
        files->append(DetectorFile());
        int STATUS_OK = files->back().set(file_paths->at(i));
        if (STATUS_OK)
        {
            // Get suggestions on the minimum search radius that can safely be applied during interpolation
            if (suggested_search_radius_low > files->back().getSearchRadiusLowSuggestion()) suggested_search_radius_low = files->back().getSearchRadiusLowSuggestion();
            if (suggested_search_radius_high < files->back().getSearchRadiusHighSuggestion()) suggested_search_radius_high = files->back().getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if (suggested_q < files->back().getQSuggestion()) suggested_q = files->back().getQSuggestion();
            
            emit changedFile(files->last().getPath());
        }
        else
        {
            files->removeLast();
            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Could not process \""+QString(file_paths->at(i))+"\"");
        }

        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/file_paths->size());
    }
    size_t t = stopwatch.restart();

    if (!kill_flag)
    {
        emit changedMessageString(" "+QString::number(files->size())+" of "+QString::number(file_paths->size())+" files were processed successfully (" + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");

        // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
        float resolution_min = 2*suggested_q/suggested_search_radius_high;
        float resolution_max = 2*suggested_q/suggested_search_radius_low;

//        float level_min = std::log(resolution_min/(float)svo->getBrickInnerDimension())/std::log(2.0);
        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2.0);

        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggested minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Use at least octree level "+QString::number((int)level_max)+" to achieve good resolution");
    }

    qDebug() << suggested_q;

    emit qSpaceInfoChanged(suggested_search_radius_low, suggested_search_radius_high, suggested_q);
    emit finished();
}


/***
 *       dBBBBBb    dBBBP dBBBBBb     dBBBBb
 *           dBP               BB        dBP
 *       dBBBBK'  dBBP     dBP BB   dBP dBP
 *      dBP  BB  dBP      dBP  BB  dBP dBP
 *     dBP  dB' dBBBBP   dBBBBBBB dBBBBBP
 *
 */

ReadFileWorker::ReadFileWorker()
{
    this->isCLInitialized = false;
}

ReadFileWorker::~ReadFileWorker()
{

}

//void ReadFileWorker::test()
//{
//    qDebug("Test");
//}

void ReadFileWorker::process()
{
    QCoreApplication::processEvents();
    
    if (files->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files specified!");
        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Reading "+QString::number(files->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit changedTabWidget(1);


    QElapsedTimer stopwatch;
    stopwatch.start();
    kill_flag = false;
    size_t size_raw = 0;

    for (size_t i = 0; i < (size_t) files->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(files->size()));
            break;
        }

        // Read file and get status
        int STATUS_OK = (*files)[i].readData();
        size_raw += (*files)[i].getBytes();
        
        emit changedFile(files->at(i).getPath());
        
        if (!STATUS_OK)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Warning: could not read \""+files->at(i).getPath()+"\"");

            emit changedMessageString(str);

            kill_flag = true;
        }

        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/files->size());
    }
    size_t t = stopwatch.restart();

    if (!kill_flag)
    {
        emit changedMessageString(" "+QString::number(files->size())+" files were read successfully ("+QString::number(size_raw/1000000.0, 'f', 3)+" MB) (" + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'f', 3)+" ms/file)");
    }

    emit finished();
}


/***
 *       dBBBBBb dBBBBBb    dBBBBP    dBP dBBBP  dBBBP dBBBBBBP
 *           dB'     dBP   dBP.BP
 *       dBBBP'  dBBBBK   dBP.BP    dBP dBBP   dBP      dBP
 *      dBP     dBP  BB  dBP.BP dB'dBP dBP    dBP      dBP
 *     dBP     dBP  dB' dBBBBP dBBBBP dBBBBP dBBBBP   dBP
 *
 */

ProjectFileWorker::ProjectFileWorker()
{
    this->isCLInitialized = false;
    offset_omega = 0;
    offset_kappa = 0;
    offset_phi = 0;
}

ProjectFileWorker::~ProjectFileWorker()
{

    if (isCLInitialized && project_kernel) clReleaseKernel(project_kernel);
}

void ProjectFileWorker::initializeCLKernel()
{
    Matrix<const char *> paths(1,1);
    paths[0] = "kernels/project.cl";

    program = context_cl->createProgram(&paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    project_kernel = clCreateKernel(program, "FRAME_FILTER", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isCLInitialized = true;
}

void ProjectFileWorker::process()
{
    /* For each file, project the detector coordinate and corresponding intensity down onto the Ewald sphere. Intensity corrections are also carried out in this step. The header of each file should include all the required information to to the transformations. The result is stored in a seprate container. There are different file formats, and all files coming here should be of the same base type. */
    QCoreApplication::processEvents();
    
    if (files->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files have been specified!");

        emit changedMessageString(str);

        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Processing "+QString::number(files->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));

    QElapsedTimer stopwatch;
    stopwatch.start();
    kill_flag = false;

    n_reduced_pixels = 0;

//    qDebug() << reduced_pixels->size();

    reduced_pixels->reserve(1, REDUCED_PIXELS_MAX_BYTES/sizeof(float));
    
    for (size_t i = 0; i < (size_t) files->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(files->size()));
            break;
        }

        // Project and correct file and get status
        if (n_reduced_pixels + 3 >= reduced_pixels->size())
        {
            // Break if there is too much data.
            emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Warning: There was too much data!"));
            kill_flag = true;
        }
        else
        {
            int STATUS_OK = projectFile(&(*files)[i]);
//            qDebug() << n_reduced_pixels;
//            emit changedImageSize(files->at(i).getFastDimension(), files->at(i).getSlowDimension());
//            (*files)[i].setActiveAngle(active_angle);
//            (*files)[i].setProjectionKernel(&project_kernel);
//            (*files)[i].setOffsetOmega(offset_omega);
//            (*files)[i].setOffsetKappa(offset_kappa);
//            (*files)[i].setOffsetPhi(offset_phi);

//            int STATUS_OK = (*files)[i].filterData( &n, reduced_pixels, threshold_reduce_low, threshold_reduce_high, threshold_project_low, threshold_project_high,1);
            if (!STATUS_OK)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: could not process data \""+files->at(i).getPath()+"\"");
                kill_flag = true;
            }
        }
        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/files->size());

    }
    size_t t = stopwatch.restart();

    reduced_pixels->resize(1, n_reduced_pixels);

//    qDebug() << "Reduced pixel size" << reduced_pixels->size();
//    reduced_pixels->print(2,"Projworker");
    /* Create dummy dataset for debugging purposes.
    */
    if (0) // A sphere
    {
        int theta_max = 180; // Up to 180
        int phi_max = 360; // Up to 360

        reduced_pixels->resize(1, theta_max*phi_max*4);

        float radius = 0.7;
        double pi = 4.0*std::atan(1.0);

        for (int i = 0; i < theta_max; i++)
        {
            for (int j = 0; j < phi_max; j++)
            {
                float theta = ((float) i)/180.0 * pi;
                float phi = ((float) j)/180.0 * pi;

                (*reduced_pixels)[(i*phi_max+j)*4+0] = radius * std::sin(theta)*std::cos(phi);
                (*reduced_pixels)[(i*phi_max+j)*4+1] = radius * std::sin(theta)*std::sin(phi);
                (*reduced_pixels)[(i*phi_max+j)*4+2] = radius * std::cos(theta);
                (*reduced_pixels)[(i*phi_max+j)*4+3] = 100;
            }
        }
    }
    else if (0) // A box
    {
        int res = 64;
        reduced_pixels->resize(1, res*res*res*4);

        for (int i = 0; i < res; i++)
        {
            for (int j = 0; j < res; j++)
            {
                for (int k = 0; k < res; k++)
                {
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+0] = (((float)i/(float)(res-1)) - 0.5)*2.0*0.5;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+1] = (((float)j/(float)(res-1)) - 0.5)*2.0*0.5;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+2] = (((float)k/(float)(res-1)) - 0.5)*2.0*0.5;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+3] = 100.0;
//                    (*reduced_pixels)[(i+j*res+k*res*res)*4+3] = (1.0 + std::sin(std::sqrt((float)(i*i+j*j+k*k))/std::sqrt((float)(3*res*res))*50))*1000;
                }
            }
        }
    }

    if (!kill_flag)
    {
        emit changedMessageString(" "+QString::number(files->size())+" files were processed successfully ("+QString::number((float)reduced_pixels->bytes()/(float)1000000.0, 'f', 3)+" MB) (" + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'f', 3)+" ms/file)");
    }

    emit finished();
}

int ProjectFileWorker::projectFile(DetectorFile * file)
{
    // Project and correct the data
    cl_image_format target_format;
    target_format.image_channel_order = CL_RGBA;
    target_format.image_channel_data_type = CL_FLOAT;

    // Prepare the target for storage of projected and corrected pixels (intensity but also xyz position)
    cl_mem xyzi_target_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        &target_format,
        file->getFastDimension(),
        file->getSlowDimension(),
        0,
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Load data into a CL texture
    cl_image_format source_format;
    source_format.image_channel_order = CL_INTENSITY;
    source_format.image_channel_data_type = CL_FLOAT;

    cl_mem source_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &source_format,
        file->getFastDimension(),
        file->getSlowDimension(),
        file->getFastDimension()*sizeof(cl_float),
        file->getData().data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // A sampler. The filtering should be CL_FILTER_NEAREST unless a linear interpolation of the data is actually what you want
    cl_sampler intensity_sampler = clCreateSampler(*context_cl->getContext(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Sample rotation matrix to be applied to each projected pixel to account for rotations. First set the active angle. Ideally this would be given by the header file, but for some reason it is not stated in there. Maybe it is just so normal to rotate around the omega angle to keep the resolution function consistent
    
    double phi, kappa, omega;
    if(active_angle == 0)
    {
        phi = file->start_angle + 0.5*file->angle_increment;
        kappa = file->kappa;
        omega = file->omega;
    }
    else if(active_angle == 1) 
    {
        phi = file->phi;
        kappa = file->start_angle + 0.5*file->angle_increment;
        omega = file->omega;
    }
    else if(active_angle == 2) 
    {
        phi = file->phi;
        kappa = file->kappa;
        omega = file->start_angle + 0.5*file->angle_increment;
    }
    
    RotationMatrix<double> PHI;
    RotationMatrix<double> KAPPA;
    RotationMatrix<double> OMEGA;
    RotationMatrix<double> sampleRotMat;

    file->alpha =  0.8735582;
    file->beta =  0.000891863;
    
    PHI.setArbRotation(file->beta, 0, -(phi+offset_phi)); 
    KAPPA.setArbRotation(file->alpha, 0, -(kappa+offset_kappa));
    OMEGA.setZRotation(-(omega+offset_omega));
    
    // The sample rotation matrix. Some rotations perturb the other rotation axes, and in the above calculations for phi, kappa, and omega we use fixed axes. It is therefore neccessary to put a rotation axis back into its basic position before the matrix is applied. In our case omega perturbs kappa and phi, and kappa perturbs phi. Thus we must first rotate omega back into the base position to recover the base rotation axis of kappa. Then we recover the base rotation axis for phi in the same manner. The order of matrix operations thus becomes:
    
    sampleRotMat = PHI*KAPPA*OMEGA;

    cl_mem sample_rotation_matrix_cl = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
        sampleRotMat.toFloat().bytes(),
        sampleRotMat.toFloat().data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // The sampler for cl_tsf_tex
    cl_sampler tsf_sampler = clCreateSampler(*context_cl->getContext(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Set kernel arguments
    err = clSetKernelArg(project_kernel, 0, sizeof(cl_mem), (void *) &xyzi_target_cl);
    err |= clSetKernelArg(project_kernel, 1, sizeof(cl_mem), (void *) &source_cl);
    err |= clSetKernelArg(project_kernel, 2, sizeof(cl_sampler), &tsf_sampler);
    err |= clSetKernelArg(project_kernel, 3, sizeof(cl_sampler), &intensity_sampler);
    err |= clSetKernelArg(project_kernel, 4, sizeof(cl_mem), (void *) &sample_rotation_matrix_cl);
    float threshold_one[2], threshold_two[2];
    threshold_one[0] = this->threshold_reduce_low;
    threshold_one[1] = this->threshold_reduce_high;
    threshold_two[0] = this->threshold_project_low;
    threshold_two[1] = this->threshold_project_high;
//    qDebug() << "Reduce" << threshold_reduce_low << threshold_reduce_high;
//    qDebug() << "Project" << threshold_project_low << threshold_project_high;
    
    err |= clSetKernelArg(project_kernel, 5, 2*sizeof(cl_float), threshold_one);
    err |= clSetKernelArg(project_kernel, 6, 2*sizeof(cl_float), threshold_two);
    err |= clSetKernelArg(project_kernel, 7, sizeof(cl_float), &file->background_flux);
    err |= clSetKernelArg(project_kernel, 8, sizeof(cl_float), &file->backgroundExpTime);
    err |= clSetKernelArg(project_kernel, 9, sizeof(cl_float), &file->pixel_size_x);
    err |= clSetKernelArg(project_kernel, 10, sizeof(cl_float), &file->pixel_size_y);
    err |= clSetKernelArg(project_kernel, 11, sizeof(cl_float), &file->exposure_time);
    err |= clSetKernelArg(project_kernel, 12, sizeof(cl_float), &file->wavelength);
    err |= clSetKernelArg(project_kernel, 13, sizeof(cl_float), &file->detector_distance);
    err |= clSetKernelArg(project_kernel, 14, sizeof(cl_float), &file->beam_x);
    err |= clSetKernelArg(project_kernel, 15, sizeof(cl_float), &file->beam_y);
    err |= clSetKernelArg(project_kernel, 16, sizeof(cl_float), &file->flux);
    err |= clSetKernelArg(project_kernel, 17, sizeof(cl_float), &file->start_angle);
    err |= clSetKernelArg(project_kernel, 18, sizeof(cl_float), &file->angle_increment);
    err |= clSetKernelArg(project_kernel, 19, sizeof(cl_float), &file->kappa);
    err |= clSetKernelArg(project_kernel, 20, sizeof(cl_float), &file->phi);
    err |= clSetKernelArg(project_kernel, 21, sizeof(cl_float), &file->omega);
    err |= clSetKernelArg(project_kernel, 22, sizeof(cl_float), &file->max_counts);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    /* Launch rendering kernel */
    size_t area_per_call[2] = {128, 128};
    size_t call_offset[2] = {0,0};
    size_t loc_ws[2];
    size_t glb_ws[2];
    
    loc_ws[0] = 16;
    loc_ws[1] = 16;
    glb_ws[0] = file->getFastDimension() + loc_ws[0] - (file->getFastDimension()%loc_ws[0]);
    glb_ws[1] = file->getSlowDimension() + loc_ws[1] - (file->getSlowDimension()%loc_ws[1]);
    
    for (size_t glb_x = 0; glb_x < glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), project_kernel, 2, call_offset, area_per_call, loc_ws, 0, NULL, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }
    clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Read the data
    size_t origin[3];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    size_t region[3];
    region[0] = file->getFastDimension();
    region[1] = file->getSlowDimension();
    region[2] = 1;

    Matrix<float> projected_data_buf(1,file->getFastDimension()*file->getSlowDimension()*4);
    err = clEnqueueReadImage ( *context_cl->getCommandQueue(), xyzi_target_cl, true, origin, region, 0, 0, projected_data_buf.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    if (xyzi_target_cl){
        err = clReleaseMemObject(xyzi_target_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (source_cl){
        err = clReleaseMemObject(source_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (sample_rotation_matrix_cl){
        err = clReleaseMemObject(sample_rotation_matrix_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (intensity_sampler){
        err = clReleaseSampler(intensity_sampler);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (tsf_sampler){
        err = clReleaseSampler(tsf_sampler);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

//    if (isProjectionActive)
//    {
    for (size_t i = 0; i < file->getFastDimension()*file->getSlowDimension(); i++)
    {
        if (projected_data_buf[i*4+3] > 0.0) // Above 0 check?
        {
//                if (projected_data_buf[i*4+3] < 0.0) qDebug() <<  projected_data_buf[i*4+3] << i;
            if ((n_reduced_pixels)+3 < reduced_pixels->size())
            {
                (*reduced_pixels)[(n_reduced_pixels)+0] = projected_data_buf[i*4+0];
                (*reduced_pixels)[(n_reduced_pixels)+1] = projected_data_buf[i*4+1];
                (*reduced_pixels)[(n_reduced_pixels)+2] = projected_data_buf[i*4+2];
                (*reduced_pixels)[(n_reduced_pixels)+3] = projected_data_buf[i*4+3];
                (n_reduced_pixels)+=4;
            }
            else
            {
                qDebug() << "TODO: send proper warning";
                return 0;
            }
        }
    }
//    }
    
    return 1;
}

/***
 *      dBBBBBb     dBP    dBP
 *           BB
 *       dBP BB   dBP    dBP
 *      dBP  BB  dBP    dBP
 *     dBBBBBBB dBBBBP dBBBBP
 *
 */


MultiWorker::MultiWorker()
{
    this->isCLInitialized = false;
}

MultiWorker::~MultiWorker()
{

}

void MultiWorker::process()
{
    QCoreApplication::processEvents();

    int verbose = 0;

    kill_flag = false;
    if (file_paths->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files have been specified");

        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Processing "+QString::number(file_paths->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit changedTabWidget(1);

    // Parameters for Ewald's projection
//    qDebug() << reduced_pixels->size();
    reduced_pixels->reserve(1, REDUCED_PIXELS_MAX_BYTES/sizeof(float));

    // Reset suggested values
    suggested_q = std::numeric_limits<float>::min();
    suggested_search_radius_low = std::numeric_limits<float>::max();
    suggested_search_radius_high = std::numeric_limits<float>::min();

    QElapsedTimer stopwatch;
    stopwatch.start();
    size_t n_ok_files = 0;
    n_reduced_pixels = 0;
    size_t size_raw = 0;
    
    for (size_t i = 0; i < (size_t) file_paths->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i+1)+" of "+QString::number(file_paths->size()));
            reduced_pixels->clear();

            break;
        }

        // Set file and get status
        DetectorFile file;
        int STATUS_OK = file.set(file_paths->at(i));

        if (STATUS_OK)
        {
            emit changedFile(file_paths->at(i));

            // Read file and get status
            int STATUS_OK = file.readData();
            if (STATUS_OK)
            {
                size_raw += file.getBytes();

                // Project and correct file and get status
                if (n_reduced_pixels > REDUCED_PIXELS_MAX_BYTES/sizeof(float))
                {
                    // Break if there is too much data.
                    emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Warning: There was too much data!"));
                    kill_flag = true;
                }
                else
                {
//                    emit changedImageSize(file.getFastDimension(), file.getSlowDimension());

//                    file.setActiveAngle(active_angle);
//                    file.setProjectionKernel(&project_kernel);
//                    file.setOffsetOmega(offset_omega);
//                    file.setOffsetKappa(offset_kappa);
//                    file.setOffsetPhi(offset_phi);

                    int STATUS_OK = projectFile(&file);
                    
                    if (STATUS_OK)
                    {
                        // Get suggestions on the minimum search radius that can safely be applied during interpolation
                        if (suggested_search_radius_low > file.getSearchRadiusLowSuggestion()) suggested_search_radius_low = file.getSearchRadiusLowSuggestion();
                        if (suggested_search_radius_high < file.getSearchRadiusHighSuggestion()) suggested_search_radius_high = file.getSearchRadiusHighSuggestion();

                        // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
                        if (suggested_q < file.getQSuggestion()) suggested_q = file.getQSuggestion();

                        n_ok_files++;
                    }
                    else
                    {
                        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: could not process data \""+files->at(i).getPath()+"\"");
                        kill_flag = true;
                    }
                }
            }
            else if (!STATUS_OK)
            {
                QString str("\n["+QString(this->metaObject()->className())+"] Warning: could not read \""+files->at(i).getPath()+"\"");

                emit changedMessageString(str);

                kill_flag = true;
            }
        }
        else
        {
            emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Warning: Could not set \""+QString(file_paths->at(i))+"\"");
        }

        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/file_paths->size());
    }
    reduced_pixels->resize(1, n_reduced_pixels);

//    reduced_pixels->print(2,"Multiworker");

    size_t t = stopwatch.restart();

    if (!kill_flag)
    {
        emit changedMessageString(" "+QString::number(n_ok_files)+" of "+QString::number(file_paths->size())+" files were successfully processed ("+QString::number(size_raw/1000000.0, 'f', 3)+" MB -> "+QString::number((float)reduced_pixels->bytes()/(float)1000000.0, 'f', 3)+" MB, " + QString::number(t) + " ms, "+QString::number((float)t/(float)n_ok_files, 'g', 3)+" ms/file)");

        // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
        float resolution_min = 2*suggested_q/suggested_search_radius_high;
        float resolution_max = 2*suggested_q/suggested_search_radius_low;

//        float level_min = std::log(resolution_min/(float)svo->getBrickInnerDimension())/std::log(2.0);
        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2.0);

        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
        if (verbose) emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggested minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Use at least octree level "+QString::number((int)level_max)+" to achieve good resolution");
    }

    emit qSpaceInfoChanged(suggested_search_radius_low, suggested_search_radius_high, suggested_q);
    emit finished();
}


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
}

VoxelizeWorker::~VoxelizeWorker()
{
    if (isCLInitialized) clReleaseKernel(voxelize_kernel);
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
    Matrix<const char *> paths(1,1);
    paths[0] = "kernels/voxelize.cl";

    program = context_cl->createProgram(&paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    voxelize_kernel = clCreateKernel(program, "voxelize", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    fill_kernel = clCreateKernel(program, "fill", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isCLInitialized = true;
}

void VoxelizeWorker::process()
{
    QCoreApplication::processEvents();
//qDebug() << "Before";
//    svo->print();

    kill_flag = false;
    qDebug() << suggested_q;
    if (reduced_pixels->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No data available!");
        emit changedMessageString(str);
        kill_flag = true;
    }
    if (!kill_flag)
    {
        // Emit to appropriate slots
        emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Creating Interpolation Data Structure: %p%"));

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Generating Sparse Voxel Octtree "+QString::number(svo->getLevels())+" levels deep.");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] The source data is "+QString::number(reduced_pixels->bytes()/1000000.0, 'g', 3)+" MB");

        // The number of data points in a single brick
        size_t n_points_brick = svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension();

        // The extent of the volume
        svo->setExtent(suggested_q);
        
        // The extent of the volume
        svo->setMetaData("You can write notes about the dataset here.");

        // Prepare the brick pool
        Matrix<int> pool_dimension(1, 4, 0);
        pool_dimension[0] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
        pool_dimension[1] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
        pool_dimension[2] = (BRICK_POOL_SOFT_MAX_BYTES/(sizeof(cl_float)*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) / ((1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower()));

        if (pool_dimension[2] < 1) pool_dimension[2] = 1;
        pool_dimension[2] *= svo->getBrickOuterDimension();

        size_t BRICK_POOL_HARD_MAX_BYTES = pool_dimension[0]*pool_dimension[1]*pool_dimension[2]*sizeof(float);

        size_t n_max_bricks = BRICK_POOL_HARD_MAX_BYTES/(n_points_brick*sizeof(float));

        cl_mem pool_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            BRICK_POOL_HARD_MAX_BYTES,
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        cl_mem pool_cluster_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                                nodes_per_kernel_call*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*sizeof(float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        cl_mem brick_extent_cl =  clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            nodes_per_kernel_call*6*sizeof(float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        cl_mem point_data_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            max_points_per_cluster*sizeof(cl_float4),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        cl_mem point_data_offset_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            nodes_per_kernel_call*sizeof(int),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                
        cl_mem point_data_count_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            nodes_per_kernel_call*sizeof(int),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                           
        Matrix<float> sum_check(1, nodes_per_kernel_call, 0);
        cl_mem sum_check_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            nodes_per_kernel_call*sizeof(float),
            sum_check.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        Matrix<float> variance_check(1, nodes_per_kernel_call, 0);
        cl_mem variance_check_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            nodes_per_kernel_call*sizeof(float),
            variance_check.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        // Generate an octtree data structure from which to construct bricks
        SearchNode root(NULL, svo->getExtent()->data());
//        root.setOpenCLContext(context_cl);
        
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
            /* Create an octtree from brick data. The nodes are maintained in a linear array rather than on the heap. This is due to lack of proper support for recursion on GPUs */
            Matrix<BrickNode> gpuHelpOcttree(1, n_max_bricks*16);
            gpuHelpOcttree[0].setParent(0);
            Matrix<unsigned int> nodes;
            nodes.set(1, 64, (unsigned int) 0);
            nodes[0] = 1;

            unsigned int nodes_prev_lvls = 0, non_empty_node_counter = 0;

            // Cycle through the levels
            QElapsedTimer timer;
            
            // Keep track of the maximum sum returned by a brick and use it later to estimate max value in data set
            float max_brick_sum = 0.0;
            
            // Containers 
            Matrix<double> brick_extent(1, 6*nodes_per_kernel_call);
            Matrix<float> point_data(max_points_per_cluster,4);
            Matrix<int> point_data_offset(1, nodes_per_kernel_call);
            Matrix<int> point_data_count(1, nodes_per_kernel_call);
            
            for (size_t lvl = 0; lvl < svo->getLevels(); lvl++)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl+1)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                // Find the correct range search radius
                float search_radius = sqrt(3.0f)*0.5f*((svo->getExtent()->at(1)-svo->getExtent()->at(0))/ (svo->getBrickInnerDimension()*(1 << lvl)));
                if (search_radius < suggested_search_radius_high) search_radius = suggested_search_radius_high;

                double tmp = (svo->getExtent()->at(1) - svo->getExtent()->at(0)) / (1 << lvl);

                // For each cluster of nodes
                for (size_t i = 0; i < nodes[lvl]; i += nodes_per_kernel_call)
                {
                    if((non_empty_node_counter+1) >= n_max_bricks)
                    {
                        QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed due to memory overflow. The dataset has grown too large! ("+QString::number(non_empty_node_counter*n_points_brick*sizeof(cl_float)/1e6, 'g', 3)+" MB)");
                        emit changedMessageString(str);
                        kill_flag = true;
                    }

                    if (kill_flag) break;
                        
                    // First pass: find relevant data for cluster of nodes
                    unsigned int currentId;
                    size_t accumulated_points = 0;
                    for (size_t j = 0; j < nodes_per_kernel_call; j++)
                    {
                        
                        // The id of the octnode in the octnode array
                        currentId = nodes_prev_lvls+i+j;
                        
                        // Set the level
                        gpuHelpOcttree[currentId].setLevel(lvl);

                        // Set the brick id
                        gpuHelpOcttree[currentId].calcBrickId((i+j)%8 ,&gpuHelpOcttree[gpuHelpOcttree[currentId].getParent()]);

                        // Based on brick id calculate the brick extent 
                        unsigned int * brick_id = gpuHelpOcttree[currentId].getBrickId();
                        
                        brick_extent[j*6 + 0] = svo->getExtent()->at(0) + tmp*brick_id[0];
                        brick_extent[j*6 + 1] = svo->getExtent()->at(0) + tmp*(brick_id[0]+1);
                        brick_extent[j*6 + 2] = svo->getExtent()->at(2) + tmp*brick_id[1];
                        brick_extent[j*6 + 3] = svo->getExtent()->at(2) + tmp*(brick_id[1]+1);
                        brick_extent[j*6 + 4] = svo->getExtent()->at(4) + tmp*brick_id[2];
                        brick_extent[j*6 + 5] = svo->getExtent()->at(4) + tmp*(brick_id[2]+1);
                        
                        // Offset of points accumulated thus far
                        point_data_offset[j] = accumulated_points;
                        
                        // Get point data needed for this brick 
                        root.getData(brick_extent.data() + j*6,
                                     point_data.data(),
                                     &accumulated_points,
                                     search_radius);
                        
                        // Number of points accumulated thus far
                        point_data_count[j] = accumulated_points - point_data_offset[j];
                        
                        // Upload this point data to an OpenCL buffer
                        if (point_data_count[j] > 0)
                        {
                            err = clEnqueueWriteBuffer(*context_cl->getCommandQueue(),
                                point_data_cl ,
                                CL_FALSE,
                                point_data_offset[j]*sizeof(cl_float4),
                                point_data_count[j]*sizeof(cl_float4),
                                point_data.data() + point_data_offset[j]*4,
                                0, NULL, NULL);
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                        }
                        
                        // Increment node
                        if (i + j + 1 >= nodes[lvl]) break;
                    }
                    
                    // Upload this extent to an OpenCL buffer
                    err = clEnqueueWriteBuffer(
                        *context_cl->getCommandQueue(),
                        brick_extent_cl ,
                        CL_FALSE,
                        0,
                        brick_extent.toFloat().bytes(),
                        brick_extent.toFloat().data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    // Upload other stuff before launcing the kernel
                    err = clEnqueueWriteBuffer(*context_cl->getCommandQueue(),
                        point_data_offset_cl ,
                        CL_FALSE,
                        0,
                        point_data_offset.bytes(),
                        point_data_offset.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    err = clEnqueueWriteBuffer(*context_cl->getCommandQueue(),
                        point_data_count_cl ,
                        CL_FALSE,
                        0,
                        point_data_count.bytes(),
                        point_data_count.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    err = clSetKernelArg( voxelize_kernel, 0, sizeof(cl_mem), (void *) &point_data_cl);
                    err |= clSetKernelArg( voxelize_kernel, 1, sizeof(cl_mem), (void *) &point_data_offset_cl);
                    err |= clSetKernelArg( voxelize_kernel, 2, sizeof(cl_mem), (void *) &point_data_count_cl);
                    err |= clSetKernelArg( voxelize_kernel, 3, sizeof(cl_mem), (void *) &brick_extent_cl);
                    err |= clSetKernelArg( voxelize_kernel, 4, sizeof(cl_mem), (void *) &pool_cluster_cl);
                    err |= clSetKernelArg( voxelize_kernel, 5, sizeof(cl_mem), (void *) &sum_check_cl);
                    err |= clSetKernelArg( voxelize_kernel, 6, sizeof(cl_mem), (void *) &variance_check_cl);
                    err |= clSetKernelArg( voxelize_kernel, 7, svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*sizeof(cl_float), NULL);
                    int tmp = svo->getBrickOuterDimension();
                    err |= clSetKernelArg( voxelize_kernel, 8, sizeof(cl_int), &tmp);
                    err |= clSetKernelArg( voxelize_kernel, 9, sizeof(cl_float), &search_radius);

                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    err = clFinish(*context_cl->getCommandQueue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                            
                    // Second pass: calculate the data for each node in the cluster (OpenCL)
                    for (size_t j = 0; j < nodes_per_kernel_call; j++)
                    {
                        // Launch kernel
                        size_t glb_offset[3] = {0,0,8*j};
                        size_t loc_ws[3] = {8,8,8};
                        size_t glb_ws[3] = {8,8,8};
                        err = clEnqueueNDRangeKernel(
                                    *context_cl->getCommandQueue(), 
                                    voxelize_kernel, 
                                    3, 
                                    glb_offset, 
                                    glb_ws, 
                                    loc_ws, 
                                    0, NULL, NULL);
                        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                        
                        if (i + j + 1 >= nodes[lvl]) break;
                    }
                    err = clFinish(*context_cl->getCommandQueue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    // Read relevant data
                    err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
                        sum_check_cl,
                        CL_TRUE,
                        0,
                        nodes_per_kernel_call*sizeof(float),
                        sum_check.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

                    err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
                        variance_check_cl,
                        CL_TRUE,
                        0,
                        nodes_per_kernel_call*sizeof(float),
                        variance_check.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    err = clFinish(*context_cl->getCommandQueue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    // Third pass: transfer non-empty nodes to svo data structure (OpenCL)
                    for (size_t j = 0; j < nodes_per_kernel_call; j++)
                    {
                        // The id of the octnode in the octnode array
                        currentId = nodes_prev_lvls+i+j;
                        
                        // If a node simply has no data
                        if ((sum_check[j] <= 0.0))
                        {
                            gpuHelpOcttree[currentId].setDataFlag(0);
                            gpuHelpOcttree[currentId].setMsdFlag(1);
                            gpuHelpOcttree[currentId].setChild(0);
                        }
                        // Else a node has data
                        else
                        {
                            if (non_empty_node_counter + 1 >= n_max_bricks)
                            {
                                emit popup(QString("Warning - Data Overflow"), QString("The dataset you are trying to create grew too large. This event occured at level X. The data exceeded the limit of Y MB. The issue can be remedied by increasing the lower thresholds."));
                                kill_flag = true;
                                break;
                            }
                            
                            // Else the node is a parent with children
                            else
                            {
                                gpuHelpOcttree[currentId].setDataFlag(1);
                                gpuHelpOcttree[currentId].setMsdFlag(0);

                                if (lvl >= svo->getLevels() - 1) gpuHelpOcttree[currentId].setMsdFlag(1);

                                // Account for children
                                if (!gpuHelpOcttree[currentId].getMsdFlag())
                                {
                                    unsigned int childId = nodes_prev_lvls + nodes[lvl] + nodes[lvl+1];
                                    gpuHelpOcttree[currentId].setChild(childId); // Index points to first child only

                                    // For each child
                                    for (size_t k = 0; k < 8; k++)
                                    {
                                        gpuHelpOcttree[childId+k].setParent(currentId);
                                        nodes[lvl+1]++;
                                    }
                                }
                            }

                            // Set the pool id of the brick corresponding to the node
                            gpuHelpOcttree[currentId].calcPoolId(svo->getBrickPoolPower(), non_empty_node_counter);

                            // Find the max sum of a brick
                            if (sum_check[j] > max_brick_sum) max_brick_sum = sum_check[j];

                            
                            // Transfer brick data to pool
                            err = clSetKernelArg( fill_kernel, 0, sizeof(cl_mem), (void *) &pool_cluster_cl);
                            err |= clSetKernelArg( fill_kernel, 1, sizeof(cl_mem), (void *) &pool_cl);
                            err |= clSetKernelArg( fill_kernel, 2, sizeof(cl_int4), pool_dimension.data());
                            int tmp = svo->getBrickOuterDimension();
                            err |= clSetKernelArg( fill_kernel, 3, sizeof(cl_uint), &tmp);
                            err |= clSetKernelArg( fill_kernel, 4, sizeof(cl_uint), &non_empty_node_counter);
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                            
                            // Launch kernel to write data to pool
                            size_t glb_offset[3] = {0,0,8*j};
                            size_t loc_ws[3] = {8,8,8};
                            size_t glb_ws[3] = {8,8,8};
                            err = clEnqueueNDRangeKernel(
                                        *context_cl->getCommandQueue(), 
                                        fill_kernel, 
                                        3, 
                                        glb_offset, 
                                        glb_ws, 
                                        loc_ws, 
                                        0, NULL, NULL);
                            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                            
                            
                            non_empty_node_counter++;
                            
                            err = clFinish(*context_cl->getCommandQueue());
                            if ( err != CL_SUCCESS)
                            {
                                qFatal(cl_error_cstring(err));
                            }
                        }
                        if (i + j + 1 >= nodes[lvl]) break;
                    }

                    emit changedGenericProgress((i+1)*100/nodes[lvl]);
                }
                if (kill_flag)
                {

                    QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(lvl+1)+" of "+QString::number(svo->getLevels())+"!");

                    emit changedMessageString(str);
                    break;
                }

                nodes_prev_lvls += nodes[lvl];
                
                size_t t = timer.restart();
                emit changedMessageString(" ...done ("+QString::number(t)+" ms)");
            }
            
            
            
            if (!kill_flag)
            {
                // Use the node structure to populate the GPU arrays
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Transforming: %p%"));
                svo->index.reserve(1, nodes_prev_lvls);
                svo->brick.reserve(1, nodes_prev_lvls);

                for (size_t i = 0; i < nodes_prev_lvls; i++)
                {
                    svo->index[i] = getOctIndex(gpuHelpOcttree[i].getMsdFlag(), gpuHelpOcttree[i].getDataFlag(), gpuHelpOcttree[i].getChild());
                    svo->brick[i] = getOctBrick(gpuHelpOcttree[i].getPoolId()[0], gpuHelpOcttree[i].getPoolId()[1], gpuHelpOcttree[i].getPoolId()[2]);

                    emit changedGenericProgress((i+1)*100/nodes_prev_lvls);
                }

                // Round up to the lowest number of bricks that is multiple of the brick pool dimensions. Use this value to reserve data for the data pool
                unsigned int non_empty_node_counter_rounded_up = non_empty_node_counter + ((pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) - (non_empty_node_counter % (pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension()))));

                // Read results
                svo->pool.reserve(1, non_empty_node_counter_rounded_up*n_points_brick);
                
                svo->setMin(0.0f);
                svo->setMax(max_brick_sum/(float)(n_points_brick));
                
                err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
                    pool_cl,
                    CL_TRUE,
                    0,
                    non_empty_node_counter_rounded_up*n_points_brick*sizeof(float),
                    svo->pool.data(),
                    0, NULL, NULL);
                if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            }

            if (!kill_flag)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Finished successfully. The dataset consists of "+QString::number(nodes_prev_lvls)+" bricks and is approxiamtely "+QString::number((svo->getBytes())/1e6, 'g', 3)+" MB\nThe dataset can now be saved");

            }
        }



        clReleaseMemObject(point_data_cl);
        clReleaseMemObject(point_data_offset_cl);
        clReleaseMemObject(point_data_count_cl);
        clReleaseMemObject(brick_extent_cl);
        clReleaseMemObject(pool_cluster_cl);
        clReleaseMemObject(pool_cl);
        clReleaseMemObject(sum_check_cl);
    }
//    qDebug() << "After";
//        svo->print();
    emit finished();
}

