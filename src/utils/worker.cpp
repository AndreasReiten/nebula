#include "worker.h"

static const size_t REDUCED_PIXELS_MAX_BYTES = 1e9;
static const size_t BRICK_POOL_SOFT_MAX_BYTES = 7e8;


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

void BaseWorker::setOpenCLContext(ContextCL * context)
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


void BaseWorker::setFiles(QList<PilatusFile> * files)
{

    this->files = files;
}
void BaseWorker::setReducedPixels(MiniArray<float> * reduced_pixels)
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
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No paths specified!");

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
            QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(file_paths->size())+"!");

            emit changedMessageString(str);
            files->clear();

            break;
        }

        // Set file and get status
        files->append(PilatusFile());
        int STATUS_OK = files->back().set(file_paths->at(i), context_cl);
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
//    emit enableSetFileButton(false);
//    emit enableReadFileButton(false);
//    emit enableProjectFileButton(false);
//    emit enableVoxelizeButton(false);
//    emit enableAllInOneButton(false);
//    emit showGenericProgressBar(true);
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
            QString str("\n["+QString(this->metaObject()->className())+"] Warning: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");
            emit changedMessageString(str);
            break;
        }

        // Read file and get status
        int STATUS_OK = (*files)[i].readData();
        size_raw += (*files)[i].getBytes();
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
    paths[0] = "cl_kernels/project.cl";

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
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No files specified!");

        emit changedMessageString(str);

        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Correcting and Projecting "+QString::number(files->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));

    Matrix<float> test_background;

    test_background.set(1679, 1475, 0.0);

    QElapsedTimer stopwatch;
    stopwatch.start();

    kill_flag = false;

    size_t n = 0;
    reduced_pixels->reserve(REDUCED_PIXELS_MAX_BYTES/sizeof(float));

    for (size_t i = 0; i < (size_t) files->size(); i++)
    {
        // Kill process if requested
        if (kill_flag)
        {
            QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");

            emit changedMessageString(str);
            break;
        }

        // Project and correct file and get status
        if (n > reduced_pixels->size())
        {
            std::cout << "TOO MUCH for reduced_pixels: " << n << " > " << reduced_pixels->size()<< std::endl;
            // Break if there is too much data.
            emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Error: There was too much data!"));
            kill_flag = true;
        }
        else
        {
//            qDebug() << "New measurements ------";
//            QElapsedTimer timer;
//            timer.start();
//            emit testToWindow();
//            qDebug() << timer.restart() << " (this->main->window)";
//            emit testToMain();
//            qDebug() << timer.restart() << " (this->window)";

            emit changedImageSize(files->at(i).getWidth(), files->at(i).getHeight());
//            qDebug() << timer.restart() << " First signal sent";

            (*files)[i].setProjectionKernel(&project_kernel);
//            qDebug() << timer.restart();

            (*files)[i].setBackground(&test_background, files->front().getFlux(), files->front().getExpTime());
//            qDebug() << timer.restart();

            emit aquireSharedBuffers();
//            qDebug() << timer.restart()<< " (Before launch)" << " Second signal sent";

            int STATUS_OK = (*files)[i].filterData( &n, reduced_pixels->data(), *threshold_reduce_low, *threshold_reduce_high, *threshold_project_low, *threshold_project_high,1);
//            qDebug() << timer.restart()<< " (After launch)";

            emit releaseSharedBuffers();
//            qDebug() << timer.restart() << " Third signal sent";

            emit updateRequest();
//            qDebug() << timer.restart() << " Update signal sent";


            if (STATUS_OK)
            {
//                emit repaintImageWidget();
                //--QCoreApplication::processEvents();
            }
            else
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Error: could not process data \""+files->at(i).getPath()+"\"");
                kill_flag = true;
            }
//            qDebug() << timer.restart();
        }
        // Update the progress bar
        emit changedGenericProgress(100*(i+1)/files->size());

    }
    size_t t = stopwatch.restart();

    std::cout << "RESIZING for reduced_pixels: " << n << std::endl;
    reduced_pixels->resize(n);

    /* Create dummy dataset for debugging purposes.
     *
    */
    if (0) // A sphere
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


    kill_flag = false;
    if (file_paths->size() <= 0)
    {
        QString str("\n["+QString(this->metaObject()->className())+"] Error: No paths specified!");

        emit changedMessageString(str);
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Treating "+QString::number(file_paths->size())+" files...");
    emit changedFormatGenericProgress(QString("Progress: %p%"));
//    emit enableSetFileButton(false);
//    emit enableReadFileButton(false);
//    emit enableProjectFileButton(false);
//    emit enableVoxelizeButton(false);
//    emit enableAllInOneButton(false);
//    emit showGenericProgressBar(true);
    emit changedTabWidget(1);

    // Parameters for Ewald's projection
    Matrix<float> test_background;
    test_background.set(1679, 1475, 0.0);
    std::cout << "RESERVING for reduced_pixels: " << REDUCED_PIXELS_MAX_BYTES/sizeof(float) << std::endl;
    reduced_pixels->reserve(REDUCED_PIXELS_MAX_BYTES/sizeof(float));

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
            QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(file_paths->size())+"!");

            emit changedMessageString(str);
//            std::cout << "CLEARING for reduced_pixels" << std::endl;
            reduced_pixels->clear();

            break;
        }

        // Set file and get status
        PilatusFile file;
        int STATUS_OK = file.set(file_paths->at(i), context_cl);
        file.setOpenCLBuffers(alpha_img_clgl, beta_img_clgl, gamma_img_clgl, tsf_img_clgl);

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
                    emit changedMessageString(QString("\n["+QString(this->metaObject()->className())+"] Error: There was too much data!"));
                    kill_flag = true;
                }
                else
                {
                    emit changedImageSize(file.getWidth(), file.getHeight());

                    file.setProjectionKernel(&project_kernel);
                    file.setBackground(&test_background, file.getFlux(), file.getExpTime());

                    emit aquireSharedBuffers();
                    int STATUS_OK = file.filterData( &n, reduced_pixels->data(), *threshold_reduce_low, *threshold_reduce_high, *threshold_project_low, *threshold_project_high,1);
                    emit releaseSharedBuffers();
                    emit updateRequest();

                    if (STATUS_OK)
                    {
//                        emit repaintImageWidget();

                        // Get suggestions on the minimum search radius that can safely be applied during interpolation
                        if ((*suggested_search_radius_low) > file.getSearchRadiusLowSuggestion()) (*suggested_search_radius_low) = file.getSearchRadiusLowSuggestion();
                        if ((*suggested_search_radius_high) < file.getSearchRadiusHighSuggestion()) (*suggested_search_radius_high) = file.getSearchRadiusHighSuggestion();

                        // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
                        if ((*suggested_q) < file.getQSuggestion()) (*suggested_q) = file.getQSuggestion();

                        n_ok_files++;
                    }
                    else
                    {
                        emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Error: could not process data \""+files->at(i).getPath()+"\"");
                        kill_flag = true;
                    }
                }
            }
            else if (!STATUS_OK)
            {
                QString str("\n["+QString(this->metaObject()->className())+"] Error: could not read \""+files->at(i).getPath()+"\"");

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
    reduced_pixels->resize(n);

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
    paths[0] = "cl_kernels/voxelize.cl";

    program = context_cl->createProgram(&paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    voxelize_kernel = clCreateKernel(program, "voxelize", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isCLInitialized = true;
}

void VoxelizeWorker::process()
{


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

        // Prepare the brick pool
        MiniArray<int> pool_dimension(4, 0);
        pool_dimension[0] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
        pool_dimension[1] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
        pool_dimension[2] = (BRICK_POOL_SOFT_MAX_BYTES/(sizeof(cl_float)*svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) / ((1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower()));

        if (pool_dimension[2] < 1) pool_dimension[2] = 1;
        pool_dimension[2] *= svo->getBrickOuterDimension();

        svo->pool.set(pool_dimension[0]*pool_dimension[1]*pool_dimension[2], 0);
        size_t BRICK_POOL_HARD_MAX_BYTES = svo->pool.bytes();

        size_t n_max_bricks = BRICK_POOL_HARD_MAX_BYTES/(n_points_brick*sizeof(float));

        cl_mem pool_cl = clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            svo->pool.bytes(),
            svo->pool.data(),
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        svo->pool.clear();

        // Other CL buffers
        cl_mem items_cl =  clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            1024*2*2*2*2*2*2*2*16,
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


        cl_mem brick_extent_cl =  clCreateBuffer(*context_cl->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
            6*sizeof(float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


        err = clSetKernelArg( voxelize_kernel, 0, sizeof(cl_mem), (void *) &items_cl);
        err |= clSetKernelArg( voxelize_kernel, 1, sizeof(cl_mem), (void *) &brick_extent_cl);
        err |= clSetKernelArg( voxelize_kernel, 7, sizeof(cl_int4), pool_dimension.data());
        err |= clSetKernelArg( voxelize_kernel, 9, sizeof(cl_mem), (void *) &pool_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


        // Generate an octtree data structure from which to construct bricks
        SearchNode root(NULL, svo->getExtent()->data());
        root.setContextCL(context_cl);

        for (size_t i = 0; i < reduced_pixels->size()/4; i++)
        {
            if (kill_flag)
            {
                QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(reduced_pixels->size()/4)+"!");

                emit changedMessageString(str);
                break;
            }
            root.insert(reduced_pixels->data()+i*4);
            emit changedGenericProgress((i+1)*100/(reduced_pixels->size()/4));
        }

        if (!kill_flag)
        {
            /* Create an octtree from brick data. The nodes are maintained in a linear array rather than on the heap. This is due to lack of proper support for recursion on GPUs */
            MiniArray<BrickNode> gpuHelpOcttree(n_max_bricks*8);
            gpuHelpOcttree[0].setParent(0);
            MiniArray<unsigned int> nodes;
            nodes.set(64, (unsigned int) 0);
            nodes[0] = 1;

            unsigned int confirmed_nodes = 0, non_empty_node_counter = 0;
            int method;

            // Cycle through the levels
            QElapsedTimer timer;
            for (size_t lvl = 0; lvl < svo->getLevels(); lvl++)
            {


                qDebug() << "Level " << lvl;


                size_t cpu_counter = 0;
                size_t gpu_counter = 0;

                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+")");
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(svo->getBrickInnerDimension() * (1 <<  lvl))+"): %p%");

                timer.start();

                // Find the correct range search radius
                float search_radius = sqrt(3.0f)*0.5f*((svo->getExtent()->at(1)-svo->getExtent()->at(0))/ (svo->getBrickInnerDimension()*(1 << lvl)));
                if (search_radius < (*suggested_search_radius_high)) search_radius = (*suggested_search_radius_high);

                double tmp = (svo->getExtent()->at(1)-svo->getExtent()->at(0))/(1 << lvl);
                // For each node
                size_t iter = 0;
                for (size_t i = 0; i < nodes[lvl]; i++)
                {
                    if((non_empty_node_counter+1) >= n_max_bricks)
                    {
                        QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed due to memory overflow. The dataset has grown too large! ("+QString::number(non_empty_node_counter*n_points_brick*sizeof(cl_float)/1e6, 'g', 3)+" MB)");
                        emit changedMessageString(str);
                        kill_flag = true;
                    }

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

                    // Set brick-specific arguments
                    err = clEnqueueWriteBuffer(*context_cl->getCommandQueue(),
                        brick_extent_cl ,
                        CL_TRUE,
                        0,
                        brick_extent.toFloat().bytes(),
                        brick_extent.toFloat().data(),
                        0, NULL, NULL);
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

                    err = clSetKernelArg( voxelize_kernel, 8, sizeof(cl_int), (void *) &non_empty_node_counter );
                    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

                    // Calculate the brick data // Next step really slow on Windows

                    qDebug() << context_cl->getContext();
                    int isEmpty = root.getBrick(
                        &brick_extent,
                        search_radius,
                        svo->getBrickOuterDimension(),
                        lvl,
                        &items_cl,
                        &pool_cl,
                        &voxelize_kernel,
                        &method,
                        non_empty_node_counter,
                        svo->getBrickPoolPower());

                    if (method == 0) gpu_counter++;
                    if (method == 1) cpu_counter++;
                    if (isEmpty)
                    {
                        gpuHelpOcttree[currentId].setDataFlag(0);
                        gpuHelpOcttree[currentId].setMsdFlag(1);
                        gpuHelpOcttree[currentId].setChild(0);
                    }
                    else
                    {
                        gpuHelpOcttree[currentId].setDataFlag(1);
                        gpuHelpOcttree[currentId].setMsdFlag(0);
                        if (lvl >= svo->getLevels() - 1) gpuHelpOcttree[currentId].setMsdFlag(1);
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
                }
                if (kill_flag)
                {

                    QString str("\n["+QString(this->metaObject()->className())+"] Error: Process killed at iteration "+QString::number(lvl)+" of "+QString::number(svo->getLevels())+"!");

                    emit changedMessageString(str);
                    break;
                }

                confirmed_nodes += nodes[lvl];

                size_t t = timer.restart();
                emit changedMessageString(" ...done (time: "+QString::number(t)+" ms)");
                //~if (cpu_counter+gpu_counter > 0) std::cout << "L " << lvl << " cpu: "<< 100*cpu_counter/(cpu_counter+gpu_counter) << "%, gpu: " << 100*gpu_counter/(cpu_counter+gpu_counter) << "%" << std::endl;
            }

            if (!kill_flag)
            {
                // Use the node structure to populate the GPU arrays
                emit changedFormatGenericProgress("["+QString(this->metaObject()->className())+"]"+QString(" Transforming: %p%"));
                svo->index.reserve(confirmed_nodes);
                svo->brick.reserve(confirmed_nodes);

                for (size_t i = 0; i < confirmed_nodes; i++)
                {
                    svo->index[i] = getOctIndex(gpuHelpOcttree[i].getMsdFlag(), gpuHelpOcttree[i].getDataFlag(), gpuHelpOcttree[i].getChild());
                    svo->brick[i] = getOctBrick(gpuHelpOcttree[i].getPoolId()[0], gpuHelpOcttree[i].getPoolId()[1], gpuHelpOcttree[i].getPoolId()[2]);

                    emit changedGenericProgress((i+1)*100/confirmed_nodes);
                }

                // Round up to the lowest number of bricks that is multiple of the brick pool dimensions. Use this value to reserve data for the data pool
                unsigned int non_empty_node_counter_rounded_up = non_empty_node_counter + ((pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) - (non_empty_node_counter % (pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension()))));

                // Read results
                svo->pool.reserve(non_empty_node_counter_rounded_up*n_points_brick);

                err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
                    pool_cl,
                    CL_TRUE,
                    0,
                    svo->pool.bytes(),
                    svo->pool.data(),
                    0, NULL, NULL);
                if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            }

            //~std::cout << "reduced_pixels.max() " << reduced_pixels->max() << std::endl;
            //~std::cout << "reduced_pixels.min() " << reduced_pixels->min() << std::endl;
//~
            //~std::cout << "pool.max() " << svo->pool.max() << std::endl;
            //~std::cout << "pool.min() " << svo->pool.min() << std::endl;

            if (!kill_flag)
            {
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Total size (uncompressed): "+QString::number((svo->getBytes())/1e6, 'g', 3)+" MB");
                emit changedMessageString("\n["+QString(this->metaObject()->className())+"] Number of bricks: "+QString::number(confirmed_nodes));
            }
        }
        clReleaseMemObject(items_cl);
        clReleaseMemObject(pool_cl);
    }

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
    this->isCLInitialized = false;
    test_background.set(1475, 1679, 0.0);
}

DisplayFileWorker::~DisplayFileWorker()
{

    if (isCLInitialized) clReleaseKernel(project_kernel);
}

void DisplayFileWorker::setDisplayFile(int value)
{
    display_file = value;
}


void DisplayFileWorker::process()
{

    PilatusFile file;

    int STATUS_OK = file.set(file_paths->at(display_file), context_cl);
    if (STATUS_OK)
    {
        file.setOpenCLBuffers(alpha_img_clgl, beta_img_clgl, gamma_img_clgl, tsf_img_clgl);
        STATUS_OK = file.readData();
        if (STATUS_OK)
        {
//            emit changedImageWidth(file.getWidth());
//            emit changedImageHeight(file.getHeight());
            emit changedImageSize(file.getWidth(), file.getHeight());
            file.setProjectionKernel(&project_kernel);
            file.setBackground(&test_background, file.getFlux(), file.getExpTime());

            size_t n;
            emit aquireSharedBuffers();
            STATUS_OK = file.filterData( &n, NULL, *threshold_reduce_low, *threshold_reduce_high, *threshold_project_low, *threshold_project_high, 0);
            emit releaseSharedBuffers();
//            if (STATUS_OK)
//            {
////                semit repaintImageWidget();
//            }
        }
    }
    emit finished();
}
