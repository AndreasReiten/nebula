/* Copyright 2013 Andreas Reiten
 * This file is part of Nebula.
 *
 * Nebula is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation (version 3).
 *
 * Nebula is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nebula.  If not, see <http://www.gnu.org/licenses/>. */

/* Notes:
 * Some cl and gl resources are not released before program exit
 * Some clSetKernelArg calls are redundant
 * Make container classes for OpenCL objects, possibly in QOpenCLFunctions, that automate destruction and can tell if an object has been initialized. OpenCL functions can be modified to take them as input arguments.
 * (Done) Make QOpenCL class similar to QOpenGLFunctions
 * (Time consuming) Switch to QOpenGLWidget
 * (Somewhat time consuming) Remove hard memory requirements in cases where information about max RAM is known (case for vRAM). Adapt to hardware.
 * (Somewhat time consuming) Implement LP correction with convenient GUI. Polarization corr. based on beam polarization. Lorentz correction based on sample rotation axis (given by two angles)
 * (Somewhat time consuming) Implement clutter removal, median filter, flux and exposure normalization
 * (Quick) Save all parameters in save files and make the program relaxed regarding version. "Use what you can"-approach
 * When generating an octree, allow to save at any level less than or equal to the max chosen one.
 * (Time consuming) Implement a transfer function editor.
 * (Time consuming) Shadows
 * (Time consuming) Pre-integrated transfer functions
 * (Quick) Try opening folder structure in file browser when opening project file. Ability to append more unique files if requested.
 * (Low priority) Radial selectivity. I. e. confining viewable area to a spherical shell
 * (Time consuming) A solid linear texture edit (tsf function)
 * (Done) Area selection prior to reduction to limit data size.
 * (Done) Align slice frame to normal to a*, b* and c*.
 * (Done) Persistent screenshot dir.
 * (Done, quick fixed) Going from slice mode to integrate mode is bugged.
 * (Done on GUI level) Remove PCT threshold altogether. It can no longer be justified if the early box termination based on variance becomes successful. This could also be where the problem with voxelize lies.
 * (Somewhat time consuming) Too much VRAM is being used and without proper warnigns when too much is used
 * (Somewhat time consuming) Make native save/load (<< and >>)functions for svo's and possibly reconstruction projects
 * (Done, but early MSD features are still very flawed) Vincinity check for bricks. Bricks can not be made MSD early if some voxels are filled, but others are zero. Would help smoothen surface features. Basically check for voxels of value zero in each brick. It is also flawed in that it disallows MSD for cases where a brick completely envelops a small feature.
 * (Difficult, and variance should be found from raw data, not interpolated) Proper setting of transparent voxels. There are artefacts.
 * Other interpolation modes than IDW to get rid of blobbyness. IDW great unless sparsely populated.
 * UB matrix input
 *
 *
 *
 * In the intermediate tree, when a certain conditions are met, points are merged rather than added. Points can also be added directly into the intermediate octree rather than having them put into a list first.
 * Conditional rebinning could work. Placing values directly into intermediate octree and either rebin or split octnodes depending on data properties when they exceed a given number of points. If the contained data is smooth, then rebinning to n**3 samples is ok.
 * In lack of an easy-to-implement way of generating a both lossless and moderate intermediate data interpolation structure, it might be that values must simply be added to a "giant" octree, and only let compression ensue when the tree is deeper (more resolved) than the resolution dictated by the intersected data points (strict rebinning), or a size limit is reached  (conditional upwards rebinning, first relaxed then strict).
 * Integration boxes. You pick which side or line within the box is to be integrated along (and in which direction). Arrows indicate direction.
 *
 * Option in visualization to "enhance for visibility, at which time the max value in the current frame is read and scaled for to best use colour to show difference.
 * Contour plot
 *
 * Remove bloated/unused/poor functionality in a "Nebula Light" branch
 * */

#include <QApplication>
#include <QIcon>
#include <QDateTime>
#include <QString>
#include <QDebug>
#include <QByteArray>
#include <QStyleFactory>
#include <QLocale>

//#include <QMessageBox>

#include "mainwindow.h"

void writeToLogAndPrint(QString text, QString file, bool append)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString("[" + dateTime.toString("hh:mm:ss") + "] ");

    std::ofstream myfile (file.toStdString().c_str(), std::ios::out | ((append == true) ? std::ios::app : std::ios::trunc));

    if (myfile.is_open())
    {
        myfile << dateTimeString.toStdString().c_str() << text.toStdString().c_str() << std::endl;
        std::cout << "[Log]" << dateTimeString.toStdString().c_str() << text.toStdString().c_str() << std::endl;
    }
    else
    {
        std::cout << "Unable to open log file" << std::endl;
    }
}

void appOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    switch (type)
    {
        case QtDebugMsg:
            writeToLogAndPrint("Debug: " +
                               QString(localMsg.constData()) + " (" +
                               QString(context.file) + ":" +
                               QString::number(context.line) + ", " +
                               QString(context.function) + ")", "nebula.log", 1);
            break;

        case QtWarningMsg:
            writeToLogAndPrint("Warning: " +
                               QString(localMsg.constData()) + " (" +
                               QString(context.file) + ":" +
                               QString::number(context.line) + ", " +
                               QString(context.function) + ")", "nebula.log", 1);
            break;

        case QtCriticalMsg:
            writeToLogAndPrint("Critical: " +
                               QString(localMsg.constData()) + " (" +
                               QString(context.file) + ":" +
                               QString::number(context.line) + ", " +
                               QString(context.function) + ")", "nebula.log", 1);
            break;

        case QtFatalMsg:
            writeToLogAndPrint("Fatal: " +
                               QString(localMsg.constData()) + " (" +
                               QString(context.file) + ":" +
                               QString::number(context.line) + ", " +
                               QString(context.function) + ")", "nebula.log", 1);
            abort();
    }
}


/* This is the top level of the GUI application*/
int main(int argc, char ** argv)
{
    // Fusion style
    QLocale::setDefault(QLocale::C);
    qApp->setStyle(QStyleFactory::create("Fusion"));

    // Initialize the log file
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString(dateTime.toString("dd/MM/yyyy hh:mm:ss"));
    writeToLogAndPrint("### NEBULA LOG " + dateTimeString + " ###", "nebula.log", 0);

    // Handle Qt messages
    qInstallMessageHandler(appOutput);

    // Register custom objects for signal/slot transfer
    qRegisterMetaType<ImageInfo>();
    qRegisterMetaType<ImageSeries>();
    qRegisterMetaType<SeriesSet>();
    qRegisterMetaType<Selection>();
    qRegisterMetaType<DetectorFile>();
    qRegisterMetaType<Line>();
//    qRegisterMetaType<Box>();
    qRegisterMetaType<Matrix<double>>();
    qRegisterMetaType<Matrix<double>>();
    qRegisterMetaType<Matrix<float>>();
    qRegisterMetaType<Matrix<int>>();
    qRegisterMetaType<Matrix<size_t>>();
    qRegisterMetaType<Matrix<char>>();
    qRegisterMetaType<Matrix<uchar>>();
    qRegisterMetaType<Matrix<uint>>();

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/art/app.png"));

    app.setOrganizationName("Norwegian University of Science and Technology");
    app.setApplicationName("Nebula");

    // Check hardware and software compatability

    // Qt version
//    QString qt_str = QT_VERSION_STR;

//    // OpenGL version
//    QString opengl_str = "";

//    // OpenCL version
//    QString opencl_str = "";

//    // Devices
//    QString device_str = "";

//    QString message = "Compiled with Qt version " + qt_str +
//                    + "OpenGL version " + opengl_str + opencl_str + device_str;

//    QString title = "System compatability overview";

//    QMessageBox box(QMessageBox::Information, title, message);
//    box.exec();

    MainWindow window;
    window.setAnimated(false);
    window.show();

    return app.exec();
}
