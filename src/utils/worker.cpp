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
    // you could copy data from constructor arguments to internal variables here.
}

BaseWorker::~BaseWorker()
{
    // free resources
}

void BaseWorker::killProcess()
{
    kill_flag = true;
    //~std::cout << "Setting kill flag: "<< kill_flag << std::endl;
}

void BaseWorker::setFilePaths(QStringList * file_paths)
{
    this->file_paths = file_paths;
}
void BaseWorker::setBrickInfo(int brick_inner_dimension, int brick_outer_dimension)
{
    this->brick_inner_dimension = brick_inner_dimension;
    this->brick_outer_dimension = brick_outer_dimension;
}
void BaseWorker::setFiles(QList<PilatusFile> * files)
{
    this->files = files;
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
    // you could copy data from constructor arguments to internal variables here.
}

SetFileWorker::~SetFileWorker()
{
    // free resources
}

void SetFileWorker::process()
{
    //~test_background.set(1679, 1475, 0.0);
    QCoreApplication::processEvents();
    kill_flag = false;

    if (file_paths->size() <= 0)
    {
        QString str("\n[Set] Error: No paths specified!");
        emit error(str);
        emit changedMessageString(str);
        emit abort();
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n[Set] Setting "+QString::number(file_paths->size())+" files (headers etc.)...");
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
        QCoreApplication::processEvents();
        if (kill_flag)
        {
            QString str("\n[Set] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(file_paths->size())+"!");
            emit error(str);
            emit changedMessageString(str);
            files->clear();
            emit abort();
            break;
        }

        // Set file and get status
        files->append(PilatusFile());
        int STATUS_OK = files->back().set(file_paths->at(i), context, queue, projection_kernel, imageRenderWidget);
        if (STATUS_OK)
        {
            // Get suggestions on the minimum search radius that can safely be applied during interpolation
            if (suggested_search_radius_low > files->back().getSearchRadiusLowSuggestion()) suggested_search_radius_low = files->back().getSearchRadiusLowSuggestion();
            if (suggested_search_radius_high < files->back().getSearchRadiusHighSuggestion()) suggested_search_radius_high = files->back().getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if (suggested_q < files->back().getQSuggestion()) suggested_q = files->back().getQSuggestion();

            // Set the background that will be subtracted from the data
            //~files->back().setBackground(&test_background, files->front().getFlux(), files->front().getExpTime());
        }
        else
        {
            files->removeLast();
            emit changedMessageString("\n[Set] Warning: Could not process \""+QString(file_paths->at(i))+"\"");
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

        emit changedMessageString("\n[Set] "+QString::number(files->size())+" of "+QString::number(file_paths->size())+" files were successfully set (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");

        // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
        float resolution_min = 2*suggested_q/suggested_search_radius_high;
        float resolution_max = 2*suggested_q/suggested_search_radius_low;

        float level_min = std::log(resolution_min/(float)this->brick_inner_dimension)/std::log(2);
        float level_max = std::log(resolution_max/(float)this->brick_inner_dimension)/std::log(2);

        emit changedMessageString("\n[Set] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n[Set] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
        emit changedMessageString("\n[Set] Suggesting minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
        emit changedMessageString("\n[Set] Suggesting minimum octtree level: "+QString::number(level_min, 'f', 2)+" to "+QString::number(level_max, 'f', 2)+"");
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
    // you could copy data from constructor arguments to internal variables here.
}

ReadFileWorker::~ReadFileWorker()
{
    // free resources
}

void ReadFileWorker::process()
{
    QCoreApplication::processEvents();

    if (files->size() <= 0)
    {
        QString str("\n[Read] Error: No files specified!");
        emit error(str);
        emit changedMessageString(str);
        emit abort();
        kill_flag = true;
    }

    // Emit to appropriate slots
    emit changedMessageString("\n[Read] Reading "+QString::number(files->size())+" files...");
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
        QCoreApplication::processEvents();
        if (kill_flag)
        {
            QString str("\n[Read] Error: Process killed at iteration "+QString::number(i)+" of "+QString::number(files->size())+"!");
            emit error(str);
            emit changedMessageString(str);
            break;
        }

        // Read file and get status
        int STATUS_OK = (*files)[i].readData();
        size_raw += (*files)[i].getBytes();
        if (!STATUS_OK)
        {
            emit changedMessageString("\n[Read] Error: could not read \""+files->at(i).getPath()+"\"");
            emit abort();
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

        emit changedMessageString("\n[Read] "+QString::number(files->size())+" files were successfully read ("+QString::number(size_raw/1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number((float)t/(float)files->size(), 'g', 3)+" ms/file)");
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
    // you could copy data from constructor arguments to internal variables here.
}

ProjectFileWorker::~ProjectFileWorker()
{
    // free resources
}

void ProjectFileWorker::process()
{
    // allocate resources using new here
    qDebug("Hello World!");
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
    // you could copy data from constructor arguments to internal variables here.
}

AllInOneWorker::~AllInOneWorker()
{
    // free resources
}

void AllInOneWorker::process()
{
    // allocate resources using new here
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
    // you could copy data from constructor arguments to internal variables here.
}

VoxelizeWorker::~VoxelizeWorker()
{
    // free resources
}

void VoxelizeWorker::process()
{
    // allocate resources using new here
    qDebug("Hello World!");
    emit finished();
}
