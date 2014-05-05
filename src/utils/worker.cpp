#include "worker.h"

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

void BaseWorker::setQSpaceInfo(float * suggested_search_radius_low, float * suggested_search_radius_high, float * suggested_q)
{

    this->suggested_search_radius_low = suggested_search_radius_low;
    this->suggested_search_radius_high = suggested_search_radius_high;
    this->suggested_q = suggested_q;
}


void BaseWorker::setFiles(QList<DetectorFile> * files)
{

    this->files = files;
}
void BaseWorker::setReducedPixels(Matrix<float> * reduced_pixels)
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
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"]  Warning: process killed after finding "+QString::number(i)+" files");
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
    kill_flag = false;

    if (file_paths->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No paths specified!");

        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Setting "+QString::number(file_paths->size())+" files (headers etc.)...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit changedTabWidget(0);

    // Reset suggested values
    (*suggested_q) = std::numeric_limits<float>::min();
    (*suggested_search_radius_low) = std::numeric_limits<float>::max();
    (*suggested_search_radius_high) = std::numeric_limits<float>::min();

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
            QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i)+" of "+QString::number(file_paths->size())+"!");

            emit changedMessageString(str);
            files->clear();

            break;
        }

        // Set file and get status
        files->append(DetectorFile());
        int STATUS_OK = files->back().set(file_paths->at(i), context_cl);
        if (STATUS_OK)
        {
            // Get suggestions on the minimum search radius that can safely be applied during interpolation
            if ((*suggested_search_radius_low) > files->back().getSearchRadiusLowSuggestion()) (*suggested_search_radius_low) = files->back().getSearchRadiusLowSuggestion();
            if ((*suggested_search_radius_high) < files->back().getSearchRadiusHighSuggestion()) (*suggested_search_radius_high) = files->back().getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if ((*suggested_q) < files->back().getQSuggestion()) (*suggested_q) = files->back().getQSuggestion();
            
            emit changedFile(files->size()-1);
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
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(files->size())+" of "+QString::number(file_paths->size())+" files were successfully set (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");

        // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
        float resolution_min = 2*(*suggested_q)/(*suggested_search_radius_high);
        float resolution_max = 2*(*suggested_q)/(*suggested_search_radius_low);

        float level_min = std::log(resolution_min/(float)svo->getBrickInnerDimension())/std::log(2.0);
        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2.0);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number((*suggested_q), 'g', 3)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number((*suggested_search_radius_low), 'g', 2)+" to "+QString::number((*suggested_search_radius_high), 'g', 2)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggesting minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggesting minimum octtree level: "+QString::number(level_min, 'f', 2)+" to "+QString::number(level_max, 'f', 2)+"");
    }

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

void ReadFileWorker::test()
{
    qDebug("Test");
}

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
            QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");
            emit changedMessageString(str);
            break;
        }

        // Read file and get status
        int STATUS_OK = (*files)[i].readData();
        size_raw += (*files)[i].getBytes();
        
        emit changedFile(i);
        
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
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(files->size())+" files were successfully read ("+QString::number(size_raw/1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");
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
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No files specified!");

        emit changedMessageString(str);

        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Correcting and Projecting "+QString::number(files->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));

    QElapsedTimer stopwatch;
    stopwatch.start();
    kill_flag = false;

    size_t n = 0;
    reduced_pixels->reserve(1, REDUCED_PIXELS_MAX_BYTES/sizeof(float));
    
    for (size_t i = 0; i < (size_t) files->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");

            emit changedMessageString(str);
            break;
        }

        // Project and correct file and get status
        if (n > reduced_pixels->size())
        {
            // Break if there is too much data.
            emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Warning: There was too much data!"));
            kill_flag = true;
        }
        else
        {
            emit changedImageSize(files->at(i).getWidth(), files->at(i).getHeight());
            (*files)[i].setActiveAngle(active_angle);
            (*files)[i].setProjectionKernel(&project_kernel);
            (*files)[i].setOffsetOmega(offset_omega);
            (*files)[i].setOffsetKappa(offset_kappa);
            (*files)[i].setOffsetPhi(offset_phi);

            int STATUS_OK = (*files)[i].filterData( &n, reduced_pixels->data(), threshold_reduce_low, threshold_reduce_high, threshold_project_low, threshold_project_high,1);
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

    reduced_pixels->resize(1, n);

    /* Create dummy dataset for debugging purposes.
    */
    if (0) // A sphere
    {
        int theta_max = 180; // Up to 180
        int phi_max = 360; // Up to 360

        reduced_pixels->resize(1, theta_max*phi_max*4);

        float radius = 1.15;
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
                (*reduced_pixels)[(i*phi_max+j)*4+3] = 10;
            }
        }
    }
    else if (0) // A gradiented box
    {
        int res = 32;
        reduced_pixels->resize(1, res*res*res*4);

        for (int i = 0; i < res; i++)
        {
            for (int j = 0; j < res; j++)
            {
                for (int k = 0; k < res; k++)
                {
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+0] = (((float)i/(float)(res-1)) - 0.5)*2.0*1.25;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+1] = (((float)j/(float)(res-1)) - 0.5)*2.0*1.25;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+2] = (((float)k/(float)(res-1)) - 0.5)*2.0*1.25;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+3] = (1.0 + std::sin(std::sqrt((float)(i*i+j*j+k*k))/std::sqrt((float)(3*res*res))*50))*1000;
                }
            }
        }
    }

    if (!kill_flag)
    {
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(files->size())+" files were successfully projected and merged ("+QString::number((float)reduced_pixels->bytes()/(float)1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");
    }

    emit finished();
}



/***
 *      dBBBBBb     dBP    dBP
 *           BB
 *       dBP BB   dBP    dBP
 *      dBP  BB  dBP    dBP
 *     dBBBBBBB dBBBBP dBBBBP
 *
 */


AllInOneWorker::AllInOneWorker()
{
    this->isCLInitialized = false;
}

AllInOneWorker::~AllInOneWorker()
{

}

void AllInOneWorker::process()
{
    QCoreApplication::processEvents();

    kill_flag = false;
    if (file_paths->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Warning: No paths specified!");

        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Treating "+QString::number(file_paths->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit changedTabWidget(1);

    // Parameters for Ewald's projection
    reduced_pixels->reserve(1, REDUCED_PIXELS_MAX_BYTES/sizeof(float));

    // Reset suggested values
    (*suggested_q) = std::numeric_limits<float>::min();
    (*suggested_search_radius_low) = std::numeric_limits<float>::max();
    (*suggested_search_radius_high) = std::numeric_limits<float>::min();

    QElapsedTimer stopwatch;
    stopwatch.start();
    size_t n_ok_files = 0;
    size_t n = 0;
    size_t size_raw = 0;
    
    for (size_t i = 0; i < (size_t) file_paths->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i)+" of "+QString::number(file_paths->size())+"!");

            emit changedMessageString(str);
            reduced_pixels->clear();

            break;
        }

        // Set file and get status
        DetectorFile file;
        int STATUS_OK = file.set(file_paths->at(i), context_cl);

        if (STATUS_OK)
        {
            // Read file and get status
            int STATUS_OK = file.readData();
            if (STATUS_OK)
            {
                size_raw += file.getBytes();

                // Project and correct file and get status
                if (n > REDUCED_PIXELS_MAX_BYTES/sizeof(float))
                {
                    // Break if there is too much data.
                    emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Warning: There was too much data!"));
                    kill_flag = true;
                }
                else
                {
                    emit changedImageSize(file.getWidth(), file.getHeight());

                    file.setActiveAngle(active_angle);
                    file.setProjectionKernel(&project_kernel);
                    file.setOffsetOmega(offset_omega);
                    file.setOffsetKappa(offset_kappa);
                    file.setOffsetPhi(offset_phi);

                    int STATUS_OK = file.filterData( &n, reduced_pixels->data(), threshold_reduce_low, threshold_reduce_high, threshold_project_low, threshold_project_high,1);
                    
                    if (STATUS_OK)
                    {
                        // Get suggestions on the minimum search radius that can safely be applied during interpolation
                        if ((*suggested_search_radius_low) > file.getSearchRadiusLowSuggestion()) (*suggested_search_radius_low) = file.getSearchRadiusLowSuggestion();
                        if ((*suggested_search_radius_high) < file.getSearchRadiusHighSuggestion()) (*suggested_search_radius_high) = file.getSearchRadiusHighSuggestion();

                        // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
                        if ((*suggested_q) < file.getQSuggestion()) (*suggested_q) = file.getQSuggestion();

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
    reduced_pixels->resize(1, n);

    size_t t = stopwatch.restart();

    if (!kill_flag)
    {
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(n_ok_files)+" of "+QString::number(file_paths->size())+" files were successfully set (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)n_ok_files, 'g', 3)+" ms/file)");

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(n_ok_files)+" files were successfully read ("+QString::number(size_raw/1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)n_ok_files, 'g', 3)+" ms/file)");

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(n_ok_files)+" files were successfully projected and merged ("+QString::number((float)reduced_pixels->bytes()/(float)1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)n_ok_files, 'g', 3)+" ms/file)");

        // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
        float resolution_min = 2*(*suggested_q)/(*suggested_search_radius_high);
        float resolution_max = 2*(*suggested_q)/(*suggested_search_radius_low);

        float level_min = std::log(resolution_min/(float)svo->getBrickInnerDimension())/std::log(2.0);
        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2.0);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number((*suggested_q), 'g', 3)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number((*suggested_search_radius_low), 'g', 2)+" to "+QString::number((*suggested_search_radius_high), 'g', 2)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggesting minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Suggesting minimum octtree level: "+QString::number(level_min, 'f', 2)+" to "+QString::number(level_max, 'f', 2)+"");
    }

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

    kill_flag = false;

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
        svo->setExtent(*suggested_q);
        
        svo->print();
        
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
                           
        Matrix<float> empty_check(1, nodes_per_kernel_call, 0);
        cl_mem empty_check_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            nodes_per_kernel_call*sizeof(float),
            empty_check.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        // Generate an octtree data structure from which to construct bricks
        SearchNode root(NULL, svo->getExtent()->data());
        root.setOpenCLContext(context_cl);
        
        for (size_t i = 0; i < reduced_pixels->size()/4; i++)
        {
            if (kill_flag)
            {
                QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i)+" of "+QString::number(reduced_pixels->size()/4)+"!");

                emit changedMessageString(str);
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

            unsigned int confirmed_nodes = 0, non_empty_node_counter = 0;

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
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                // Find the correct range search radius
                float search_radius = sqrt(3.0f)*0.5f*((svo->getExtent()->at(1)-svo->getExtent()->at(0))/ (svo->getBrickInnerDimension()*(1 << lvl)));
                if (search_radius < (*suggested_search_radius_high)) search_radius = (*suggested_search_radius_high);

                double tmp = (svo->getExtent()->at(1) - svo->getExtent()->at(0)) / (1 << lvl);

                // For each cluster of nodes
                size_t iter = 0;
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
                        currentId = confirmed_nodes+i+j;
                        
                        // Set the level
                        gpuHelpOcttree[currentId].setLevel(lvl);

                        // Set the brick id
                        gpuHelpOcttree[currentId].calcBrickId((i+j)%8 ,&gpuHelpOcttree[gpuHelpOcttree[currentId].getParent()]);

                        // Based on brick id calculate the brick extent 
                        unsigned int * brickId = gpuHelpOcttree[currentId].getBrickId();
                        
                        brick_extent[j*6 + 0] = svo->getExtent()->at(0) + tmp*brickId[0];
                        brick_extent[j*6 + 1] = svo->getExtent()->at(0) + tmp*(brickId[0]+1);
                        brick_extent[j*6 + 2] = svo->getExtent()->at(2) + tmp*brickId[1];
                        brick_extent[j*6 + 3] = svo->getExtent()->at(2) + tmp*(brickId[1]+1);
                        brick_extent[j*6 + 4] = svo->getExtent()->at(4) + tmp*brickId[2];
                        brick_extent[j*6 + 5] = svo->getExtent()->at(4) + tmp*(brickId[2]+1);
                        
                        // Offset of points accumulated thus far
                        point_data_offset[j] = accumulated_points;
                        
                        // Get point data needed for this brick 
                        root.getData(brick_extent.data() + j*6,
                                     point_data.data(),
                                     &accumulated_points,
                                     search_radius);
                        
//                        for (size_t k = point_data_offset[j]; k < accumulated_points; k++)
//                        {
//                            if (point_data[k] < 0)
//                            {
//                                qDebug() << "-- Abnormal data point detected in node" << i+j;
//                                qDebug() << "point_data[" << k << "] =" << point_data[k];
//                                qDebug() << "point_data_offset[j] =" << point_data_offset[j];
//                                qDebug() << "accumulated_points =" << accumulated_points;
//                            }
//                        }

                        /* TODO:
                         * Check how self-similar the data is. If it is deemed sufficiently self-similar, set the corresponding node's max subdivision flag to true.
                         * */

                        if (accumulated_points - point_data_offset[j] > 0)
                        {
                            // Find the average
                            double average = 0;
                            double count = 0;
                            for (size_t k = point_data_offset[j]; k < accumulated_points; k++)
                            {
                                average += point_data[k*4+3];
                                count += 1.0;
//                                qDebug() << point_data[k];
//                                if (point_data[k] < 0)
//                                {
//                                    qDebug() << "Abnormal data point detected";
//                                    qDebug() << "point_data[" << k << "] =" << point_data[k];
//                                    qDebug() << "point_data_offset[j] =" << point_data_offset[j];
//                                    qDebug() << "accumulated_points =" << accumulated_points;
//                                }
                            }

                            average /= count;

                            // Find the most deviating point of data
                            double max_deviation = 0;
                            for (size_t k = point_data_offset[j]; k < accumulated_points; k++)
                            {
                                double deviation = fabs(average - point_data[k*4+3]);
                                if (deviation > max_deviation) max_deviation = deviation;
                            }

                            // The relative magnitude of the deviation
                            double magnitude = max_deviation/average;

//                            qDebug() << magnitude << max_deviation << average << accumulated_points - point_data_offset[j];
//                            if (accumulated_points - point_data_offset[j] == 1) qDebug() << point_data[point_data_offset[j]];

                            // Save the result so it can be used later to determine if a node is self-similar.


                        }




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
                    err |= clSetKernelArg( voxelize_kernel, 5, sizeof(cl_mem), (void *) &empty_check_cl);
                    err |= clSetKernelArg( voxelize_kernel, 6, svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*sizeof(cl_float), NULL);
                    int tmp = svo->getBrickOuterDimension();
                    err |= clSetKernelArg( voxelize_kernel, 7, sizeof(cl_int), &tmp);
                    err |= clSetKernelArg( voxelize_kernel, 8, sizeof(cl_float), &search_radius);
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
                    
                    // Read currently relevant data
                    err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
                        empty_check_cl,
                        CL_TRUE,
                        0,
                        nodes_per_kernel_call*sizeof(float),
                        empty_check.data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    err = clFinish(*context_cl->getCommandQueue());
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                    
                    
                    // Third pass: transfer non-empty nodes to svo data structure (OpenCL)
                    for (size_t j = 0; j < nodes_per_kernel_call; j++)
                    {
                        // The id of the octnode in the octnode array
                        currentId = confirmed_nodes+i+j;
                        
                        if (empty_check[j] <= 0.0)
                        {
                            gpuHelpOcttree[currentId].setDataFlag(0);
                            gpuHelpOcttree[currentId].setMsdFlag(1);
                            gpuHelpOcttree[currentId].setChild(0);
                        }
                        else
                        {
                            if (non_empty_node_counter + 1 >= n_max_bricks)
                            {
                                emit popup(QString("Warning - Data Overflow"), QString("The dataset you are trying to create grew too large. This event occured at level X. The data exceeded the limit of Y MB. The issue can be remedied by increasing the lower thresholds."));
                                kill_flag = true;
                                break;
                            }
                            
                            if (empty_check[j] > max_brick_sum) max_brick_sum = empty_check[j];
                            
                            gpuHelpOcttree[currentId].setDataFlag(1);
                            gpuHelpOcttree[currentId].setMsdFlag(0);
                            if (lvl >= svo->getLevels() - 1) gpuHelpOcttree[currentId].setMsdFlag(1);
                            gpuHelpOcttree[currentId].calcPoolId(svo->getBrickPoolPower(), non_empty_node_counter);
                            if (!gpuHelpOcttree[currentId].getMsdFlag())
                            {
                                unsigned int childId = confirmed_nodes + nodes[lvl] + iter*8;
                                gpuHelpOcttree[currentId].setChild(childId);
    
                                // For each child
                                for (size_t k = 0; k < 8; k++)
                                {
                                    gpuHelpOcttree[childId+k].setParent(currentId);
                                    nodes[lvl+1]++;
                                }
                            }
                            
                            // Transfer data to pool
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
                            iter++;
                            
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

                    QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(lvl)+" of "+QString::number(svo->getLevels())+"!");

                    emit changedMessageString(str);
                    break;
                }

                confirmed_nodes += nodes[lvl];
                
                size_t t = timer.restart();
                emit changedMessageString(" ...done (time: "+QString::number(t)+" ms)");
            }
            
            
            
            if (!kill_flag)
            {
                // Use the node structure to populate the GPU arrays
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Transforming: %p%"));
                svo->index.reserve(1, confirmed_nodes);
                svo->brick.reserve(1, confirmed_nodes);

                for (size_t i = 0; i < confirmed_nodes; i++)
                {
                    svo->index[i] = getOctIndex(gpuHelpOcttree[i].getMsdFlag(), gpuHelpOcttree[i].getDataFlag(), gpuHelpOcttree[i].getChild());
                    svo->brick[i] = getOctBrick(gpuHelpOcttree[i].getPoolId()[0], gpuHelpOcttree[i].getPoolId()[1], gpuHelpOcttree[i].getPoolId()[2]);

                    emit changedGenericProgress((i+1)*100/confirmed_nodes);
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
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Total size (uncompressed): "+QString::number((svo->getBytes())/1e6, 'g', 3)+" MB");
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Number of bricks: "+QString::number(confirmed_nodes));
            }
        }

        clReleaseMemObject(point_data_cl);
        clReleaseMemObject(point_data_offset_cl);
        clReleaseMemObject(point_data_count_cl);
        clReleaseMemObject(brick_extent_cl);
        clReleaseMemObject(pool_cluster_cl);
        clReleaseMemObject(pool_cl);
        clReleaseMemObject(empty_check_cl);
    }

    emit finished();
}

