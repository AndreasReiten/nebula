#include "worker.h"
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
{
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
}

BaseWorker::~BaseWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
}

void BaseWorker::writeLog(QString str)
{
    writeToLogAndPrint(str.toStdString().c_str(), "riv.log", 1);
}

void BaseWorker::setOpenCLContext(cl_device * device, cl_context * context, cl_command_queue * queue)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->device = device;
    this->context = context;
    this->queue = queue;
}

void BaseWorker::setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->alpha_img_clgl = alpha_img_clgl;
    this->beta_img_clgl = beta_img_clgl;
    this->gamma_img_clgl = gamma_img_clgl;
    this->tsf_img_clgl = tsf_img_clgl;
}

void BaseWorker::setReduceThresholdLow(float * value)
{
    this->threshold_reduce_low = value;
}
void BaseWorker::setReduceThresholdHigh(float * value)
{
    this->threshold_reduce_high = value;
}
void BaseWorker::setProjectThresholdLow(float * value)
{
    this->threshold_project_low = value;
}
void BaseWorker::setProjectThresholdHigh(float * value)
{
    this->threshold_project_high = value;
}

void BaseWorker::killProcess()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    kill_flag = true;
}

void BaseWorker::setFilePaths(QStringList * file_paths)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->file_paths = file_paths;
}
void BaseWorker::setBrickInfo(int brick_inner_dimension, int brick_outer_dimension)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->brick_inner_dimension = brick_inner_dimension;
    this->brick_outer_dimension = brick_outer_dimension;
}
void BaseWorker::setFiles(QList<PilatusFile> * files)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->files = files;
}
void BaseWorker::setReducedPixels(MiniArray<float> * reduced_pixels)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->reduced_pixels = reduced_pixels;
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
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
}

SetFileWorker::~SetFileWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
}

void SetFileWorker::process()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    QCoreApplication::processEvents();
    kill_flag = false;

    if (file_paths->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No paths specified!");
        emit writeLog(str);
        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Setting "+QString::number(file_paths->size())+" files (headers etc.)...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit enableSetFileButton(false);
    emit enableReadFileButton(false);
    emit enableProjectFileButton(false);
    emit enableVoxelizeButton(false);
    emit enableAllInOneButton(false);
    emit showGenericProgressBar(true);
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
        //--QCoreApplication::processEvents();
        if (kill_flag)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(file_paths->size())+"!");
            emit writeLog(str);
            emit changedMessageString(str);
            files->clear();

            break;
        }

        // Set file and get status
        files->append(PilatusFile());
        int STATUS_OK = files->back().set(file_paths->at(i), context, queue);
        files->back().setOpenCLBuffers(alpha_img_clgl, beta_img_clgl, gamma_img_clgl, tsf_img_clgl);
        if (STATUS_OK)
        {
            // Get suggestions on the minimum search radius that can safely be applied during interpolation
            if (suggested_search_radius_low > files->back().getSearchRadiusLowSuggestion()) suggested_search_radius_low = files->back().getSearchRadiusLowSuggestion();
            if (suggested_search_radius_high < files->back().getSearchRadiusHighSuggestion()) suggested_search_radius_high = files->back().getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if (suggested_q < files->back().getQSuggestion()) suggested_q = files->back().getQSuggestion();
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

    emit enableSetFileButton(true);
    emit enableAllInOneButton(true);
    emit showGenericProgressBar(false);

    if (!kill_flag)
    {
        emit enableReadFileButton(true);
        emit enableProjectFileButton(false);
        emit enableVoxelizeButton(false);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] "+QString::number(files->size())+" of "+QString::number(file_paths->size())+" files were successfully set (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");

        // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
        float resolution_min = 2*suggested_q/suggested_search_radius_high;
        float resolution_max = 2*suggested_q/suggested_search_radius_low;

        float level_min = std::log(resolution_min/(float)this->brick_inner_dimension)/std::log(2);
        float level_max = std::log(resolution_max/(float)this->brick_inner_dimension)/std::log(2);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
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
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
}

ReadFileWorker::~ReadFileWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
}

void ReadFileWorker::process()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    QCoreApplication::processEvents();

    if (files->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No files specified!");
        emit writeLog(str);
        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Reading "+QString::number(files->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit enableSetFileButton(false);
    emit enableReadFileButton(false);
    emit enableProjectFileButton(false);
    emit enableVoxelizeButton(false);
    emit enableAllInOneButton(false);
    emit showGenericProgressBar(true);
    emit changedTabWidget(0);


    QElapsedTimer stopwatch;
    stopwatch.start();
    kill_flag = false;
    size_t size_raw = 0;

    for (size_t i = 0; i < (size_t) files->size(); i++)
    {
        // Kill process if requested
        //--QCoreApplication::processEvents();
        if (kill_flag)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");
            emit writeLog(str);
            emit changedMessageString(str);
            break;
        }

        // Read file and get status
        int STATUS_OK = (*files)[i].readData();
        size_raw += (*files)[i].getBytes();
        if (!STATUS_OK)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Error: could not read \""+files->at(i).getPath()+"\"");
            emit writeLog(str);
            emit changedMessageString(str);

            kill_flag = true;
        }

        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/files->size());
    }
    size_t t = stopwatch.restart();

    emit enableSetFileButton(true);
    emit enableReadFileButton(true);
    emit enableAllInOneButton(true);
    emit showGenericProgressBar(false);

    if (!kill_flag)
    {
        emit enableProjectFileButton(true);
        emit enableVoxelizeButton(false);
        emit enableAllInOneButton(true);

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
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
}

ProjectFileWorker::~ProjectFileWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    if (isCLInitialized && projection_kernel) clReleaseKernel(projection_kernel);
}

void ProjectFileWorker::initializeCLKernel()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    // Program
    QByteArray qsrc = open_resource(":/src/kernels/frameFilter.cl");
    const char * src = qsrc.data();
    size_t src_length = strlen(src);

    program = clCreateProgramWithSource((*context), 1, (const char **)&src, &src_length, &err);
    if (err != CL_SUCCESS)
    {
        QString str("Error in "+QString(this->metaObject()->className())+": Could not create program from source: "+QString(cl_error_cstring(err)));
        std::cout << str.toStdString().c_str() << std::endl;
        emit writeLog(str);
        return;
    }
    // Compile kernel
    const char * options = "-cl-single-precision-constant -cl-mad-enable -cl-fast-relaxed-math";
    err = clBuildProgram(program, 1, &device->device_id, options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // Compile log
        char* build_log;
        size_t log_size;
        clGetProgramBuildInfo(program, device->device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        build_log = new char[log_size+1];
        clGetProgramBuildInfo(program, device->device_id, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
        build_log[log_size] = '\0';

        QString str("Error in "+QString(this->metaObject()->className())+
            ": Could not compile/link program: "+
            QString(cl_error_cstring(err))+
            "\n--- START KERNEL COMPILE LOG ---"+
            QString(build_log)+
            "\n---  END KERNEL COMPILE LOG  ---");
        std::cout << str.toStdString().c_str() << std::endl;
        emit writeLog(str);
        delete[] build_log;
        return;
    }

    // Entry point
    projection_kernel = clCreateKernel(program, "FRAME_FILTER", &err);
    if (err != CL_SUCCESS)
    {
        QString str("Error in "+QString(this->metaObject()->className())+": Could not create kernel object: "+QString(cl_error_cstring(err)));
        std::cout << str.toStdString().c_str() << std::endl;
        emit writeLog(str);
        return;
    }
}

void ProjectFileWorker::process()
{
    /* For each file, project the detector coordinate and corresponding intensity down onto the Ewald sphere. Intensity corrections are also carried out in this step. The header of each file should include all the required information to to the transformations. The result is stored in a seprate container. There are different file formats, and all files coming here should be of the same base type. */

    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    QCoreApplication::processEvents();

    if (files->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No files specified!");
        emit writeLog(str);
        emit changedMessageString(str);

        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Correcting and Projecting "+QString::number(files->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
    emit enableSetFileButton(false);
    emit enableReadFileButton(false);
    emit enableProjectFileButton(false);
    emit enableVoxelizeButton(false);
    emit enableAllInOneButton(false);
    emit showGenericProgressBar(true);

    Matrix<float> test_background;
    test_background.set(1679, 1475, 0.0);
    QElapsedTimer stopwatch;
    stopwatch.start();
    kill_flag = false;
    size_t n = 0;
    size_t limit = 0.25e9;
    reduced_pixels->reserve(limit);

    for (size_t i = 0; i < (size_t) files->size(); i++)
    {
        // Kill process if requested
        //--QCoreApplication::processEvents();
        if (kill_flag)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");
            emit writeLog(str);
            emit changedMessageString(str);
            break;
        }

        // Project and correct file and get status
        if (n > limit)
        {
            // Break if there is too much data.
            emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Error: There was too much data!"));
            kill_flag = true;
        }
        else
        {
            emit changedImageWidth(files->at(i).getWidth());
            emit changedImageHeight(files->at(i).getHeight());

            (*files)[i].setProjectionKernel(&projection_kernel);
            (*files)[i].setBackground(&test_background, files->front().getFlux(), files->front().getExpTime());

            int STATUS_OK = (*files)[i].filterData( &n, reduced_pixels->data(), *threshold_reduce_low, *threshold_reduce_high, *threshold_project_low, *threshold_project_high,1);

            if (STATUS_OK)
            {
                emit repaintImageWidget();
                //--QCoreApplication::processEvents();
            }
            else
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Error: could not process data \""+files->at(i).getPath()+"\"");
                kill_flag = true;
            }
            std::cout << n << std::endl;
        }
        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/files->size());
    }
    size_t t = stopwatch.restart();

    reduced_pixels->resize(n);

    emit enableSetFileButton(true);
    emit enableReadFileButton(true);
    emit enableAllInOneButton(true);
    emit enableProjectFileButton(true);
    emit showGenericProgressBar(false);

    if (!kill_flag)
    {
        emit enableVoxelizeButton(true);

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
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
}

AllInOneWorker::~AllInOneWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
}

void AllInOneWorker::process()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    qDebug("Hello World!");
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
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
}

VoxelizeWorker::~VoxelizeWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
}

void VoxelizeWorker::process()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    emit finished();
}
