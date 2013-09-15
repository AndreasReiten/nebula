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
    emit changedTabWidget(1);


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
    if (isCLInitialized && project_kernel) clReleaseKernel(project_kernel);
}

void ProjectFileWorker::initializeCLKernel()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    // Program
    QByteArray qsrc = open_resource(":/src/kernels/project.cl");
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
    const char * options = "-cl-mad-enable -Werror";
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
            "\n--- START KERNEL COMPILE LOG ---\n"+
            QString(build_log)+
            "\n---  END KERNEL COMPILE LOG  ---");
        emit writeLog(str);
        delete[] build_log;
        return;
    }

    // Entry point
    project_kernel = clCreateKernel(program, "FRAME_FILTER", &err);
    if (err != CL_SUCCESS)
    {
        QString str("Error in "+QString(this->metaObject()->className())+": Could not create kernel object: "+QString(cl_error_cstring(err)));
        std::cout << str.toStdString().c_str() << std::endl;
        emit writeLog(str);
        return;
    }

    isCLInitialized = true;
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

            (*files)[i].setProjectionKernel(&project_kernel);
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
    if (1) // A sphere
    {
        int theta_max = 180; // Up to 180
        int phi_max = 360; // Up to 360

        reduced_pixels->resize(theta_max*phi_max*4);

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
        reduced_pixels->resize(res*res*res*4);

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

    if (isCLInitialized && voxelize_kernel) clReleaseKernel(voxelize_kernel);
    if (isCLInitialized && items_cl) clReleaseMemObject(items_cl);
    if (isCLInitialized && target_cl) clReleaseMemObject(target_cl);
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
    const char * options = "-cl-mad-enable -Werror";
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
            "\n--- START KERNEL COMPILE LOG ---\n"+
            QString(build_log)+
            "\n---  END KERNEL COMPILE LOG  ---");
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

    items_cl =  clCreateBuffer((*context),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        1024*2*2*2*2*2*2*2*16,
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
        return;
    }

    brick_extent_cl =  clCreateBuffer((*context),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        6*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
        return;
    }

    target_cl =  clCreateBuffer((*context),
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        513*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
        return;
    }

    isCLInitialized = true;
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

        // Prepare the brick pool
        size_t pool_max_size = 1e9;
        MiniArray<int> pool_dimension(4, 0);
        pool_dimension[0] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
        pool_dimension[1] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
        pool_dimension[2] = (pool_max_size/(sizeof(cl_float)*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) / ((1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower()));

        if (pool_dimension[2] < 2) pool_dimension[2] = 2;
        pool_dimension[2] *= svo->getBrickOuterDimension();

        pool_dimension.print(2,"pool_dimension");

        cl_mem pool_cl = clCreateBuffer((*context),
            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            pool_dimension[0]*pool_dimension[1]*pool_dimension[2]*sizeof(cl_float),
            NULL,
            &err);
        if (err != CL_SUCCESS)
        {
            writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
            return;
        }

        err = clSetKernelArg( voxelize_kernel, 7, sizeof(cl_int4), pool_dimension.data() );
        err |= clSetKernelArg( voxelize_kernel, 9, sizeof(cl_mem), (void *) &pool_cl );
        if (err != CL_SUCCESS)
        {
            writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        }


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

            unsigned int confirmed_nodes = 0, non_empty_node_counter = 0;
            int method;

            // Cycle through the levels
            QElapsedTimer timer;
            for (size_t lvl = 0; lvl < svo->getLevels(); lvl++)
            {
                size_t cpu_counter = 0;
                size_t gpu_counter = 0;

                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                // Find the correct range search radius
                float search_radius = sqrt(3.0f)*0.5f*((svo->getExtent()->at(1)-svo->getExtent()->at(0))/ (svo->getBrickInnerDimension()*(1 << lvl)));
                if (search_radius < (*suggested_search_radius_high)) search_radius = (*suggested_search_radius_high);

                //~std::cout << "search_radius: "<< search_radius << std::endl;

                double tmp = (svo->getExtent()->at(1)-svo->getExtent()->at(0))/(1 << lvl);

                // For each node
                size_t iter = 0;

                //~QElapsedTimer timer_total, timer_spec;
                //~timer_total.start();
                //~timer_spec.start();
                //~size_t t0 = 0, t1 = 0, t2 = 0, t_total;

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

                    MiniArray<double> brick_extent(6);
                    brick_extent[0] = svo->getExtent()->at(0) + tmp*brickId[0];
                    brick_extent[1] = svo->getExtent()->at(0) + tmp*(brickId[0]+1);
                    brick_extent[2] = svo->getExtent()->at(2) + tmp*brickId[1];
                    brick_extent[3] = svo->getExtent()->at(2) + tmp*(brickId[1]+1);
                    brick_extent[4] = svo->getExtent()->at(4) + tmp*brickId[2];
                    brick_extent[5] = svo->getExtent()->at(4) + tmp*(brickId[2]+1);

                    float * brick_data = new float[n_points_brick];

                    // Set brick-specific arguments
                    err = clSetKernelArg( voxelize_kernel, 8, sizeof(cl_int), (void *) &non_empty_node_counter );
                    if (err != CL_SUCCESS)
                    {
                        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
                    }

                    //~t0 += timer_spec.nsecsElapsed();
                    //~timer_spec.restart();
                    int isEmpty = root.getBrick(brick_data,
                        &brick_extent,
                        1.0,
                        search_radius,
                        svo->getBrickOuterDimension(),
                        lvl,
                        &items_cl,
                        &brick_extent_cl,
                        &target_cl,
                        &voxelize_kernel,
                        queue,
                        &method);


                    if (method == 0) gpu_counter++;
                    if (method == 1) cpu_counter++;

                    //~t1 += timer_spec.nsecsElapsed();
                    //~timer_spec.restart();
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
                    //~t2 += timer_spec.nsecsElapsed();
                    //~timer_spec.restart();
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

                //~t_total = timer_total.nsecsElapsed();
                //~std::cout << "L " << lvl << " t0: "<< t0 << " ns " << t0*100/t_total << "% t1: "<< t1 << " ns "  << t1*100/t_total << "% t2: "<< t0 << " ns "  << t2*100/t_total << "%" << std::endl;
                std::cout << "L " << lvl << " cpu: "<< 100*cpu_counter/(cpu_counter+gpu_counter) << "%, gpu: " << 100*gpu_counter/(cpu_counter+gpu_counter) << "%" << std::endl;
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
                            svo->pool[n_points_brick*iter + j] = gpuHelpOcttree[i].getBrick()[j]; //Omfg this is so wasteful
                        }
                        iter++;
                    }
                    emit changedGenericProgress((i+1)*100/confirmed_nodes);
                }

                // Round up to the lowest number of bricks that is multiple of the brick pool dimensions. Use this value to reserve data for the data pool
                unsigned int non_empty_node_counter_rounded_up = non_empty_node_counter + ((pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) - (non_empty_node_counter % (pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension()))));

                // Allocate resources
                svo->pool.set(non_empty_node_counter_rounded_up*n_points_brick, 0.0);

                // Read results
                err = clEnqueueReadBuffer ( *queue,
                    pool_cl,
                    CL_TRUE,
                    0,
                    non_empty_node_counter*n_points_brick*sizeof(cl_float),
                    svo->pool.data(),
                    0, NULL, NULL);
                if (err != CL_SUCCESS)
                {
                    writeLog("[!][SearchNode] Error before line "+QString::number(__LINE__)+":"+QString(cl_error_cstring(err)));
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

        clReleaseMemObject(pool_cl);
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
    if (isCLInitialized) clReleaseKernel(project_kernel);
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
            file.setProjectionKernel(&project_kernel);
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
