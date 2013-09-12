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

void BaseWorker::setSVOFile(SparseVoxelOcttree * svo)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->svo = svo;
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

void BaseWorker::setQSpaceInfo(float * suggested_search_radius_low, float * suggested_search_radius_high, float * suggested_q)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->suggested_search_radius_low = suggested_search_radius_low;
    this->suggested_search_radius_high = suggested_search_radius_high;
    this->suggested_q = suggested_q;
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
            if ((*suggested_search_radius_low) > files->back().getSearchRadiusLowSuggestion()) (*suggested_search_radius_low) = files->back().getSearchRadiusLowSuggestion();
            if ((*suggested_search_radius_high) < files->back().getSearchRadiusHighSuggestion()) (*suggested_search_radius_high) = files->back().getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if ((*suggested_q) < files->back().getQSuggestion()) (*suggested_q) = files->back().getQSuggestion();
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
        float resolution_min = 2*(*suggested_q)/(*suggested_search_radius_high);
        float resolution_max = 2*(*suggested_q)/(*suggested_search_radius_low);

        float level_min = std::log(resolution_min/(float)svo->getBrickInnerDimension())/std::log(2);
        float level_max = std::log(resolution_max/(float)svo->getBrickInnerDimension())/std::log(2);

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

            emit aquireSharedBuffers();
            int STATUS_OK = (*files)[i].filterData( &n, reduced_pixels->data(), *threshold_reduce_low, *threshold_reduce_high, *threshold_project_low, *threshold_project_high,1);
            emit releaseSharedBuffers();

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
        }
        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/files->size());
    }
    size_t t = stopwatch.restart();

    reduced_pixels->resize(n);

    /* Create dummy dataset for debugging purposes.
     *
    */
    if (0) // A sphere
    {
        int theta_max = 180; // Up to 180
        int phi_max = 360; // Up to 360

        reduced_pixels->resize(theta_max*phi_max*4);

        float radius = 0.45;
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
    else if (1) // A gradiented box
    {
        int res = 60;
        reduced_pixels->resize(res*res*res*4);

        for (int i = 0; i < res; i++)
        {
            for (int j = 0; j < res; j++)
            {
                for (int k = 0; k < res; k++)
                {
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+0] = (i - res*0.5)/res;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+1] = (j - res*0.5)/res;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+2] = (k - res*0.5)/res;
                    (*reduced_pixels)[(i+j*res+k*res*res)*4+3] = (float)std::sqrt(i*i+j*j+k*k);
                }
            }
        }
    }


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
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    // Crate the program
    QByteArray qsrc = open_resource(":/src/kernels/voxelize.cl");
    const char * src = qsrc.data();
    size_t src_length = strlen(src);

    program = clCreateProgramWithSource((*context), 1, (const char **)&src, &src_length, &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
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
    voxelize_kernel = clCreateKernel(program, "voxelize", &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
        return;
    }
}

void VoxelizeWorker::process()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    kill_flag = false;

    if (reduced_pixels->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No data available!");
        emit writeLog(str);
        emit changedMessageString(str);
        kill_flag = true;
    }
    if (!kill_flag)
    {
        // Emit to appropriate slots
        emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Creating Interpolation Data Structure: %p%"));
        emit enableSetFileButton(false);
        emit enableReadFileButton(false);
        emit enableProjectFileButton(false);
        emit enableVoxelizeButton(false);
        emit enableAllInOneButton(false);
        emit showGenericProgressBar(true);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Generating Sparse Voxel Octtree "+QString::number(svo->getLevels())+" levels deep.");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] The source data is "+QString::number(reduced_pixels->bytes()/1000000.0, 'g', 3)+" MB");

        // The number of data points in a single brick
        size_t n_points_brick = std::pow(svo->getBrickOuterDimension(), 3);
        size_t n_max_bricks = 2e9/n_points_brick*sizeof(float); // Allow up 2 GB of bricks

        // The extent of the volume
        svo->setExtent(*suggested_q);

        // Generate an octtree data structure from which to construct bricks
        SearchNode root(NULL, svo->getExtent()->data());

        for (size_t i = 0; i < reduced_pixels->size()/4; i++)
        {
            if (kill_flag)
            {
                QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(reduced_pixels->size()/4)+"!");
                emit writeLog(str);
                emit changedMessageString(str);
                break;
            }
            root.insert(reduced_pixels->data()+i*4);
            emit changedGenericProgress((i+1)*100/(reduced_pixels->size()/4));
        }

        if (!kill_flag)
        {
            /* Create an octtree from brick data. The nodes are maintained in a linear array rather than on the heap. This is due to lack of proper support for recursion on GPUs */
            MiniArray<BrickNode> gpuHelpOcttree(n_max_bricks*2);
            MiniArray<unsigned int> nodes;
            nodes.set(64, (unsigned int) 0);
            nodes[0] = 1;
            nodes[1] = 8;

            unsigned int confirmed_nodes = 1, non_empty_node_counter = 1;


            // Intitialize the first level
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level 0 (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  0))+")");

                gpuHelpOcttree[0].setMsdFlag(0);
                gpuHelpOcttree[0].setDataFlag(1);
                gpuHelpOcttree[0].setChild(1);
                gpuHelpOcttree[0].setPoolId(0,0,0);
                gpuHelpOcttree[0].setBrickId(0,0,0);
                gpuHelpOcttree[0].setLevel(0);
                gpuHelpOcttree[1].setParent(0);
                gpuHelpOcttree[2].setParent(0);
                gpuHelpOcttree[3].setParent(0);
                gpuHelpOcttree[4].setParent(0);
                gpuHelpOcttree[5].setParent(0);
                gpuHelpOcttree[6].setParent(0);
                gpuHelpOcttree[7].setParent(0);

                float * brick_data = new float[n_points_brick];
                float search_radius = sqrt(3.0f)*0.5f*((svo->getExtent()->at(1) - svo->getExtent()->at(0))/ (svo->getBrickInnerDimension()*(1 << 0)));

                if (search_radius < (*suggested_search_radius_high)) search_radius = (*suggested_search_radius_high);
                root.getBrick(brick_data, svo->getExtent()->data(), 1.0, search_radius, svo->getBrickOuterDimension());

                gpuHelpOcttree[0].setBrick(brick_data);

                emit changedMessageString(" ...done");
            }



            // Cycle through the remaining levels
            QElapsedTimer timer;
            for (size_t lvl = 1; lvl < svo->getLevels(); lvl++)
            {




                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                // Find the correct range search radius
                float search_radius = sqrt(3.0f)*0.5f*((svo->getExtent()->at(1)-svo->getExtent()->at(0))/ (svo->getBrickInnerDimension()*(1 << lvl)));
                if (search_radius < (*suggested_search_radius_high)) search_radius = (*suggested_search_radius_high);

                double tmp = (svo->getExtent()->at(1)-svo->getExtent()->at(0))/(1 << lvl);

                // For each node
                size_t iter = 0;

                QElapsedTimer timer_total, timer_spec;
                timer_total.start();
                timer_spec.start();
                size_t t0 = 0, t1 = 0, t2 = 0, t3 = 0, t_total;

                for (size_t i = 0; i < nodes[lvl]; i++)
                {
                    if (kill_flag) break;

                    // The id of the octnode in the octnode array
                    unsigned int currentId = confirmed_nodes + i;

                    // Set the level
                    gpuHelpOcttree[currentId].setLevel(lvl);

                    // Set the brick id
                    gpuHelpOcttree[currentId].calcBrickId(i%8 ,&gpuHelpOcttree[gpuHelpOcttree[currentId].getParent()]);

                    // Based on brick id calculate the brick extent and then calculate and set the brick data
                    unsigned int * brickId = gpuHelpOcttree[currentId].getBrickId();

                    Matrix<double> brick_extent(3,2);
                    brick_extent[0] = svo->getExtent()->at(0) + tmp*brickId[0];
                    brick_extent[1] = svo->getExtent()->at(0) + tmp*(brickId[0]+1);
                    brick_extent[2] = svo->getExtent()->at(2) + tmp*brickId[1];
                    brick_extent[3] = svo->getExtent()->at(2) + tmp*(brickId[1]+1);
                    brick_extent[4] = svo->getExtent()->at(4) + tmp*brickId[2];
                    brick_extent[5] = svo->getExtent()->at(4) + tmp*(brickId[2]+1);

                    float * brick_data = new float[n_points_brick];

                    t0 += timer_spec.nsecsElapsed();
                    timer_spec.restart();
                    bool isEmpty = root.getBrick(brick_data, brick_extent.data(), 1.0, search_radius, svo->getBrickOuterDimension());
                    t1 += timer_spec.nsecsElapsed();
                    timer_spec.restart();
                    if (isEmpty)
                    {
                        gpuHelpOcttree[currentId].setDataFlag(0);
                        gpuHelpOcttree[currentId].setMsdFlag(1);
                        gpuHelpOcttree[currentId].setChild(0);
                        delete[] brick_data;
                    }
                    else
                    {
                        gpuHelpOcttree[currentId].setDataFlag(1);
                        gpuHelpOcttree[currentId].setMsdFlag(0);
                        if (lvl >= svo->getLevels() - 1) gpuHelpOcttree[currentId].setMsdFlag(1);
                        gpuHelpOcttree[currentId].setBrick(brick_data);
                        gpuHelpOcttree[currentId].calcPoolId(svo->getBrickPoolPower(), non_empty_node_counter);

                        if (!gpuHelpOcttree[currentId].getMsdFlag())
                        {
                            unsigned int childId = confirmed_nodes + nodes[lvl] + iter*8;
                            gpuHelpOcttree[currentId].setChild(childId);

                            // For each child
                            for (size_t j = 0; j < 8; j++)
                            {
                                gpuHelpOcttree[childId+j].setParent(currentId);
                                nodes[lvl+1]++;
                            }
                        }
                        non_empty_node_counter++;
                        iter++;
                    }
                    emit changedGenericProgress((i+1)*100/nodes[lvl]);
                    t2 += timer_spec.nsecsElapsed();
                    timer_spec.restart();
                }
                if (kill_flag)
                {
                    QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(lvl)+" of "+QString::number(svo->getLevels())+"!");
                    emit writeLog(str);
                    emit changedMessageString(str);
                    break;
                }
                confirmed_nodes += nodes[lvl];

                size_t t = timer.restart();
                emit changedMessageString(" ...done (time: "+QString::number(t)+" ms)");

                t_total = timer_total.nsecsElapsed();
                std::cout << "L " << lvl << " t0: "<< t0 << "ns " << t0*100/t_total << "% t1: "<< t1 << "ns "  << t1*100/t_total << "% t2: "<< t0 << "ns "  << t2*100/t_total << std::endl;
            }

            if (!kill_flag)
            {
                // Use the node structure to populate the GPU arrays
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Transforming: %p%"));
                svo->index.reserve(confirmed_nodes);
                svo->brick.reserve(confirmed_nodes);
                svo->pool.reserve(non_empty_node_counter*n_points_brick);

                size_t iter = 0;
                for (size_t i = 0; i < confirmed_nodes; i++)
                {
                    if (kill_flag)
                    {
                        QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(confirmed_nodes)+"!");
                        emit writeLog(str);
                        emit changedMessageString(str);
                        break;
                    }

                    svo->index[i] = getOctIndex(gpuHelpOcttree[i].getMsdFlag(), gpuHelpOcttree[i].getDataFlag(), gpuHelpOcttree[i].getChild());
                    svo->brick[i] = getOctBrick(gpuHelpOcttree[i].getPoolId()[0], gpuHelpOcttree[i].getPoolId()[1], gpuHelpOcttree[i].getPoolId()[2]);

                    if (gpuHelpOcttree[i].getDataFlag())
                    {
                        for (size_t j = 0; j < n_points_brick; j++)
                        {
                            svo->pool[n_points_brick*iter + j] = gpuHelpOcttree[i].getBrick()[j];
                        }
                        iter++;
                    }
                    emit changedGenericProgress((i+1)*100/confirmed_nodes);
                }
            }

            std::cout << "reduced_pixels.max() " << reduced_pixels->max() << std::endl;
            std::cout << "reduced_pixels.min() " << reduced_pixels->min() << std::endl;

            std::cout << "pool.max() " << svo->pool.max() << std::endl;
            std::cout << "pool.min() " << svo->pool.min() << std::endl;

            if (!kill_flag)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Total size (uncompressed): "+QString::number((svo->getBytes())/1e6, 'g', 3)+" MB");
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Number of bricks: "+QString::number(confirmed_nodes));
            }
        }
    }

    emit enableSetFileButton(true);
    emit enableReadFileButton(true);
    emit enableAllInOneButton(true);
    emit enableProjectFileButton(true);
    emit enableVoxelizeButton(true);
    emit enableAllInOneButton(true);
    emit showGenericProgressBar(false);

    emit finished();
}

void VoxelizeWorker::processOpenCL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    kill_flag = false;

    if (reduced_pixels->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No data available!");
        emit writeLog(str);
        emit changedMessageString(str);
        kill_flag = true;
    }
    if (!kill_flag)
    {
        // Emit to appropriate slots
        emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Creating Interpolation Data Structure: %p%"));
        emit enableSetFileButton(false);
        emit enableReadFileButton(false);
        emit enableProjectFileButton(false);
        emit enableVoxelizeButton(false);
        emit enableAllInOneButton(false);
        emit showGenericProgressBar(true);

        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Generating Sparse Voxel Octtree "+QString::number(svo->getLevels())+" levels deep.");
        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] The source data is "+QString::number(reduced_pixels->bytes()/1000000.0, 'g', 3)+" MB");

        // The number of data points in a single brick
        size_t n_points_brick = std::pow(svo->getBrickOuterDimension(), 3);
        size_t n_max_bricks = 2e9/n_points_brick*sizeof(float); // Allow up 2 GB of bricks

        // The extent of the volume
        svo->setExtent(*suggested_q);

        // Generate a "search box" data structure from which to construct bricks

        //##################################//
        //##################################//
        //##################################//

        // "rsds" is shorthand for "range search data structure"
        n_points_brick = std::pow(svo->getBrickOuterDimension(), 3);
        MiniArray<int> level_dimesnion(svo->getLevels());
        size_t rsds_dimension = 256;
        size_t rsds_items_whole = rsds_dimension*rsds_dimension*rsds_dimension;
        size_t rsds_items_slice = rsds_dimension*rsds_dimension;
        size_t rsds_items_row = rsds_dimension;
        MiniArray<int> rsds_tag;
        rsds_tag.set(rsds_items_whole,-1);
        size_t ws_global[3];
        size_t ws_local[3];

        svo->setExtent(*suggested_q);
        MiniArray<float> volume_extent;
        volume_extent.setDeep(6, svo->getExtent()->toFloat().data());

        for (size_t i = 0; i < svo->getLevels(); i++)
        {
            level_dimesnion[i] = svo->getBrickInnerDimension() * (1 << i);
        }

        // Some declarations
        int rsds_pixel_id;
        MiniArray<int> rsds_pixel_id_3d(3);
        bool OUT_OF_RANGE = false;

        // First we generate a range search data structure (RSS) by splitting up the volume into equally sized cubic items. Then we arrange the pixel data in accordance to the item they are contained in
        MiniArray<int> rsds_item_size(rsds_items_whole, 0);
        MiniArray<int> rsds_item_offset(rsds_items_whole);
        MiniArray<int> rsds_counter(rsds_items_whole, 0);

        // Here we determine the number of points to reserve space for in each item. Some items will get many points, others few.
        for (size_t i = 0; i < reduced_pixels->size()/4; i++)
        {
            // Find 3D RSS box index of current point
            OUT_OF_RANGE = false;
            for (int j = 0; j < 3; j++)
            {
                rsds_pixel_id_3d[j] = (rsds_dimension*(reduced_pixels->at(i*4+j) - volume_extent[j*2])/(volume_extent[j*2+1]-volume_extent[j*2]));
                if ((rsds_pixel_id_3d[j] >=  (int) rsds_dimension) || (rsds_pixel_id_3d[j] < 0))
                {
                    OUT_OF_RANGE = true;
                }
            }
            if (OUT_OF_RANGE) continue;

            // Find index for point
            rsds_pixel_id = rsds_pixel_id_3d[0] + rsds_pixel_id_3d[1]*rsds_items_row + rsds_pixel_id_3d[2]*rsds_items_slice;
            // Increment the sice of the corresponding item
            rsds_item_size[rsds_pixel_id]++;

            if (kill_flag)
            {
                QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(reduced_pixels->size()/4)+"!");
                emit writeLog(str);
                emit changedMessageString(str);
                break;
            }

            emit changedGenericProgress((i+1)*100/(reduced_pixels->size()/4));
        }

        if(!kill_flag)
        {
            // Here we calculate the offset for each item so that we know where to find the different boxes later.
            // One could omit empty boxes completly to save space, but that would mean an extra array to keep track of offsets
            rsds_item_offset[0] = 0;
            for (size_t i = 1; i < rsds_items_whole; i++)
            {
                rsds_item_offset[i] = rsds_item_offset[i-1] + rsds_item_size[i-1];
            }
            MiniArray<float> rsds_reduced_pixels((rsds_item_offset[rsds_items_whole-1] + rsds_item_size[rsds_items_whole-1])*4, 0.0);

            // Here we populate the data array for item
            for (size_t i = 0; i < reduced_pixels->size()/4; i++)
            {

                OUT_OF_RANGE = false;
                for (size_t j = 0; j < 3; j++)
                {
                    rsds_pixel_id_3d[j] = (rsds_dimension*(reduced_pixels->at(i*4+j) - volume_extent[j*2])/(volume_extent[j*2+1]-volume_extent[j*2]));
                    if ((rsds_pixel_id_3d[j] >=  (int) rsds_dimension) || (rsds_pixel_id_3d[j] < 0))
                    {
                        OUT_OF_RANGE = true;
                    }
                }
                if (OUT_OF_RANGE) continue;

                // Find index for point
                rsds_pixel_id = rsds_pixel_id_3d[0] + rsds_pixel_id_3d[1]*rsds_items_row + rsds_pixel_id_3d[2]*rsds_items_slice;
                rsds_reduced_pixels[rsds_item_offset[rsds_pixel_id]*4 + rsds_counter[rsds_pixel_id]*4 + 0] = reduced_pixels->at(i*4 + 0);
                rsds_reduced_pixels[rsds_item_offset[rsds_pixel_id]*4 + rsds_counter[rsds_pixel_id]*4 + 1] = reduced_pixels->at(i*4 + 1);
                rsds_reduced_pixels[rsds_item_offset[rsds_pixel_id]*4 + rsds_counter[rsds_pixel_id]*4 + 2] = reduced_pixels->at(i*4 + 2);
                rsds_reduced_pixels[rsds_item_offset[rsds_pixel_id]*4 + rsds_counter[rsds_pixel_id]*4 + 3] = reduced_pixels->at(i*4 + 3);
                rsds_counter[rsds_pixel_id]++;
                emit changedGenericProgress((i+1)*100/(reduced_pixels->size()/4));
            }

            // We now have an array containing data for all items in the rsds (range search data structure)
            // In addition we have two arrays detailing the offset and length of each sub box in the array

            /* Generate bricks for each octree level using interpolation on the GPU. All levels employ interpolation from the raw data. The GPU can treat multiple bricks simultaneously. All rsds items that intersect with a batch of bricks will be tagged/marked for upload to the GPU */

            size_t pool_brick_whole = (1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower());
            svo->pool.reserve((size_t)(n_points_brick*pool_brick_whole));
            MiniArray<unsigned int> brick_index(pool_brick_whole*2);
            MiniArray<unsigned int> brick_index_offset(svo->getLevels(), -1);
            MiniArray<unsigned int> brick_index_size(svo->getLevels(), -1);
            bool OUT_OF_BRICK_MEMORY = false;
            size_t glb_brick_counter = 0;
            size_t BRICKS_OFFSET = 0;
            size_t MAX_BRICKS_KERNEL_CALL = 32768;
            cl_mem  cl_rsds_reduced_pixels, cl_brick_index, cl_brick_index_offset, cl_brick_extent, cl_brick_index_size, cl_bricks, cl_rsds_item_offset, cl_rsds_item_size;

            // A Buffer to hold rsds pixel data
            cl_rsds_reduced_pixels = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, device->gpu_max_mem_alloc_size , NULL, &err); // THIS ONE A BIT BIG?
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The offset of each tagged rsds item
            cl_rsds_item_offset = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, rsds_items_whole*2*sizeof(cl_int) , NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The length of each tagged item
            cl_rsds_item_size = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, rsds_items_whole*2*sizeof(cl_int) , NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The item indicies reached by each brick
            cl_brick_index = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, rsds_items_whole*2*sizeof(cl_int) , NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The offset for each brick in the previous array
            cl_brick_index_offset = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, MAX_BRICKS_KERNEL_CALL*sizeof(cl_int) , NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The number of item indices associated with each brick
            cl_brick_index_size = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, MAX_BRICKS_KERNEL_CALL*sizeof(cl_int) , NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The 3D spatial extent of each brick in Cartesian coords
            cl_brick_extent = clCreateBuffer(
                (*context), CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, MAX_BRICKS_KERNEL_CALL*sizeof(cl_float)*6 , NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }
            // The output buffer for the finished (and presumably nonzero) bricks
            cl_bricks = clCreateBuffer(
                (*context), CL_MEM_WRITE_ONLY, MAX_BRICKS_KERNEL_CALL*sizeof(cl_float)*n_points_brick, NULL, &err);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }

            err = clSetKernelArg(voxelize_kernel, 0, sizeof(cl_mem), &cl_rsds_reduced_pixels);
            err |= clSetKernelArg(voxelize_kernel, 1, sizeof(cl_mem), &cl_rsds_item_offset);
            err |= clSetKernelArg(voxelize_kernel, 2, sizeof(cl_mem), &cl_rsds_item_size);
            err |= clSetKernelArg(voxelize_kernel, 3, sizeof(cl_mem), &cl_brick_index);
            err |= clSetKernelArg(voxelize_kernel, 4, sizeof(cl_mem), &cl_brick_index_offset);
            err |= clSetKernelArg(voxelize_kernel, 5, sizeof(cl_mem), &cl_brick_index_size);
            err |= clSetKernelArg(voxelize_kernel, 6, sizeof(cl_mem), &cl_brick_extent);
            err |= clSetKernelArg(voxelize_kernel, 7, sizeof(cl_mem), &cl_bricks);
            int tmpp = svo->getBrickOuterDimension();
            err |= clSetKernelArg(voxelize_kernel, 8, sizeof(cl_int), &tmpp);
            if (err != CL_SUCCESS)
            {
                writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            }

            QElapsedTimer timer;

            // Now, for each level in the octree structure:
            for (size_t lvl = 0; lvl < svo->getLevels(); lvl++)
            {
                /* Beacuse of limitations in the box model we leave the bricks in the first three levels filled and opaque. The dimension at the third level (starting from the zeroth) is 64  (8 bricks). We do this beacuse the GPU can only handle a limitied amount of data, and we cant have a kernel call that potentially covers too much data. At level 3 there are 512 bricks to share the data. This also means the traversal in the first three levels is useless since we have effectively set the minimum dimension. */

                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                brick_index_offset[lvl] = glb_brick_counter;

                // Find the correct range search radius
                float search_radius = sqrt(3.0f)*0.5f*((volume_extent[1]-volume_extent[0])/ level_dimesnion[lvl]);
                if (search_radius < (*suggested_search_radius_high)) search_radius = (*suggested_search_radius_high);

                /* For each brick, find the relevant sub data in the range search box */
                size_t bricks_row = 1 << lvl;
                size_t grp_brick_counter = 0;
                size_t GPU_MEM_LIM = device->gpu_max_mem_alloc_size/2;
                size_t GPU_MEM_USED = 0;
                size_t cl_rsds_reduced_pixels_bytes = 0;
                size_t sub_id;
                size_t tag_counter = 0;
                size_t lvl_brick_counter = 0;
                bool IS_EMPTY_BRICK = true;

                int rs_indices[6];
                int grp_index_counter = 0;
                int grp_subbox_counter = 0;

                Matrix<float> brick_extent;
                brick_extent.reserve(std::min(MAX_BRICKS_KERNEL_CALL, bricks_row*bricks_row*bricks_row), 6);
                MiniArray<int> grp_subbox_offsets(rsds_items_whole*2, 0);
                MiniArray<int> grp_subbox_lengths(rsds_items_whole*2, 0);
                MiniArray<int> grp_brick_indices(rsds_items_whole*2, 0);
                MiniArray<int> grp_brick_indices_offsets(MAX_BRICKS_KERNEL_CALL, 0);
                MiniArray<int> grp_brick_indices_lengths(MAX_BRICKS_KERNEL_CALL, 0);

                // For each brick at this level:
                for (size_t id_z = 0; id_z < bricks_row; id_z++)
                {
                    for (size_t id_y = 0; id_y < bricks_row; id_y++)
                    {
                        for (size_t id_x = 0; id_x < bricks_row; id_x++)
                        {
                            if (!OUT_OF_BRICK_MEMORY)
                            {
                                // Find spatial extent of the brick
                                brick_extent[grp_brick_counter*6 + 0] = (float) id_x * (volume_extent[1] - volume_extent[0])/((float)bricks_row) + volume_extent[0];
                                brick_extent[grp_brick_counter*6 + 1] = ((float)id_x + 1.0) * (volume_extent[1] - volume_extent[0])/((float)bricks_row) + volume_extent[0];
                                brick_extent[grp_brick_counter*6 + 2] = (float) id_y * (volume_extent[3] - volume_extent[2])/((float)bricks_row) + volume_extent[2];
                                brick_extent[grp_brick_counter*6 + 3] = ((float) id_y + 1.0) * (volume_extent[3] - volume_extent[2])/((float)bricks_row) + volume_extent[2];
                                brick_extent[grp_brick_counter*6 + 4] = (float) id_z * (volume_extent[5] - volume_extent[4])/((float)bricks_row) + volume_extent[4];
                                brick_extent[grp_brick_counter*6 + 5] = ((float) id_z + 1.0) * (volume_extent[5] - volume_extent[4])/((float)bricks_row) + volume_extent[4];

                                /* Find data required by brick. Do this by finding the index span over the RS sub boxes. Next tag relevant boxes. Update the value of required GPU memory. Once enough memory is used, send data to GPU and process. Also keep track of the sub boxes used by each brick */
                                for (int i = 0; i < 3; i++)
                                {
                                    rs_indices[i*2] = floor(((brick_extent[grp_brick_counter*6 + i*2]-search_radius)-volume_extent[i*2])/(volume_extent[i*2+1] - volume_extent[i*2])*rsds_dimension);
                                    if (rs_indices[i*2] < 0) rs_indices[i*2] = 0;

                                    rs_indices[i*2+1] = ceil(((brick_extent[grp_brick_counter*6 + i*2+1]+search_radius)-volume_extent[i*2])/(volume_extent[i*2+1] - volume_extent[i*2])*rsds_dimension);
                                    if (rs_indices[i*2+1] >= (int) rsds_dimension) rs_indices[i*2+1] = rsds_dimension - 1;
                                }

                                // Record the offset connected to this brick
                                grp_brick_indices_offsets[grp_brick_counter] = grp_index_counter;
                                IS_EMPTY_BRICK = true;

                                // Tag sub boxes needed by the brick
                                for (int i = rs_indices[4]; i <= rs_indices[5]; i++)
                                {
                                    for (int j = rs_indices[2]; j <= rs_indices[3]; j++)
                                    {
                                        for (int k = rs_indices[0]; k <= rs_indices[1]; k++)
                                        {
                                            sub_id = k + j*rsds_items_row + i*rsds_items_slice;
                                            if (rsds_item_size[sub_id] > 0)
                                            {
                                                if (rsds_tag[sub_id] == -1)
                                                {
                                                    GPU_MEM_USED += rsds_item_size[sub_id]*sizeof(cl_float4);
                                                    rsds_tag[sub_id] = tag_counter;
                                                    tag_counter++;
                                                    // Upload data for unique tags
                                                    err = clEnqueueWriteBuffer((*queue), cl_rsds_reduced_pixels, CL_TRUE, cl_rsds_reduced_pixels_bytes,
                                                        rsds_item_size[sub_id]*sizeof(cl_float4),
                                                        rsds_reduced_pixels.data()+rsds_item_offset[sub_id]*4,
                                                        0, NULL, NULL);
                                                    if (err != CL_SUCCESS)
                                                    {
                                                        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                                                    }
                                                    grp_subbox_lengths[grp_subbox_counter] = rsds_item_size[sub_id];
                                                    grp_subbox_offsets[grp_subbox_counter] = cl_rsds_reduced_pixels_bytes/16;
                                                    cl_rsds_reduced_pixels_bytes += rsds_item_size[sub_id]*sizeof(cl_float4);
                                                    grp_subbox_counter++;
                                                }
                                                IS_EMPTY_BRICK = false;
                                                grp_brick_indices[grp_index_counter] = rsds_tag[sub_id];
                                                grp_brick_indices_lengths[grp_brick_counter]++;
                                                grp_index_counter++;
                                            }
                                        }
                                    }
                                }

                                // The index of the brick
                                size_t id = id_x + id_y*bricks_row + id_z*bricks_row*bricks_row;

                                if (!IS_EMPTY_BRICK)
                                {
                                    brick_index[glb_brick_counter] = id;
                                    brick_index_size[lvl]++;
                                    glb_brick_counter++;
                                    grp_brick_counter++;
                                    lvl_brick_counter++;
                                }

                                if ((GPU_MEM_USED >= GPU_MEM_LIM) || (id == bricks_row*bricks_row*bricks_row-1) || (grp_brick_counter >= MAX_BRICKS_KERNEL_CALL))
                                {
                                    // Launch kernel if any of above reqs. are met
                                    err = clEnqueueWriteBuffer((*queue), cl_rsds_item_offset, CL_TRUE, 0,
                                        grp_subbox_counter*sizeof(cl_int),
                                        grp_subbox_offsets.data(),
                                        0, NULL, NULL);
                                    err |= clEnqueueWriteBuffer((*queue), cl_rsds_item_size, CL_TRUE, 0,
                                        grp_subbox_counter*sizeof(cl_int),
                                        grp_subbox_lengths.data(),
                                        0, NULL, NULL);
                                    err |= clEnqueueWriteBuffer((*queue), cl_brick_index, CL_TRUE, 0,
                                        grp_index_counter*sizeof(cl_int),
                                        grp_brick_indices.data(),
                                        0, NULL, NULL);
                                    err |= clEnqueueWriteBuffer((*queue), cl_brick_index_offset, CL_TRUE, 0,
                                        grp_brick_counter*sizeof(cl_int),
                                        grp_brick_indices_offsets.data(),
                                        0, NULL, NULL);
                                    err |= clEnqueueWriteBuffer((*queue), cl_brick_index_size, CL_TRUE, 0,
                                        grp_brick_counter*sizeof(cl_int),
                                        grp_brick_indices_lengths.data(),
                                        0, NULL, NULL);

                                    err |= clEnqueueWriteBuffer((*queue), cl_brick_extent, CL_TRUE, 0,
                                        grp_brick_counter*sizeof(cl_float)*6,
                                        brick_extent.data(),
                                        0, NULL, NULL);
                                    if (err != CL_SUCCESS)
                                    {
                                        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                                    }

                                    err = clSetKernelArg(voxelize_kernel, 9, sizeof(cl_float), &search_radius);
                                    if (err != CL_SUCCESS)
                                    {
                                        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                                    }

                                    ws_global[0] = svo->getBrickOuterDimension();
                                    ws_global[1] = svo->getBrickOuterDimension();
                                    ws_global[2] = (svo->getBrickOuterDimension())*grp_brick_counter;

                                    ws_local[0] = svo->getBrickOuterDimension();
                                    ws_local[1] = svo->getBrickOuterDimension();
                                    ws_local[2] = svo->getBrickOuterDimension();

                                    err = clEnqueueNDRangeKernel((*queue), voxelize_kernel,
                                    3, NULL, ws_global, ws_local,
                                    0, NULL, NULL);
                                    if (err != CL_SUCCESS)
                                    {
                                        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                                    }

                                    // Read processed brick data
                                    if (glb_brick_counter > pool_brick_whole)
                                    {
                                        std::cout << "Brick memory exceeded!" << std::endl;
                                        OUT_OF_BRICK_MEMORY = true;
                                        break;
                                    }


                                    std::cout << "Reading results: " << grp_brick_counter << " bricks, offset: "<< BRICKS_OFFSET << std::endl;
                                    err = clEnqueueReadBuffer(
                                        *queue,
                                        cl_bricks,
                                        CL_TRUE,
                                        0,
                                        sizeof(cl_float)*n_points_brick*grp_brick_counter,
                                        svo->pool.data() + BRICKS_OFFSET,
                                         0, NULL, NULL );
                                    if (err != CL_SUCCESS)
                                    {
                                        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                                    }

                                    BRICKS_OFFSET = glb_brick_counter*n_points_brick;
                                    rsds_tag.set(rsds_items_whole,-1);
                                    GPU_MEM_USED = 0;
                                    grp_index_counter = 0;
                                    grp_brick_counter = 0;
                                    cl_rsds_reduced_pixels_bytes = 0;
                                    grp_subbox_counter = 0;
                                    tag_counter = 0;
                                }
                            }
                            if (kill_flag)
                            {
                                QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(id_x + id_y*bricks_row + id_z*bricks_row*bricks_row)+" of "+QString::number(bricks_row*bricks_row*bricks_row)+"!");
                                emit writeLog(str);
                                emit changedMessageString(str);
                                break;
                            }
                            emit changedGenericProgress((id_x + id_y*bricks_row + id_z*bricks_row*bricks_row)*100.0/(bricks_row*bricks_row*bricks_row));
                        }
                        if (kill_flag) break;
                    }
                    if (kill_flag) break;
                }
                if (kill_flag) break;
                size_t t = timer.restart();

                emit changedMessageString(" ...done (time: "+QString::number(t)+" ms)");

            }

            //~size_t N_BRICKS = glb_brick_counter;
            svo->pool.resize(glb_brick_counter*n_points_brick*1);

            if(cl_rsds_item_offset) clReleaseMemObject(cl_rsds_item_offset);
            if(cl_rsds_item_size) clReleaseMemObject(cl_rsds_item_size);
            if(cl_rsds_reduced_pixels) clReleaseMemObject(cl_rsds_reduced_pixels);
            if(cl_brick_index) clReleaseMemObject(cl_brick_index);
            if(cl_brick_index_offset) clReleaseMemObject(cl_brick_index_offset);
            if(cl_brick_index_size) clReleaseMemObject(cl_brick_index_size);
            if(cl_brick_extent) clReleaseMemObject(cl_brick_extent);
            if(cl_bricks) clReleaseMemObject(cl_bricks);


            //##################################//
            /* Improvement suggestions:
             * have all pixel data in single buffer/texture to limit cpu<->gpu talk
             * Somehow utilize local memory
             * Skip emty items in the rs_boxes?
             * Have 3D brick pool texture in memory and write directly to it
             * */
            //##################################//

            if (!kill_flag)
            {
                /* Create an octtree from brick data */
                MiniArray<BrickNode> gpu_help_octtree;
                gpu_help_octtree.reserve(n_max_bricks*2);

                MiniArray<unsigned int> nodes;
                nodes.set(64, (unsigned int) 0);
                nodes[0] = 1;
                nodes[1] = 8;

                unsigned int confirmed_nodes = 1, non_empty_node_counter = 1;


                // Intitialize the first level
                gpu_help_octtree[0].setMsdFlag(0);
                gpu_help_octtree[0].setDataFlag(1);
                gpu_help_octtree[0].setChild(1);
                gpu_help_octtree[0].setPoolId(0,0,0);
                gpu_help_octtree[0].setBrickId(0,0,0);
                gpu_help_octtree[0].setLevel(0);
                gpu_help_octtree[1].setParent(0);
                gpu_help_octtree[2].setParent(0);
                gpu_help_octtree[3].setParent(0);
                gpu_help_octtree[4].setParent(0);
                gpu_help_octtree[5].setParent(0);
                gpu_help_octtree[6].setParent(0);
                gpu_help_octtree[7].setParent(0);

                // Cycle through the remaining levels
                for (size_t lvl = 1; lvl < svo->getLevels(); lvl++)
                {
                    timer.start();

                    // For each node
                    for (size_t i = 0; i < nodes[lvl]; i++)
                    {
                        unsigned int currentId = confirmed_nodes + i;
                        gpu_help_octtree[currentId].setLevel(lvl); // Set the level
                        gpu_help_octtree[currentId].calcBrickId(i%8 ,&gpu_help_octtree[gpu_help_octtree[currentId].getParent()]); // Set the brick id
                    }


                    /* Sort based on the the recent brick level indices, check
                     * against actual brick indices, unsort, set empty/non-empty
                     * bricks */
                    Matrix<unsigned int> dataFlag;
                    Matrix<unsigned int> poolId1D;
                    {
                        Matrix<unsigned int> counter;
                        Matrix<unsigned int> brickId;

                        dataFlag.set(1, nodes[lvl], 0);
                        counter.reserve(1, nodes[lvl]);
                        brickId.reserve(1, nodes[lvl]);
                        poolId1D.set(1, nodes[lvl], 0);

                        for (size_t k = 0; k < nodes[lvl]; k++)
                        {
                            counter[k] = k;
                            brickId[k] = gpu_help_octtree[confirmed_nodes + k].getBrickId1D();
                        }

                        // Sort
                        this->quickSortAux2(brickId.data(), counter.data(), dataFlag.data(),  0, nodes[lvl]-1);

                        // Mark new nodes as empty or non empty
                        size_t count = 0;
                        for (size_t k = 0; k < nodes[lvl]; k++)
                        {
                            if ( brickId[k] == brick_index[brick_index_offset[lvl] + count])
                            {

                                dataFlag[k] = 1;
                                poolId1D[k] = brick_index_offset[lvl] + count;
                                count++;
                            }
                        }

                        // Sort back
                        this->quickSortAux3(counter.data(), brickId.data(), dataFlag.data(), poolId1D.data(),  0, nodes[lvl]-1);
                    }


                    // For each node
                    size_t iter = 0;
                    for (size_t i = 0; i < nodes[lvl]; i++)
                    {
                        // If not empty, mark as not empty, prepare children, and proceed
                        unsigned int currentId = confirmed_nodes + i;

                        if (lvl >= svo->getLevels() - 1) gpu_help_octtree[currentId].setMsdFlag(1);
                        else gpu_help_octtree[currentId].setMsdFlag(0);

                        if (dataFlag[i])
                        {
                            gpu_help_octtree[currentId].setDataFlag(1);
                            gpu_help_octtree[currentId].calcPoolId(svo->getBrickPoolPower(), poolId1D[i]);

                            if (!gpu_help_octtree[currentId].getMsdFlag())
                            {
                                unsigned int childId = confirmed_nodes + nodes[lvl] + iter*8;
                                gpu_help_octtree[currentId].setChild(childId);

                                // For each child
                                for (size_t j = 0; j < 8; j++)
                                {
                                    gpu_help_octtree[childId+j].setParent(currentId);
                                    nodes[lvl+1]++;
                                }
                            }
                            non_empty_node_counter++;
                            iter++;
                        }
                        // If empty, mark as empty and proceed
                        else
                        {
                            gpu_help_octtree[currentId].setDataFlag(0);
                            gpu_help_octtree[currentId].setChild(0);
                        }
                    }
                    confirmed_nodes += nodes[lvl];

                    size_t t = timer.restart();
                    std::stringstream ss;
                    ss << std::endl << "Octree L" << lvl << " constructed (time: " << t << " ms)";
                    QString str(ss.str().c_str());

                }

                // Use the node structure to populate the GPU arrays
                svo->index.reserve(confirmed_nodes);
                svo->brick.reserve(confirmed_nodes);
                for (size_t i = 0; i < confirmed_nodes; i++)
                {
                    svo->index[i] = getOctIndex(gpu_help_octtree[i].getMsdFlag(), gpu_help_octtree[i].getDataFlag(), gpu_help_octtree[i].getChild());
                    svo->brick[i] = getOctBrick(gpu_help_octtree[i].getPoolId()[0], gpu_help_octtree[i].getPoolId()[1], gpu_help_octtree[i].getPoolId()[2]);
                }


                //##################################//
                //##################################//
                //##################################//


                if (!kill_flag)
                {
                    /* Create an octtree from brick data. The nodes are maintained in a linear array rather than on the heap. This is due to lack of proper support for recursion on GPUs (2013) */


                    if (!kill_flag)
                    {
                        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Total size (uncompressed): "+QString::number((svo->getBytes())/1e6, 'g', 3)+" MB");
                        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Number of bricks: "+QString::number(confirmed_nodes));
                    }
                }
            }
        }
    }
    emit enableSetFileButton(true);
    emit enableReadFileButton(true);
    emit enableAllInOneButton(true);
    emit enableProjectFileButton(true);
    emit enableVoxelizeButton(true);
    emit enableAllInOneButton(true);
    emit showGenericProgressBar(false);

    emit finished();
}


void VoxelizeWorker::swap(unsigned int * arr, int i, int j)
{
    int temp = arr[j];
    arr[j] = arr[i];
    arr[i] = temp;
}

void VoxelizeWorker::quickSortAux2(unsigned int * arr, unsigned int * arr_aux, unsigned int * arr_aux2,  int left, int right)
{
    /* Quicksort of an array "arr" while retaining an auxilary array "arr_aux" accordingly */
    if (left >= right)
        return;

    int key = arr[left];
    int i = left + 1, j = right;
    while (i < j)
    {
        while (i < j && (int)arr[j] >= key)
            --j;
        while (i < j && (int)arr[i] <= key)
            ++i;
        if (i < j)
        {
            swap(arr, i, j);
            swap(arr_aux, i, j);
            swap(arr_aux2, i, j);
        }
    }
    if (arr[left] > arr[i])
    {
        swap(arr, left, i);
        swap(arr_aux, left, i);
        swap(arr_aux2, left, i);
        quickSortAux2(arr, arr_aux, arr_aux2, left, i - 1);
        quickSortAux2(arr, arr_aux, arr_aux2, i + 1, right);
    }
    else
    {
        quickSortAux2(arr, arr_aux, arr_aux2, left + 1, right);
    }
}

void VoxelizeWorker::quickSortAux3(unsigned int * arr, unsigned int * arr_aux, unsigned int * arr_aux2, unsigned int * arr_aux3, int left, int right)
{
    /* Quicksort of an array "arr" while retaining an auxilary array "arr_aux" accordingly */
    if (left >= right)
        return;

    int key = arr[left];
    int i = left + 1, j = right;
    while (i < j)
    {
        while (i < j && (int)arr[j] >= key)
            --j;
        while (i < j && (int)arr[i] <= key)
            ++i;
        if (i < j)
        {
            swap(arr, i, j);
            swap(arr_aux, i, j);
            swap(arr_aux2, i, j);
            swap(arr_aux3, i, j);
        }
    }
    if (arr[left] > arr[i])
    {
        swap(arr, left, i);
        swap(arr_aux, left, i);
        swap(arr_aux2, left, i);
        swap(arr_aux3, left, i);
        quickSortAux3(arr, arr_aux, arr_aux2, arr_aux3, left, i - 1);
        quickSortAux3(arr, arr_aux, arr_aux2, arr_aux3, i + 1, right);
    }
    else
    {
        quickSortAux3(arr, arr_aux, arr_aux2, arr_aux3, left + 1, right);
    }
}


/***
 *         dBBBBb  dBP.dBBBBP dBBBBBb  dBP dBBBBBb dBP dBP
 *            dB'     BP          dB'           BB    dBP
 *       dBP dB' dBP  `BBBBb  dBBBP' dBP    dBP BB   dBP
 *      dBP dB' dBP      dBP dBP    dBP    dBP  BB  dBP
 *     dBBBBB' dBP  dBBBBP' dBP    dBBBBP dBBBBBBB dBP
 *
 */

DisplayFileWorker::DisplayFileWorker()
{
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->isCLInitialized = false;
    this->verbosity = verbosity;
    test_background.set(1475, 1679, 0.0);
}

DisplayFileWorker::~DisplayFileWorker()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    if (isCLInitialized) clReleaseKernel(projection_kernel);
}

void DisplayFileWorker::setDisplayFile(int value)
{
    display_file = value;
}


void DisplayFileWorker::process()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    PilatusFile file;

    int STATUS_OK = file.set(file_paths->at(display_file), context, queue);
    if (STATUS_OK)
    {
        file.setOpenCLBuffers(alpha_img_clgl, beta_img_clgl, gamma_img_clgl, tsf_img_clgl);
        STATUS_OK = file.readData();
        if (STATUS_OK)
        {
            emit changedImageWidth(file.getWidth());
            emit changedImageHeight(file.getHeight());
            file.setProjectionKernel(&projection_kernel);
            file.setBackground(&test_background, file.getFlux(), file.getExpTime());

            size_t n;
            emit aquireSharedBuffers();
            STATUS_OK = file.filterData( &n, NULL, *threshold_reduce_low, *threshold_reduce_high, *threshold_project_low, *threshold_project_high, 0);
            emit releaseSharedBuffers();
            if (STATUS_OK)
            {
                emit repaintImageWidget();
            }
        }
    }
    emit finished();
}
